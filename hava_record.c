/*
 * HavaFun code component
 * Code to change channels or push buttons on the Hava media streamer
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
#include "hava_util.h"

//#define DEBUG_MODE 1

#ifdef _MSC_VER
#define VSTUDIO
#endif

#ifdef VSTUDIO
#define MSLEEP(a) Sleep(a)
#define CLOSE(a) closesocket(a)
#include <windows.h>
#include <winbase.h>
//#include <winsock2.h>
#else
#define MSLEEP(a) usleep(a*1000)
#define CLOSE(a) close(a)
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>
#endif

int gsock=-1;
int gbound=0;
struct sockaddr_in gsi;
unsigned short gseq=0;
frameheader *gframe;
unsigned char *gbuf;
FILE *gof;
time_t gendtime;

#ifdef VSTUDIO
WSADATA global_wsa;
void winsock_done() {
//  printf("\nWaiting a sec before cleaning up winsock...");
//  Sleep(1000);
//  printf(" done.\n");
  WSACleanup();
}
void winsock_init() {
  int ret=WSAStartup(0x202,&global_wsa);
  if(ret!=0) {
    fprintf(stderr,"ERROR INITIALIZING WINSOCK\n");
    winsock_done();
    exit(1);
  }
}
#endif

Usage() {
  fprintf(stderr,"Usage: hava_record <hava_dotform_ipaddr> <duration_sec> <tgt_mpeg>\n");
#ifdef VSTUDIO
  winsock_done();
#endif
  exit(1);
}

int check_for_end() {
  struct timeval tv;
  gettimeofday(&tv,0);
  if(tv.tv_sec > gendtime) {
    return 1;
  }
  return 0;
}

// return 0 if it was not a video frame
// return 1 if we processed the packet 
// return 3 if it is time to exit
//
int process_video_packet(frameheader *fh, int len, unsigned short *nextseq) {
  int tmp;
  int retval=1;
  unsigned short seq;
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

  if(seq!=*nextseq) {
    //
    // panic!!!  someone should do something!
    //
    printf("Out of order video packet!!! 0x%04x not 0x%04x\n",
           seq,*nextseq);
  }
  *nextseq=(seq+1);

#ifdef DEBUG_MODE
//  printf("SEQUENCE ID=%04x remaining=%d\n",seq,fh->stream_remaining);
#endif
  if(fh->stream_remaining==0) { 
    seq=seq+1; 
    continue_pkt[SEQ_OFFSET+1]=(seq & 0x0ff);
    continue_pkt[SEQ_OFFSET]=((seq>>8) & 0x0ff);

#ifdef DEBUG_MODE
    printf("sending continuation from 0x%02x%02x\n",
               continue_pkt[SEQ_OFFSET],
               continue_pkt[SEQ_OFFSET+1]);
#endif
    SEND(continue_pkt); 
    if(check_for_end()) { retval=3; }
  }
  if(gof) { fwrite(&fh->payload,len-16,1,gof); }
  return retval;
}

int process_packets(unsigned char *buf,unsigned short endpkt) {
  struct sockaddr_in so;
  int i,
      ct,
      tmp,
      len,
      done=0;
  unsigned short pkt;

  for(ct=0;ct<HAVA_MAXTRIES;ct++) {
    tmp=sizeof(so);
    len=recvfrom(gsock,buf,HAVA_BUFFSIZE,0,(struct sockaddr*)&so,&tmp);
    assert(tmp==sizeof(so));

    pkt=buf[2]<<8 | buf[3];
    if(len==4 && endpkt && pkt==endpkt) {
      done=1;
      ct=HAVA_MAXTRIES;
    }
    if(len>=0) {
      ct=0;
      i=process_video_packet(gframe, len, &gseq);
      if(i==3) { 
        done=1; 
        ct=HAVA_MAXTRIES;
      }
#ifdef DEBUG_MODE
      if(!i) {
        printf("len=%d\n",len);
        for(i=0;i<len;) {
          if(i && !(i%16)) { printf("\n"); }
          printf("%02x ",buf[i]);
          if(++i==len) { printf("\n"); }
        }
      }
#endif
    } 
    else 
    {
      MSLEEP(50);
    }
  }
  printf("Ack status: %d (1=success)\n",done);
  return done;
}

void make_nonblocking() {
  int good=1;
#ifdef VSTUDIO
  int nb=1;
  if(ioctlsocket(gsock,FIONBIO,&nb)==SOCKET_ERROR) { good=0; }
#else
  if(fcntl(gsock,F_SETFL,O_NONBLOCK)==-1) { good=0; }
#endif
  if(!good) {
    printf("nonblocking mode failure.  using async mode...\n");
    gbound=0;
  }
}

main(int argc, char *argv[]) {
  int i,tmp,butt=0;
  int channy;
  struct timeval tv;
#ifdef VSTUDIO
  winsock_init();
#endif
  if(argc!=4) { Usage(); }
  tmp=-1;
  tmp=atoi(argv[2]);
  if(tmp<=0) { Usage(); }
  gettimeofday(&tv,0);
  gendtime=tv.tv_sec+tmp;

  gof=fopen(argv[3],"wb");
  assert(gof);
  fwrite(mpeg_hdr, sizeof(mpeg_hdr), 1, gof);

  gbuf=malloc(HAVA_BUFFSIZE);
  assert(gbuf);
  gframe=(frameheader*)&gbuf[0];

  gsock=socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
  assert(gsock>=0); 
  for(i=0;i<sizeof(gsi);i++) { ((char*)&gsi)[i]=0; }
  gsi.sin_family=AF_INET;
  gsi.sin_addr.s_addr=inet_addr(argv[1]);
  gsi.sin_port=htons(1778);
  if(bind(gsock,(struct sockaddr*)&gsi,sizeof(gsi)>=0)) 
  {
    gbound=1;
    make_nonblocking();
  } else {
    printf("Socket already bound; using async mode\n");
  }
  assert(gbound);

  SEND(init_pkt); 
  SEND(start_pkt); 

  process_packets(gbuf,0);

  fclose(gof);
  CLOSE(gsock);
#ifdef VSTUDIO
  winsock_done();
#endif
}
