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
#include "vulk_util.h"

FILE *gof;

void my_callback(Vulk *vulk,unsigned long now,const unsigned char *buf,int len) {
  int b;
  b=fwrite(buf,len,1,gof);

  if(b!=1) {
    fprintf(stderr,"vulk_record write failed... finalizing capture shortly...\n");
    Vulk_set_videoendtime(vulk,now);
  }
  return;
}

Usage() {
  fprintf(stderr,"Usage: vulk_record {vulk_ipaddr} {quality} {duration_sec} {tgt_mpeg}\n\n");
  fprintf(stderr,"      {vulk_ipaddr} is expressed in dot form (e.g., 192.168.1.253)\n");
  fprintf(stderr,"      {quality}     is expressed in hex as 0x00 or between '0x01'-'0x50'\n");
  fprintf(stderr,"                    Numbers >=0x30 seem to achieve the max 8Mbps thruput\n");
  fprintf(stderr,"                    Use small numbers (~0x01-0x15) if you experience video loss\n");
  fprintf(stderr,"                    Recommend >=0x10 (0x01-0x03 don't work for me at all)\n");
  fprintf(stderr,"                    Use 0x00 for broken auto-heuristic (lowish quality: ~0x0a)\n");
  fprintf(stderr,"      {duration}    is expressed in seconds.  Zero means go forever\n");
  fprintf(stderr,"      {tgt_mpeg}    is a file name that will be written\n\n");
  fprintf(stderr,"NOTE: If you use '-' for the <tgt_mpg>, it will send the output to stdout\n");
  fprintf(stderr,"      This is recommended for piping into mplayer/mencoder\n\n");
  fprintf(stderr,"NOTE: If you use '-' for the ipaddr, it will try to autodetect\n");
  fprintf(stderr,"      This is not recommended but can be useful for testing\n");
  Vulk_finishup();
  exit(1);
}

main(int argc, char *argv[]) {
  Vulk *vulk;
  int tmp,tis=-1;
  unsigned char quality=0;
  struct timeval tv;

  Vulk_startup(stderr);

  if(argc!=5) { Usage(); }

  //
  // Handle the quality parameter
  //
  if(argv[2][0]!='0' || argv[2][1]!='x') { Usage(); }
  sscanf(argv[2],"0x%x",&tmp);
  if(tmp && (tmp<0x00 || tmp>0x50)) { Usage(); }
  quality=(unsigned char)tmp;

  tis=atoi(argv[3]);
  if(tis<0) { Usage(); }

  // open output file (pipeout with -)
  //
  if(argv[4][0]=='-' && argv[4][1]==0) {
#ifdef VSTUDIO
    tmp=fileno(stdout);
    if(!isatty(tmp))
    _setmode(tmp,_O_BINARY);
#endif
    gof=stdout;
  } else {
    gof=fopen(argv[4],"wb");
  }
  assert(gof);

  vulk=Vulk_alloc(argv[1],1,1,stderr,0);
  assert(Vulk_isbound(vulk));
  Vulk_set_videocb(vulk, &my_callback);
  Vulk_set_videoquality(vulk, quality);

  // zero second tis means go forever (no set of endtime)
  //
  if(tis) { 
    Vulk_set_videoendtime(vulk,Vulk_getnow()+tis*1000);
  }

  for(tmp=0;tmp!=1;) {
    fprintf(stderr,"Sending Init request to %s\n",argv[1]);
    Vulk_sendcmd(vulk, HAVA_INIT, 0, 0); 
    tmp=Vulk_loop(vulk,HAVA_MAGIC_INIT,0); 
    if(tmp!=1) {
      fprintf(stderr,"BAD RESPONSE TO INIT REQUEST... RETRYING\n");
    }
  }

  fprintf(stderr,"Sending video start request to %s\n",argv[1]);
  Vulk_sendcmd(vulk, HAVA_START_VIDEO, 0, 0); 
  Vulk_loop(vulk,HAVA_MAGIC_RECORD,0);

  fclose(gof);

  Vulk_close(vulk);
  Vulk_finishup();
}
