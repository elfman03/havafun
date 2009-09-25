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
  fprintf(logfile,"Usage: hava_channel {hava_ip} {input_name|id} {remote_code} [nobind] {command}+\n\n");
  fprintf(logfile,"       {hava_ip} = the IP address of your hava\n");
  fprintf(logfile,"                 Example: \"192.168.1.253\" for my hava\n");
  fprintf(logfile,"       {input_name|id} = Which hava video source (see hava showinputs)\n");
  fprintf(logfile,"                 Example: \"Component\" or 3 for Component input\n");
  fprintf(logfile,"       {remote_code} = The remote control code configured in Hava wizard\n");
  fprintf(logfile,"                 Example: \"S0775\" for Dish VIP211\n");
  fprintf(logfile,"                 Note: Use \"Learned\" if you learned your IR codes\n");
  fprintf(logfile,"       [nobind] = Optional to tell hava_channel to not even try to bind to port\n");
  fprintf(logfile,"       {command}+ = One or more commands : a CHANNEL, a BUTTON or a WAIT\n");
  fprintf(logfile,"                        A CHANNEL is a number. (1-65535)\n");
  fprintf(logfile,"                        A BUTTON is a name or 0xXX code from show[learned]buttons.\n");
  fprintf(logfile,"                        A WAIT is \"wX\". X is a number of seconds (1-9) to sleep\n");
  fprintf(logfile,"                 Example: \"PowerOn\" \"w3\" \"122\" \"w3\" \"Enter\"\n");
  fprintf(logfile,"\n   Full example: hava_channel 192.168.1.253 Component S0775 PowerOn w3 122\n\n");
  fprintf(logfile,"       hava_channel showbuttons\n");
  fprintf(logfile,"                 Shows list of available buttons\n");
  fprintf(logfile,"       hava_channel showlearnedbuttons\n");
  fprintf(logfile,"                 Shows list of available learned buttons\n");
  fprintf(logfile,"       hava_channel showinputs\n");
  fprintf(logfile,"                 Shows list of available inputs\n\n");
  fprintf(logfile,"NOTE: If you use '-' for the ipaddr, it will try to autodetect\n");
  fprintf(logfile,"      This mode is not recommended but can be useful for testing\n");
  fprintf(logfile,"      It will crash if you are using the hava player at the same time.\n\n");
  fprintf(logfile,"NOTE: Create file named \"hava_fun.logme\" allow logging to \"hava_fun.log\"\n");
#ifdef VSTUDIO
  Hava_finishup();
#endif
  exit(1);
}

void showbuttons(FILE *logfile,int learned) {
  int i;
  const char *p;
  if(!learned) {
    fprintf(logfile,"Available buttons are:\n");
  } else {
    fprintf(logfile,"Available learned buttons are:\n");
  }
  for(i=0;i<256;i++) {
    if(!learned) {
      p=Hava_button_ntoa(i);
    } else {
      p=Hava_button_learned_ntoa(i);
    }
    if(p) { 
      fprintf(logfile,"  0x%02x : %s\n",i,p);
    }
  }
  exit(1);
}

void showinputs(FILE *logfile) {
  int i;
  const char *p;
  fprintf(logfile,"Available buttons are:\n");
  for(i=0;i<4;i++) {
    p=Hava_input_ntoa(i);
    if(p) { 
      fprintf(logfile,"  %d : %s\n",i,p);
    }
  }
  exit(1);
}

FILE *logfile=0;

#ifdef HAVA_NOWIN
int argc;
#define MAXARG 16
char *argv[MAXARG];
void build_argc_argv(LPSTR args) {
  int i,len;
  fprintf(logfile,"args=%s\n",args); 
  argc=1;
  argv[0]="";
  len=strlen(args);
  if(len) { 
    argv[1]=malloc(len+1);
    argc++;
    for(i=0;i<=len;i++) {
      argv[1][i]=args[i];
      if(args[i]==' ') {
        argv[1][i]=0;
        //
        // skip multiple spaces
        //
        for(;args[i+1] && args[i+1]==' ';i++) { }
        if(argc<MAXARG) {
          argv[argc]=&argv[1][i+1];
        }
        argc++;
      }
    }
  }
  fprintf(logfile,"argc=%d\n",argc);
  for(i=0;i<argc;i++) {
    fprintf(logfile,"   argv[%d]=\"%s\"\n",i,argv[i]);
  }
}
int WinMain(HINSTANCE junk1, HINSTANCE junk2, LPSTR args, int junk3)
#else
main(int argc, char *argv[]) 
#endif
{
  Hava *hava;
  int binding=1,
      channy,
      input=-2,
      aindex,
      sleepy;
  unsigned short remote,
                 butt;
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

  if(argc==2 && !strcmp(argv[1],"showbuttons")) { showbuttons(logfile,0); }
  if(argc==2 && !strcmp(argv[1],"showlearnedbuttons")) { showbuttons(logfile,1); }
  if(argc==2 && !strcmp(argv[1],"showinputs")) { showinputs(logfile); }

  Hava_startup(logfile);

  if(argc<5) { Usage(logfile); }

  if(!strcmp(argv[4],"nobind")) { 
    if(argc<6) { Usage(logfile); }
    binding=0;
    aindex=5;
  } else {
    aindex=4;
  }

  // check for input name
  //
  input=Hava_input_aton(argv[2]);
  if(input<0) {
    fprintf(logfile,"Unknown input %s\n\n",argv[2]);
    Usage(logfile);
  }

  // check for remote code
  //
  remote=0;
  if(strcmp(argv[3],"Learned")) {
    remote=Hava_remote_aton(argv[3]);
    if(remote==0) {
      fprintf(logfile,"Unknown remote %s\n\n",argv[3]);
      Usage(logfile);
    }
  }

  hava=Hava_alloc(argv[1],binding,0,logfile,0);

  for(;aindex<argc;) {

    sleepy=0;
    if(argv[aindex][0]=='w') {
      sleepy=argv[aindex][1]-'0';
      if(sleepy<=0 || sleepy>9) {
        fprintf(logfile,"Bad wait at argv[%d] : \"%s\"\n",aindex,argv[aindex]);
        Usage(logfile);
      }
    }

    // check for button command
    //
    if(sleepy) {
      butt=0;
    } else {
      if(remote) {
        butt=Hava_button_aton(argv[aindex]);
      } else {
        butt=Hava_button_learned_aton(argv[aindex]);
      }
    }

    if(sleepy==0 && butt==0) {   
      //
      // not sleep or button...  check for channel command...
      //
      channy=-1;
      channy=atoi(argv[aindex]);
      if(channy<=0 || channy>65536)  { 
        fprintf(logfile,"Bad button/channel at argv[%d] : \"%s\"\n",aindex,argv[aindex]);
        Usage(logfile); 
      }
    }
  
    if(sleepy) {
      fprintf(logfile,"Waiting %d seconds before next command\n",sleepy);
      MSLEEP(sleepy*1000);
    } else {
      Hava_sendcmd(hava, HAVA_INIT, 0, 0); 
      fprintf(logfile,"Sending Init request to %s\n",argv[1]);
      if(Hava_isbound(hava)) { Hava_loop(hava,HAVA_MAGIC_INIT,0); }

      if(butt) {
        char *p=Hava_remote_ntoa(remote);
        if(!p) { Usage(logfile); }
        fprintf(logfile,"Sending button=%s(0x%02x) request to %s(%s/0x%04x)\n",
                   remote?Hava_button_ntoa(butt):Hava_button_learned_ntoa(butt),
                   butt,argv[1],p,remote);
        free(p);
        Hava_sendcmd(hava, HAVA_BUTTON, butt, remote); 
      } else {
        fprintf(logfile,"Sending channel=%d(0x%x) to %s(%s/0x%02x)\n",
                        channy,channy,argv[1],Hava_input_ntoa(input),input);
        Hava_sendcmd(hava, HAVA_CHANNEL, (unsigned short)channy, (unsigned short)input); 
      }
      if(Hava_isbound(hava)) { Hava_loop(hava,HAVA_MAGIC_CHANBUTT,0); }
    }
    aindex++;
    fprintf(logfile,"\n");
  }

  Hava_close(hava);
  Hava_finishup();
  if(logfile==stderr) { fclose(logfile); }
}
