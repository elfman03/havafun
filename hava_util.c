/*
 * HavaFun code component
 * Utility code to interact with the Hava...  Provides encapsulated API
 *
 * Copyright (C) 2009 Chris Elford
 *
 * This program is free software; you can redistribute it and/or modify it 
 * under the terms of the GNU General Public License as published by the Free 
 * Software Foundation; either version 2 of the License, or (at your option) 
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT 
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or 
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for 
 * more details.
 *
 * You should have received a copy of the GNU General Public License along 
 * with this program; if not, see <http://www.gnu.org/licenses>.
 *
 * Linking havafun statically or dynamically with other modules is making a 
 * combined work based on havafun. Thus, the terms and conditions of the GNU 
 * General Public License cover the whole combination.
 *
 * In addition, as a special exception, the copyright holders of havafun give 
 * you permission to combine havafun with free software programs or libraries 
 * that are released under the GNU LGPL and with code included in the standard 
 * release of "myhava.com Hava Player platform SDKs (e.g., linux, mac, windows)"
 * under "myhava.com's SDK license" (or modified versions of such code, with 
 * unchanged license). You may copy and distribute such a system following the 
 * terms of the GNU GPL for havafun and the licenses of the other code concerned
 * {, provided that you include the source code of that other code when and as 
 * the GNU GPL requires distribution of source code}.
 *
 * Note that people who make modified versions of havafun are not obligated to 
 * grant this special exception for their modified versions; it is their choice
 * whether to do so. The GNU General Public License gives permission to release
 * a modified version without this exception; this exception also makes it 
 * possible to release a modified version which carries forward this exception.
 *
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>

//#define DEBUG_MODE 1
//#define DEBUG_MODE_VERBOSE 1

#include "hava_util.h"
#include "hava_util_internals.h"

void make_exclusive(Hava *);
void make_nonblocking(Hava *);
void print_the_packet(Hava *h,int len,struct in_addr addr);

Hava *Hava_alloc(const char *havaip, int binding, int blocking,
                 FILE *logfile, int verbose) {
  struct sockaddr_in si;
  Hava *h;
  int i;
  h=(Hava*)malloc(sizeof(Hava));
  assert(h);
  for(i=0;i<sizeof(Hava);i++) {
    ((char*)h)[i]=0;
  } 

  // set up default logfile
  h->logfile=logfile;

  // allocate my continuation packet
  h->mypkt_cont=malloc(sizeof(continue_pkt));
  assert(h->mypkt_cont);
  for(i=0;i<sizeof(continue_pkt);i++) { h->mypkt_cont[i]=continue_pkt[i]; }

  // allocate my button packet
  h->mypkt_butt=malloc(sizeof(button_push));
  assert(h->mypkt_butt);
  for(i=0;i<sizeof(button_push);i++) { h->mypkt_butt[i]=button_push[i]; }

  // allocate my channel packet
  h->mypkt_chan=malloc(sizeof(channel_set));
  assert(h->mypkt_chan);
  for(i=0;i<sizeof(channel_set);i++) { h->mypkt_chan[i]=channel_set[i]; }

  h->buf=malloc(HAVA_BUFFSIZE);
  assert(h->buf);

  h->sock=socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
  assert(h->sock>=0); 
  make_exclusive(h);

  h->si.sin_family=AF_INET;
  //
  // Check if want to autodetect IP address
  //
  if(strcmp(havaip,"-")) {
    h->si.sin_addr.s_addr=inet_addr(havaip);
  } else {
    h->si.sin_addr.s_addr=INADDR_ANY;
  }
  h->si.sin_port=htons(1778);

  for(i=0;i<sizeof(si);i++) { ((char*)&si)[i]=0; }
  si.sin_family=AF_INET;
  si.sin_addr.s_addr=INADDR_ANY;
  si.sin_port=htons(1778);
  if(!binding) {
    fprintf(h->logfile,"Application requested not to bind... No recv() so no ACK checking!\n");
  } else {
    if(bind(h->sock,(struct sockaddr*)&si,sizeof(si))==0) {
      h->bound=1;
      fprintf(h->logfile,"Bind successful!  recv() possible...\n");
    } else {
      fprintf(h->logfile,"Socket already bound! no recv() possible so no ACK checking...\n");
    }
  }
  if(!blocking) {
    make_nonblocking(h);
  }

  // If want to autodetect IP address
  //
  if(h->si.sin_addr.s_addr==INADDR_ANY) {
    fprintf(h->logfile,"If we hang here a long time it means nothing came in from hava\n");
    Hava_loop(h, HAVA_MAGIC_INFO, verbose);
  }
  return h;
}

int Hava_isbound(Hava *hava) {
  return hava->bound;
}

extern void Hava_set_videoendtime(Hava *hava, time_t et) {
  hava->vid_endtime=et;
}

time_t Hava_get_videoendtime(Hava *hava) {
  return hava->vid_endtime;
}

void Hava_set_videocb(Hava *hava, 
                      void (*vcb)(Hava *hava, const char *buf, int len)) {
  hava->vid_callback=vcb;
  hava->vid_starting=1;
}

void Hava_close(Hava *hava) {
  int i;
  CLOSE(hava->sock);
  free(hava->buf);
  free(hava->mypkt_cont);
  free(hava->mypkt_butt);
  free(hava->mypkt_chan);
  for(i=0;i<sizeof(Hava);i++) { ((char*)hava)[i]=0; }
  free(hava);
}

void Hava_finishup() {
#ifdef VSTUDIO
  WSACleanup();
#endif
}

void Hava_startup(FILE *logfile) {
#ifdef VSTUDIO
  WSADATA the_wsa;
  int ret=WSAStartup(0x202,&the_wsa);
  if(ret!=0) {
    fprintf(logfile,"ERROR INITIALIZING WINSOCK\n");
    Hava_finishup();
    exit(1);
  }
#endif
}

unsigned long Hava_getnow() {
#ifdef VSTUDIO
  FILETIME times;
  unsigned __int64 t;
  GetSystemTimeAsFileTime(&times);
  t=times.dwHighDateTime;
  t=(t<<32) | times.dwLowDateTime;
  return t/1000000;
#else
  struct timeval tv;
  gettimeofday(&tv,0);
  return (unsigned long)tv.tv_sec;
#endif
}

int check_for_end(Hava *hava) {
  //
  // First check run forever mode
  //
  if(!hava->vid_endtime) {
    return 0;
  }
  //
  // Now handle the timed mode
  //
  if(Hava_getnow() > hava->vid_endtime) {
    return 1;
  }
  return 0;
}

#define HAVA_VIDEO_NOPE 0
#define HAVA_VIDEO_YES  1
#define HAVA_VIDEO_END  3

// return HAVA_VIDEO_NOPE if it was not a video frame
// return HAVA_VIDEO_YES  if we processed the packet 
// return HAVA_VIDEO_END  if it is time to exit
//
int process_video_packet(Hava *hava, int len) {
  int tmp;
  int retval=HAVA_VIDEO_YES;
  unsigned short seq;
  FrameHeader *fh;
  fh=(FrameHeader*)&hava->buf[0];
  //
  // Handle Explicit Continuation Request
  //
  if(len==8 && (fh->cmdhi==0x03) && (fh->cmdlo==0x05)) {
    Hava_sendcmd(hava,HAVA_CONT_VIDEO,0,0); 
    //
    // If Hava doesn't get continuation in time it asks again
    // offset 7 holds the count of how many times it has asked
    //
    if(hava->buf[7]) { 
      fprintf(hava->logfile,"Timing! Hava wants continuation 0x%04x %d times... 0x%02x pkts\n",hava->vid_seq,hava->buf[7]+1,hava->mypkt_cont[Xa]);
    }
#ifdef DEBUG_MODE
  fprintf(hava->logfile,"Send Cont Permission 0x%04x 0x%02x pkt\n",
                        hava->vid_seq,hava->mypkt_cont[Xa]);
#endif
    return HAVA_VIDEO_YES;
  }
  //
  // Now just worry about video payload packets
  //
  if((fh->cmdhi!=0x03) || (fh->cmdlo!=0x02)) {
    return HAVA_VIDEO_NOPE;
  }
  //
  // Only two known packet sizes
  //
  assert(len==1470 || len==406);
  seq=(fh->seqhi<<8) | (fh->seqlo);

  // First time thru, do something special
  //
  if(hava->vid_starting) {
    if(hava->vid_callback) { 
      hava->vid_callback(hava,mpeg_hdr,sizeof(mpeg_hdr)); 
    }
    hava->vid_seq=seq;
    hava->vid_starting=0;
  }
  //
  // Sequence check
  //
  if(seq!=hava->vid_seq) {
    //
    // panic!!!  someone should do something!
    //
    fprintf(hava->logfile,"Out of order video packet!!! 0x%04x not 0x%04x\n",
           seq,hava->vid_seq);
  }

#ifdef DEBUG_MODE_VERBOSE
  fprintf(hava->logfile,"SEQUENCE ID=%04x (expected 0x%04x) remaining=%d\n",seq,hava->vid_seq,fh->stream_remaining);
#endif

  seq++;
  hava->vid_seq=seq;
  if(fh->stream_remaining==0) {
    hava->mypkt_cont[SEQ_OFFSET+1]=(seq & 0x0ff);
    hava->mypkt_cont[SEQ_OFFSET]=((seq>>8) & 0x0ff);
    hava->mypkt_cont[XM]=fh->next_time_desired-1;
    hava->mypkt_cont[Xa]=fh->next_time_desired;
    hava->mypkt_cont[Xb]=fh->next_time_desired;
    Hava_sendcmd(hava,HAVA_CONT_VIDEO,0,0); 
#ifdef DEBUG_MODE
    fprintf(hava->logfile,"sending continuation(0x%02x) from 0x%02x%02x\n",
               fh->next_time_desired,
               hava->mypkt_cont[SEQ_OFFSET],
               hava->mypkt_cont[SEQ_OFFSET+1]);
#endif
    if(check_for_end(hava)) { retval=HAVA_VIDEO_END; }
  }
  if(hava->vid_callback) { 
    hava->vid_callback(hava,&fh->payload,len-16); 
  }
  return retval;
}

int Hava_loop(Hava *hava, unsigned short magic, int verbose) {
  struct sockaddr_in so;
  int printpkt=0,
      sleeptime=50,
      wasvideo=0,
      ct,
      tmp,
      len,
      done=0;
  unsigned short pkt;

#ifdef DEBUG_MODE
  printpkt=1;
#endif

  if(magic==HAVA_MAGIC_INFO) { sleeptime=500; }

  ct=0;
  for(done=0;!done;) {
    tmp=sizeof(so);
    hava->buf[2]=0;
    hava->buf[3]=0;
    so.sin_addr.s_addr=INADDR_ANY;
    //
    // Get a packet or die trying
    //
    for(pkt=0;pkt<HAVA_MAXTRIES;) {
      len=recvfrom(hava->sock,hava->buf, HAVA_BUFFSIZE, 
                   0,(struct sockaddr*)&so,&tmp);
      assert(tmp==sizeof(so));
      if(len!=-1) {               // yay! got a packet
        pkt=HAVA_MAXTRIES;
      } else {                    // Lets try some more
        pkt++;
        MSLEEP(sleeptime);
      }
    }
    if(len==-1) { done=-1; }      // Give up

    //
    // In "-" mode, autopopulate Hava address
    //
    if(len==300 && magic==HAVA_MAGIC_INFO) {
      hava->si.sin_addr.s_addr=so.sin_addr.s_addr;
    }
    //
    // Process the good packet if we got it
    //
    if(len>=0 && so.sin_addr.s_addr==hava->si.sin_addr.s_addr) {
      //
      // reset our counter on good payload; increment it on info broadcast
      //
      if(len==300) {
        if(ct>HAVA_MAXTRIES) { done=-2; }
        ct++;
      } else {
        ct=0;
      }
      //
      //
      //
      pkt=hava->buf[2]<<8 | hava->buf[3];
      //
      // Response to our initialization request
      //
      if(len==336 && magic==HAVA_MAGIC_INIT) {
        if(verbose) { printpkt=1; }
        done=1;
      }
      //
      // Periodic packet that Hava broadcasts
      //
      if(len==300 && magic==HAVA_MAGIC_INFO) {
        if(verbose) { printpkt=1; }
        done=1;
      }
      if(len==4 && magic && pkt==magic) {
        done=1;
      }
      if(!done) {
        wasvideo=process_video_packet(hava, len);
        if(wasvideo==HAVA_VIDEO_END) { 
          done=1; 
        }
      }
      if(printpkt && !wasvideo) { 
        print_the_packet(hava,len,so.sin_addr); 
      }
    }
  }
  fprintf(hava->logfile,"Ack status: %d (1=success)\n",done);
  return done;
}

void Hava_sendcmd(Hava *hava, int cmd, unsigned short eA, unsigned short eB) {
  int tmp, len;
  const char *buf;
  switch (cmd) {
    case HAVA_CONT_VIDEO:
       buf=hava->mypkt_cont;     
       len=sizeof(continue_pkt);
       break;
    case HAVA_START_VIDEO:
       buf=&start_pkt[0];        // global immutable
       len=sizeof(start_pkt);
       break;
    case HAVA_INIT:
       buf=&init_pkt[0];         // global immutable
       len=sizeof(init_pkt);
       break;
    case HAVA_CHANNEL:
       //
       // Update with channel number and input number
       //
       hava->mypkt_chan[CHANNEL_OFFSET]=0;
       hava->mypkt_chan[CHANNEL_OFFSET+1]=0;
       hava->mypkt_chan[CHANNEL_OFFSET+2]=((eA>>8)&0x0ff);
       hava->mypkt_chan[CHANNEL_OFFSET+3]=(eA&0xff);
       hava->mypkt_chan[CHANNEL_INPUT_OFFSET]=(eB&0xff);
       buf=hava->mypkt_chan;
       len=sizeof(channel_set);
       break;
    case HAVA_BUTTON:
       //
       // Update with button number and remote control codeset
       //
       hava->mypkt_butt[BUTTON_REMOTE_OFFSET_HI]=(unsigned char)(eB>>8);
       hava->mypkt_butt[BUTTON_REMOTE_OFFSET_LO]=(unsigned char)(eB&0x0ff);
       hava->mypkt_butt[BUTTON_OFFSET]=(unsigned char)eA;
       buf=hava->mypkt_butt;
       len=sizeof(button_push);
       break;
    default:
      assert(0);
  }
#ifdef DEBUG_MODE 
  fprintf(hava->logfile,"send %d bytes\n",len); 
#endif
  tmp=sendto(hava->sock,buf,len,0,
             (struct sockaddr*)&hava->si, sizeof(hava->si)); 
  assert(tmp==len); 
}

unsigned short Hava_remote_aton(const char *name) {
  int hi=0;
  int lo=-1;
  if(name[0]=='C') { hi=0x01000; }
  if(name[0]=='S') { hi=0x03000; }
  if(!hi) { return 0; }
  lo=atoi(&name[1]);
  if((lo<=0)||(lo>0x0fff)) { return 0; }
  return (unsigned short)(hi | lo);
}

char *Hava_remote_ntoa(unsigned short remote) {
  int hi,
      lo;
  char ch=0;
  char *p;
  hi=remote & 0x0f000 ;
  lo=remote & 0x00fff ;
  if(hi==0x01000) { ch='C'; }
  if(hi==0x03000) { ch='S'; }
  p=malloc(16);
  assert(p);
  if(!ch) { 
    sprintf(p,"<UNSUPPORTED>"); 
  } else {
    sprintf(p,"%c%04d",ch,lo);
  }
  return p;
}

const char *Hava_input_ntoa(unsigned char ino) {
  assert(ino<4);
  return Hava_inputs[ino];
}

int Hava_input_aton(const char *name) {
  int i;
  assert(name);
  for(i=0;i<4;i++) {
    if(Hava_inputs[i] && !strcmp(name,Hava_inputs[i])) { return i; }
  }
  if(strlen(name)==1) {
    if(name[0]=='0') { return 0; }
    if(name[0]=='1') { return 1; }
    if(name[0]=='2') { return 2; }
    if(name[0]=='3') { return 3; }
  }
  return -1;
}

unsigned char Hava_button_aton(const char *name) {
  int i;
  assert(name);
  for(i=0;i<256;i++) {
    if(Hava_buttons[i] && !strcmp(name,Hava_buttons[i])) { return i; }
  }
  //Check for hex button request
  if (name[0]=='0' && name[1]=='x')
  {
    return (unsigned char)strtol(name,(char **)NULL,16);
  }
  return 0;
}

const char *Hava_button_ntoa(unsigned char button) {
  return Hava_buttons[button];
}

void make_nonblocking(Hava *hava) {
  int good=1;
#ifdef VSTUDIO
  int nb=1;
  if(ioctlsocket(hava->sock,FIONBIO,&nb)==SOCKET_ERROR) { good=0; }
#else
  if(fcntl(hava->sock,F_SETFL,O_NONBLOCK)==-1) { good=0; }
#endif
  if(!good) {
    fprintf(hava->logfile,"nonblocking mode failure.  If you needed it, app might hang...\n");
  }
}

void make_exclusive(Hava *hava) {
  int r,val;
#ifdef VSTUDIO
  val=1;
  r=setsockopt(hava->sock,SOL_SOCKET,SO_EXCLUSIVEADDRUSE,
                            (char*)&val,sizeof(val));
  fprintf(hava->logfile,"Status of setting exclusive mode=0 (0=good)\n",r);
#endif
}

void print_the_packet(Hava *hava,int len,struct in_addr addr) {
  int i;
  fprintf(hava->logfile,"len=%d from %s\n",len,inet_ntoa(addr));
  for(i=0;i<len;) {
    if(i && !(i%16)) { fprintf(hava->logfile,"\n"); }
    if(!(i%16)) { fprintf(hava->logfile,"\t0x%04x: ",i); }
    if(!(i%2)) { fprintf(hava->logfile," "); }
    fprintf(hava->logfile,"%02x",hava->buf[i]);
    if(++i==len) { fprintf(hava->logfile,"\n"); }
  }
}

