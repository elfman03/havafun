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

Usage() {
  fprintf(stderr,"Usage: hava_channel <hava_dotform_ipaddr> <target_channel_number>\n");
  fprintf(stderr,"       hava_channel <hava_dotform_ipaddr> {POW,POWON,POWOFF,SEL}\n");
#ifdef VSTUDIO
  winsock_done();
#endif
  exit(1);
}

main(int argc, char *argv[]) {
  Hava *hava;
  int channy;
  unsigned short butt=0;

  if(argc!=3) { Usage(); }

  // check for button command
  //
  if(!strcmp(argv[2],"POW"))    { butt=HAVA_BUTT_POW;    }
  if(!strcmp(argv[2],"POWON"))  { butt=HAVA_BUTT_POWON;  }
  if(!strcmp(argv[2],"POWOFF")) { butt=HAVA_BUTT_POWOFF; }
  if(!strcmp(argv[2],"SEL"))    { butt=HAVA_BUTT_SEL;    }

  if(butt==0) {   
    //
    // not button...  check for channel command...
    //
    channy=-1;
    channy=atoi(argv[2]);
    if(channy<=0)    { Usage(); }
    if(channy>65535) { Usage(); }
  }
  
  hava=Hava_alloc(argv[1]);
  //
  // Should work even unbound but cannot check ack status
  //
  // assert(Hava_isbound(hava));

  Hava_sendcmd(hava, HAVA_INIT, 0); 
  if(butt) {
    printf("Sending Init and button=%s request to %s\n",argv[2],argv[1]);
    Hava_sendcmd(hava, HAVA_BUTTON, butt); 
  } else {
    printf("Sending Init and channel=%d(0x%x) to %s\n",channy,channy,argv[1]);
    Hava_sendcmd(hava, HAVA_CHANNEL, (unsigned short)channy); 
  }

  if(Hava_isbound(hava)) {
    Hava_loop(hava,HAVA_MAGIC_CHANBUTT);
  }

  Hava_close(hava);
  Hava_finishup();
}
