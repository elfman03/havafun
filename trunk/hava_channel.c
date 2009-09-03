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

Usage(FILE *logfile) {
  fprintf(logfile,"Usage: hava_channel <hava_dotform_ipaddr> <target_channel_number>\n");
  fprintf(logfile,"       hava_channel <hava_dotform_ipaddr> <button_name>\n");
  fprintf(logfile,"       hava_channel showbuttons\n\n");
  fprintf(logfile,"NOTE: If you use '-' for the ipaddr, it will try to autodetect\n");
  fprintf(logfile,"      This mode is not recommended but can be useful for testing\n");
  fprintf(logfile,"      It will crash if you are using the hava player at the same time.\n\n");
  fprintf(logfile,"NOTE: Create file named \"hava_fun.logme\" allow logging to \"hava_fun.log\"\n");
#ifdef VSTUDIO
  Hava_finishup();
#endif
  exit(1);
}

void showbuttons(FILE *logfile) {
  int i;
  const char *p;
  fprintf(logfile,"Available buttons are:\n");
  for(i=0;i<256;i++) {
    p=Hava_button_ntoa(i);
    if(p) { 
      fprintf(logfile,"  %s\n",p);
    }
  }
  exit(1);
}

FILE *logfile=0;

#ifdef HAVA_NOWIN
int argc;
#define MAXARG 3
char *argv[MAXARG];
void build_argc_argv(LPSTR args) {
  int i,len;
  fprintf(logfile,"args=%s\n",args); 
  argc=1;
  argv[0]=0;
  len=strlen(args);
  if(len) { 
    argv[1]=malloc(len+1);
    argc++;
    for(i=0;i<=len;i++) {
      argv[1][i]=args[i];
      if(args[i]==' ') {
        argv[1][i]=0;
        if(argc<MAXARG) {
          argv[argc]=&argv[1][i+1];
        }
        argc++;
      }
    }
  }
  fprintf(logfile,"argc=%d arg2=%s arg3=%s\n",argc,argv[1],argv[2]);
}
int WinMain(HINSTANCE junk1, HINSTANCE junk2, LPSTR args, int junk3)
#else
main(int argc, char *argv[]) 
#endif
{
  Hava *hava;
  int channy;
  unsigned short butt=0;
  FILE *f;

  //
  // Create a file named "hava_fun.logme to give permission log to hava_fun.log
  //
  f=fopen("hava_fun.logme","r");
  if(f) {
    fclose(f);
    logfile=fopen("hava_fun.log","w");
  }
  if(!logfile) { logfile=stderr; }

#ifdef HAVA_NOWIN
  build_argc_argv(args);
#endif

  if(argc==2 && !strcmp(argv[1],"showbuttons")) { showbuttons(logfile); }

  Hava_startup(logfile);

  if(argc!=3) { Usage(logfile); }

  // check for button command
  //
  butt=Hava_button_aton(argv[2]);

  if(butt==0) {   
    //
    // not button...  check for channel command...
    //
    channy=-1;
    channy=atoi(argv[2]);
    if(channy<=0)    { Usage(logfile); }
    if(channy>65535) { Usage(logfile); }
  }
  
  hava=Hava_alloc(argv[1],logfile,0);
  //
  // Should work even unbound but cannot check ack status
  //
  // assert(Hava_isbound(hava));

  Hava_sendcmd(hava, HAVA_INIT, 0); 
  if(Hava_isbound(hava)) { Hava_loop(hava,HAVA_MAGIC_INIT,0); }

  if(butt) {
    fprintf(logfile,"Sending Init and button=%s request to %s\n",argv[2],argv[1]);
    Hava_sendcmd(hava, HAVA_BUTTON, butt); 
  } else {
    fprintf(logfile,"Sending Init and channel=%d(0x%x) to %s\n",channy,channy,argv[1]);
    Hava_sendcmd(hava, HAVA_CHANNEL, (unsigned short)channy); 
  }
  if(Hava_isbound(hava)) { Hava_loop(hava,HAVA_MAGIC_CHANBUTT,0); }

  Hava_close(hava);
  Hava_finishup();
  if(logfile==stderr) { fclose(logfile); }
}
