/*
 * HavaFun code component
 * Code to capture the mpeg stream
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
#include <fcntl.h>
#include <assert.h>
#include "hava_util.h"

FILE *gof;

void my_callback(const char *buf,int len) {
  fwrite(buf,len,1,gof);
//  if(gof==stdout) { fflush(gof); }
  return;
}

Usage() {
  fprintf(stderr,"Usage: hava_record <hava_dotform_ipaddr> <duration_sec> <tgt_mpeg>\n\n");
  fprintf(stderr,"NOTE: If you use '-' for the <tgt_mpg>, it will send the output to stdout\n");
  fprintf(stderr,"      This is recommended for piping into mplayer/mencoder\n\n");
  fprintf(stderr,"NOTE: If you use '-' for the ipaddr, it will try to autodetect\n");
  fprintf(stderr,"      This is not recommended but can be useful for testing\n");
  Hava_finishup();
  exit(1);
}

main(int argc, char *argv[]) {
  Hava *hava;
  int tmp,tis=-1;
  struct timeval tv;

  Hava_startup(stderr);

  if(argc!=4) { Usage(); }

  tis=atoi(argv[2]);
  if(tis<0) { Usage(); }

  // open output file (pipeout with -)
  //
  if(argv[3][0]=='-' && argv[3][1]==0) {
#ifdef VSTUDIO
    tmp=fileno(stdout);
    if(!isatty(tmp))
    _setmode(tmp,_O_BINARY);
#endif
    gof=stdout;
  } else {
    gof=fopen(argv[3],"wb");
  }
  assert(gof);

  hava=Hava_alloc(argv[1],1,1,stderr,0);
  assert(Hava_isbound(hava));
  Hava_set_videocb(hava, &my_callback);

  // zero second tis means go forever (no set of endtime)
  //
  if(tis) { 
    Hava_set_videoendtime(hava,Hava_getnow()+tis);
  }

  fprintf(stderr,"Sending Init and video start request to %s\n",argv[1]);
  Hava_sendcmd(hava, HAVA_INIT, 0, 0); 
  Hava_loop(hava,HAVA_MAGIC_INIT,0); 

  Hava_sendcmd(hava, HAVA_START_VIDEO, 0, 0); 
  Hava_loop(hava,HAVA_MAGIC_RECORD,0);

  fclose(gof);

  Hava_close(hava);
  Hava_finishup();
}
