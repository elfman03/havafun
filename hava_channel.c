/*
* Code to change channels or push buttons on the Hava media streamer
*
* Copyright (C) 2009 Chris Elford
*
* This library is free software; you can redistribute it and/or
* modify it under the terms of the GNU Lesser General Public
* License as published by the Free Software Foundation; either
* version 2.1 of the License, or (at your option) any later version.
*
* This library is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
* Lesser General Public License for more details.
*
* You should have received a copy of the GNU Lesser General Public
* License along with this library; if not, write to the Free Software
* Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <assert.h>

//#define DEBUG_MODE 1

#ifdef _MSC_VER
#define VSTUDIO
#endif

#ifdef VSTUDIO
#include <windows.h>
#include <winbase.h>
//#include <winsock2.h>
#else
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#endif

// Initialization packet
unsigned char out1[24]={ 0x03,0x00,0x00,0x00,0x00,0x00,0x00,0x0f,0x00,0x1d,0x6a,0xe1,0x2a,0x93,0x00,0x00,0x00,0x00,0x00,0x09,0x00,0x1f,0x06,0xd9, };

// Basic form of a channel changing packet
unsigned char channel_set[32]= { 0x03,0x07,0xbe,0xef,0x00,0x00,0x00,0x14,
                                 0x00,0x00,0x00,0x10,0x00,0x00,0x06,0x69,
                                 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x08, 
                                 0xde,0xad,0xbe,0xef, 
                                 0x00,0x00,0x00,0x03 };
#define CHANNEL_OFFSET 24


unsigned char button_push[56]={ 0x03,0x07,0xbe,0xef,0x00,0x00,0x00,0x14,
                                0x00,0x00,0x00,0x0b,0x00,0x00,0x08,0x23,
                                0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x20,
                                0x08,0x08,0x00,0x01,0x00,0x01,0x00,0x00,
                                0x00,0x00,0x00,0x0f,0x00,0x10,0x00,0x00,
                                0x00,0x00,0x00,0x02,0x00,0x00,0x00,0x00,
                                0x07,0x01,0x33,0x07,0x01,0x80,0x00,0x00 };
#define BUTTON_OFFSET 52
#define B_POW 0x01
#define B_POWON 0x02
#define B_POWOFF 0x03
#define B_SEL 0x2a

#define SEND(AAA) { /* printf("send %d bytes\n",sizeof(AAA)); */ \
                    tmp=sendto(gsock,AAA,sizeof(AAA),0,(struct sockaddr*)&gsi,\
                               sizeof(gsi)); \
                    assert(tmp==sizeof(AAA)); \
                  }

int gsock=-1;
int gbound=0;
struct sockaddr_in gsi;

#ifdef VSTUDIO
WSADATA global_wsa;
void winsock_done() {
  printf("\nWaiting a sec before cleaning up winsock...");
  Sleep(1000);
  printf(" done.\n");
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
  fprintf(stderr,"Usage: hava_channel <hava_dotform_ipaddr> <target_channel_number>\n");
  fprintf(stderr,"       hava_channel <hava_dotform_ipaddr> {POW,POWON,POWOFF,SEL}\n");
#ifdef VSTUDIO
  winsock_done();
#endif
  exit(1);
}

int wait_for_ack() {
  struct sockaddr_in so;
  unsigned char buf[4096];
  int i,
      ct,
      tmp,
      len,
      validated=0;
  unsigned short pkt;

  if(!gbound) {
    printf("Async mode.  No ack check\n");
    return 0;
  }
  for(ct=0;ct<8;ct++) {
    tmp=sizeof(so);
    len=recvfrom(gsock,buf,sizeof(buf),0,(struct sockaddr*)&so,&tmp);
    assert(tmp==sizeof(so));

    pkt=buf[2]<<8 | buf[3];
    if(len==4 && pkt==0x0beef) {
      validated=1;
      ct=8;
    }
    if(len<=0) {
      usleep(250000);
    } 
#ifdef DEBUG_MODE
    else 
    {
      printf("len=%d\n",len);
      for(i=0;i<len;i++) {
        if(!(i%16)) { printf("\n"); }
        printf("%02x ",buf[i]);
      }
    }
    printf("\n");
#endif
  }
  printf("Ack status: %d (1=success)\n",validated);
  return validated;
}

main(int argc, char *argv[]) {
  int i,tmp,butt=0;
  int channy;
#ifdef VSTUDIO
  winsock_init();
#endif
  if(argc!=3) { Usage(); }

  if(!strcmp(argv[2],"POW"))    { butt=1; button_push[BUTTON_OFFSET]=B_POW;    }
  if(!strcmp(argv[2],"POWON"))  { butt=1; button_push[BUTTON_OFFSET]=B_POWON;  }
  if(!strcmp(argv[2],"POWOFF")) { butt=1; button_push[BUTTON_OFFSET]=B_POWOFF; }
  if(!strcmp(argv[2],"SEL"))    { butt=1; button_push[BUTTON_OFFSET]=B_SEL;    }

  if(butt==0) {   
    channy=-1;
    channy=atoi(argv[2]);
    if(channy<=0) { Usage(); }
    if(channy>65535) { Usage(); }
    channel_set[CHANNEL_OFFSET]=0;
    channel_set[CHANNEL_OFFSET+1]=0;
    channel_set[CHANNEL_OFFSET+2]=((channy>>8)&0x0ff);
    channel_set[CHANNEL_OFFSET+3]=(channy&0xff);
  }
  
  gsock=socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
  assert(gsock>=0); 
  for(i=0;i<sizeof(gsi);i++) { ((char*)&gsi)[i]=0; }
  gsi.sin_family=AF_INET;
  gsi.sin_addr.s_addr=inet_addr(argv[1]);
  gsi.sin_port=htons(1778);
  if(bind(gsock,(struct sockaddr*)&gsi,sizeof(gsi)>=0)) 
  {
    gbound=1;
    fcntl(gsock,F_SETFL,O_NONBLOCK);
  } else {
    printf("Socket already bound; using async mode\n");
  }

  SEND(out1); 
  if(butt) {
    printf("Sending Init and button=%s request to %s\n",argv[2],argv[1]);
    SEND(button_push); 
  } else {
    printf("Sending Init and channel=%d(0x%x) to %s\n",channy,channy,argv[1]);
    SEND(channel_set); 
  }

  wait_for_ack();

  close(gsock);
#ifdef VSTUDIO
  winsock_done();
#endif
}
