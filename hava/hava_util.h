/*
 * HavaFun code component
 * Header file describing the API that talks to the Hava
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
#ifndef HAVA_UTIL_H_
#define HAVA_UTIL_H_ 1

#ifdef _MSC_VER
#define VSTUDIO
#endif

#ifdef VSTUDIO
#define MSLEEP(a) Sleep(a)
#define CLOSE(a) closesocket(a)
#include <winsock2.h>
#include <windows.h>
#include <winbase.h>
#else
#define MSLEEP(a) usleep(a*1000)
#define CLOSE(a) close(a)
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>
#endif

typedef struct Hava {
  int                sock,      // the socket
                     bound;     // is the socket bound?
  struct sockaddr_in si;        // Sockaddr with target Hava IP filled in
  FILE           *logfile;      // File to log messages to (stderr by default)

  unsigned char  *buf,          // the input buffer from the hava
                 *mypkt_cont,   // My moddable copy of a continuation packet
                 *mypkt_butt,   // My moddable copy of a button packet
                 *mypkt_buttl,  // My moddable copy of a LEARNED button packet
                 *mypkt_chan;   // My moddable copy of a channel packet
 
  unsigned long  vid_starttime, // millisecond when I started video
                 vid_endtime,   // millisecond when I should stop recording
                 vid_stattime,  // millisecond when I should print stats
                 vid_minbytes,  // video bytes during in last reporting interval
                 vid_totbytes;  // video bytes during session
  int            vid_header,    // User want video header
                 vid_starting,  // Is video just starting?
                 vid_ooo;       // Fixup out of order video
  void           *bonus_val;    // A bonus value that the client can set/get
  unsigned short vid_seq;       // video sequence number
  unsigned char  vid_quality;   // Video quality (0x00 or 0x10-0x50)

  // hava_util calls this w/pkts
  void (*vid_callback)(struct Hava *hava, unsigned long now,
                       const unsigned char *buf, int len);  
} Hava;

// input  --  Target Hava device IP or "-"
// input  --  Should I try to bind?
// input  --  Should I use blocking socket semantics
// input  --  Try to make the socket nonblocking
// input  --  Logfile to use (typically stderr)
// input  --  Request to be verbose (prints lots of stuff)
// output --  A new Hava connection structure
//
extern Hava *Hava_alloc(const char *havaip, int binding, int blocking,
                        FILE *logfile, int verbose);

//
// use Hava_set_bonus() to set bonus value
//
extern void Hava_set_bonus(Hava *hava, void *val);

//
// use Hava_get_bonus() to get bonus value
//
extern void *Hava_get_bonus(Hava *hava);

//
// use Hava_isbound() to see if it really bound to the Hava local port
//
extern int Hava_isbound(Hava *hava);

//
// Set to have an MPEG header at start of videooutput
//
void Hava_set_videoheader(Hava *hava, int val);

//
// Set the requested video quality (0x00 or value between 0x10-0x50).  
// Effectively, the number should be between 0x10-0x30.  At about 0x30
// the max rate of about 8Mbps should be achieved.  Back off if get lossy video
// 0x00 will use the numbers that Hava requests 
//
extern void Hava_set_videoquality(Hava *hava, unsigned char q);

//
// get the current user requested video quality
//
extern unsigned char Hava_get_videoquality(Hava *hava);

//
// Define millisecond when I should stop recording
//
extern void Hava_set_videoendtime(Hava *hava, unsigned long et);

//
// get millisecond when I should stop recording
//
extern unsigned long Hava_get_videoendtime(Hava *hava);

//
// Define function pointer to app function that will eat video data (or null)
//
extern void Hava_set_videocb(Hava *hava, 
                             void (*vcb)(Hava *hava, unsigned long now,
                                         const unsigned char *buf,int len));

//
// Returns the current time in milliseconds (for computing a record endtime)
//
extern unsigned long Hava_getnow();

//
// Startup a hava app...  call it first (for winsock initialization)
//
extern void  Hava_startup(FILE *logfile);

//
// Finishup a hava app...  call it last (for winsock cleanup)
//
extern void  Hava_finishup();

//
// Close the open sockets and free stuff
//
extern void Hava_close(Hava *hava);

#define HAVA_MAGIC_RECORD   0x0000
#define HAVA_MAGIC_CHANBUTT 0xface
#define HAVA_MAGIC_INIT     0xb2f8
#define HAVA_MAGIC_INFO     0x0148
//
// Loop for a while
// Use HAVA_MAGIC_RECORD for video capture
// Use HAVA_MAGIC_CHANBUTT for channel changes and button presses
//
extern int Hava_loop(Hava *hava, unsigned short magic, int verbose);

// Convert a LEARNED button name to a LEARNED button number
// Zero equals not found
//
unsigned char Hava_button_learned_aton(const char *name);

// Use this to get the name of a LEARNED button number
// null string equals no match
//
const char *Hava_button_learned_ntoa(unsigned char bno);

// Convert a button name to a button number
// Zero equals not found
//
unsigned char Hava_button_aton(const char *name);

// Use this to get the name of a button number
// null string equals no match
//
const char *Hava_button_ntoa(unsigned char bno);

// Convert a remote code name to a remote hex
// Zero equals not found
//
unsigned short Hava_remote_aton(const char *name);

// Use this to get the code name of a remote hex code
// Returns "<UNSUPPORTED>" for no match
// NOTE: RETURNED POINTER MUST BE FREED!!!
//
char *Hava_remote_ntoa(unsigned short remote);

// Use this to get the number of an input
// null string equals no match
//
int Hava_input_aton(const char *name);

// Use this to get the name of an input
// null string equals no match
//
const char *Hava_input_ntoa(unsigned char ino);


// Hava commands -- use with Hava_sendcmd()
//
#define HAVA_INIT           0
#define HAVA_START_VIDEO    1
#define HAVA_CONT_VIDEO     2
#define HAVA_CHANNEL        3
#define HAVA_BUTTON         4

//
// Send a command! (INIT, START_VIDEO, CONT_VIDEO, CHANNEL, BUTTON)
//
// CONT_VIDEO ... internal use only
// CHANNEL 
//   -- eA is channel number (0-65535) 
//   -- eB is input number (0-3)
// BUTTON 
//   -- eA is button id (e.g., from Hava_button_aton())
//   -- eB is remote control code (e.g., S0775)
//
// BUTTON_LEARNED
//   -- eA is button id (e.g., from Hava_button_learned_aton())
//
extern void Hava_sendcmd(Hava *hava, int cmd, 
                                     unsigned short eA, unsigned short Eb);

#endif
