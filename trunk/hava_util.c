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
#include <stdio.h>
#include <assert.h>

//#define DEBUG_MODE 1

#include "hava_util.h"
#include "hava_util_internals.h"

void make_nonblocking();
void print_the_packet(Hava *h,int len);

Hava *Hava_alloc(const char *havaip) {
  Hava *h;
  int i;
  h=(Hava*)malloc(sizeof(Hava));
  assert(h);
  for(i=0;i<sizeof(Hava);i++) {
    ((char*)h)[i]=0;
  } 

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

  h->si.sin_family=AF_INET;
  h->si.sin_addr.s_addr=inet_addr(havaip);
  h->si.sin_port=htons(1778);
  if(bind(h->sock,(struct sockaddr*)&h->si,sizeof(h->si)>=0)) 
  {
    h->bound=1;
    make_nonblocking(h);
  } else {
    fprintf(stderr,"Socket already bound; using async mode\n");
  }

  return h;
}

int Hava_isbound(Hava *hava) {
  return hava->bound;
}

extern void Hava_set_videoendtime(Hava *hava, time_t et) {
  hava->vid_endtime=et;
}

void Hava_set_videocb(Hava *hava, void (*vcb)(const char *buf,int len)) {
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
  WSADATA global_wsa;
  WSACleanup();
#endif
}

void Hava_startup() {
#ifdef VSTUDIO
  int ret=WSAStartup(0x202,&global_wsa);
  if(ret!=0) {
    fprintf(stderr,"ERROR INITIALIZING WINSOCK\n");
    winsock_done();
    exit(1);
  }
#endif
}

int check_for_end(Hava *hava) {
  struct timeval tv;
  gettimeofday(&tv,0);
  if(tv.tv_sec > hava->vid_endtime) {
    return 1;
  }
  return 0;
}

// return 0 if it was not a video frame
// return 1 if we processed the packet 
// return 3 if it is time to exit
//
int process_video_packet(Hava *hava, int len) {
  int tmp;
  int retval=1;
  unsigned short seq;
  FrameHeader *fh;
  fh=(FrameHeader*)&hava->buf[0];
  //
  // just worry about video payload packets
  //
  if(fh->cmdhi!=0x03 || fh->cmdlo!=0x02) {
    return 0;
  }
  //
  // Only two known packet sizes
  //
  assert(len==1470 || len==406);
  seq=fh->seqhi<<8 | fh->seqlo;

  // First time thru, do something special
  //
  if(hava->vid_starting) {
    if(hava->vid_callback) { 
      hava->vid_callback(mpeg_hdr,sizeof(mpeg_hdr)); 
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
    fprintf(stderr,"Out of order video packet!!! 0x%04x not 0x%04x\n",
           seq,hava->vid_seq);
  }
  hava->vid_seq=++seq;

#ifdef DEBUG_MODE
//  fprintf(stderr,"SEQUENCE ID=%04x remaining=%d\n",seq,fh->stream_remaining);
#endif
  if(fh->stream_remaining==0) {
    Hava_sendcmd(hava,HAVA_CONT_VIDEO,seq); 
#ifdef DEBUG_MODE
    fprintf(stderr,"sending continuation from 0x%02x%02x\n",
               hava->mypkt_cont[SEQ_OFFSET],
               hava->mypkt_cont[SEQ_OFFSET+1]);
#endif
    if(check_for_end(hava)) { retval=3; }
  }
  if(hava->vid_callback) { 
    hava->vid_callback(&fh->payload,len-16); 
  }
  return retval;
}

int Hava_loop(Hava *hava, unsigned short magic) {
  struct sockaddr_in so;
  int i,
      ct,
      tmp,
      len,
      done=0;
  unsigned short pkt;

  for(ct=0;ct<HAVA_MAXTRIES;ct++) {
    tmp=sizeof(so);
    len=recvfrom(hava->sock,hava->buf, HAVA_BUFFSIZE, 
                 0,(struct sockaddr*)&so,&tmp);
    assert(tmp==sizeof(so));

    pkt=hava->buf[2]<<8 | hava->buf[3];
    if(len==4 && magic && pkt==magic) {
      done=1;
      ct=HAVA_MAXTRIES;
    }
    if(len>=0) {
      ct=0;
      i=process_video_packet(hava, len);
      if(i==3) { 
        done=1; 
        ct=HAVA_MAXTRIES;
      }
#ifdef DEBUG_MODE
      if(!i) { print_the_packet(hava,len); }
#endif
    } 
    else 
    {
      MSLEEP(50);
    }
  }
  fprintf(stderr,"Ack status: %d (1=success)\n",done);
  return done;
}

void Hava_sendcmd(Hava *hava, int cmd, unsigned short extra) {
  int tmp, len;
  const char *buf;
  switch (cmd) {
    case HAVA_CONT_VIDEO:
       //
       // update with sequence number
       //
       hava->mypkt_cont[SEQ_OFFSET+1]=(extra & 0x0ff);
       hava->mypkt_cont[SEQ_OFFSET]=((extra>>8) & 0x0ff);
       buf=hava->mypkt_cont;     
       len=sizeof(continue_pkt);
       break;
    case HAVA_START_VIDEO:
       buf=&start_pkt[0];        // global
       len=sizeof(start_pkt);
       break;
    case HAVA_INIT:
       buf=&init_pkt[0];         // global
       len=sizeof(init_pkt);
       break;
    case HAVA_CHANNEL:
       //
       // Update with channel number
       //
       hava->mypkt_chan[CHANNEL_OFFSET]=0;
       hava->mypkt_chan[CHANNEL_OFFSET+1]=0;
       hava->mypkt_chan[CHANNEL_OFFSET+2]=((extra>>8)&0x0ff);
       hava->mypkt_chan[CHANNEL_OFFSET+3]=(extra&0xff);
       buf=hava->mypkt_chan;
       len=sizeof(channel_set);
       break;
    case HAVA_BUTTON:
       //
       // Update with button number
       //
       hava->mypkt_butt[BUTTON_OFFSET]=(unsigned char)extra;
       buf=hava->mypkt_butt;
       len=sizeof(button_push);
       break;
    default:
      assert(0);
  }
//  fprintf(stderr,"send %d bytes\n",len); 
  tmp=sendto(hava->sock,buf,len,0,
             (struct sockaddr*)&hava->si, sizeof(hava->si)); 
  assert(tmp==len); 
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
    fprintf(stderr,"nonblocking mode failure.  using async mode...\n");
    hava->bound=0;
  }
}

void print_the_packet(Hava *hava,int len) {
  int i;
  fprintf(stderr,"len=%d\n",len);
  for(i=0;i<len;) {
    if(i && !(i%16)) { fprintf(stderr,"\n"); }
    fprintf(stderr,"%02x ",hava->buf[i]);
    if(++i==len) { fprintf(stderr,"\n"); }
  }
}

