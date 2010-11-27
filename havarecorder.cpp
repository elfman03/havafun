// -*- Mode: c++ -*-
/*
 * HAVARecorder * HavaFun code component
 * C++ file for a MythTV recorder class that is a client of havafun
 *          Hope to donate to MythTV at some point
 *          Used IPTVRecorder.cpp as a template early on
 *
 * Copyright (C) 2010 Chris Elford
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
 */

#include <unistd.h>
extern "C" {
#include <stdio.h>
#include "hava_util.h"
}

// MythTV headers
#include "programinfo.h"
#include "mythverbose.h"
#include "RingBuffer.h"
#include "havarecorder.h"

#define LOC QString("HavaRec: ")
#define LOC_ERR QString("HavaRec, Error: ")

static void my_callback(Hava *h,unsigned long now,const unsigned char *buf, int len) {
  HavaRecorder *r=(HavaRecorder *)Hava_get_bonus(h);
  r->Push(now,buf,len);
//  write(r->GetPipe(1),buf,len);
}

// ============================================================================
// HavaRecorder : Processes data from Monsoon Hava UDP and writes it to RingBuffer
// ============================================================================

HavaRecorder::HavaRecorder(TVRec *rec) :
    DTVRecorder(rec)
{
    VERBOSE(VB_IMPORTANT, LOC + "HavaRecorder() -- start");
    hava=NULL;
    ptr_base=ptr_top=ptr_empty_end=ptr_full_end=ptr_push_pt=ptr_pop_pt=0;
    aspect=width=height=0;
    positionMapType = MARK_GOP_BYFRAME;
    hava_thread=NULL;
    Hava_startup(stderr);
    VERBOSE(VB_IMPORTANT, LOC + "HavaRecorder() -- finish");
}

HavaRecorder::~HavaRecorder()
{
    StopRecording();
    Hava_finishup();
}

bool HavaRecorder::Open(void)
{
    VERBOSE(VB_IMPORTANT, LOC + "Open() -- begin");
    _error = false;

    // should not happen but just in case
    if(hava) { 
      VERBOSE(VB_IMPORTANT, LOC + "Open() -- close unexpected preexisting hava");
      Hava_close(hava); 
      hava=0;
    }

    hava=Hava_alloc("192.168.1.253",1,1,stderr,0);
VERBOSE(VB_IMPORTANT, LOC + "Open() -- a");
    if(Hava_isbound(hava)) {
VERBOSE(VB_IMPORTANT, LOC + "Open() -- b");
      Hava_set_bonus(hava, (void *)this);
      Hava_set_videocb(hava, &my_callback);
      Hava_set_videoquality(hava, 0x030);
      Hava_sendcmd(hava, HAVA_INIT, 0, 0);
VERBOSE(VB_IMPORTANT, LOC + "Open() -- c");
      if(Hava_loop(hava, HAVA_MAGIC_INIT,0)!=1) {
        VERBOSE(VB_IMPORTANT, LOC + "Open() -- bad Hava init response");
        _error = true;
      }
VERBOSE(VB_IMPORTANT, LOC + "Open() -- d");
    } else {
      VERBOSE(VB_IMPORTANT, LOC + "Open() -- Cant bind socket");
      _error = true;
    }

    if(_error) {
      if(hava) { Hava_close(hava); }
      hava=0;
    }

    offset=0;

    VERBOSE(VB_IMPORTANT, LOC + "Open() -- end err(" +(_error ? "true" : "false")+ ")");
    return !_error;
}

void HavaRecorder::Close(void)
{
    VERBOSE(VB_IMPORTANT, LOC + "Close() -- begin");
    Hava_close(hava);
    hava=0;
    VERBOSE(VB_IMPORTANT, LOC + "Close() -- end");
}

void HavaRecorder::Pause(bool clear)
{
    VERBOSE(VB_IMPORTANT, LOC + "Pause() -- begin");
    DTVRecorder::Pause(clear);

    if(hava) { Hava_set_videoendtime(hava, Hava_getnow()); }

    VERBOSE(VB_IMPORTANT, LOC + "Pause() -- end");
}

//void HavaRecorder::Unpause(void)
//{
//    VERBOSE(VB_RECORD, LOC + "Unpause() -- begin");
//
//    DTVRecorder::Unpause();
//
//    VERBOSE(VB_RECORD, LOC + "Unpause() -- end");
//}

static void *hava_loop(void *p) {
VERBOSE(VB_IMPORTANT, LOC + "hava_loop() -- begin");
  Hava *hava=(Hava*)p;
  Hava_sendcmd(hava, HAVA_START_VIDEO, 0, 0);
  Hava_loop(hava, HAVA_MAGIC_RECORD, 0);
VERBOSE(VB_IMPORTANT, LOC + "hava_loop() -- end");
  pthread_exit(NULL);
}

int HavaRecorder::GetPipe(int which) {
  if(which==1) { return pipes[1]; }
  if(which==0) { return pipes[0]; }
  return -1;
}

void HavaRecorder::StartRecording(void)
{
    int status;
    VERBOSE(VB_IMPORTANT, LOC + "StartRecording() -- begin");
    if (!Open())
    {
        _error = true;
        return;
    }

    // Start up...
    _error = false;
    _recording = true;
    _request_recording = true;
    _frames_seen_count=0;
    _frames_written_count=0;
    key_base=0xffffffff;
    if(!ptr_base) {
      ptr_base=(unsigned char *)malloc(1024*1024*4);
      ptr_top=ptr_base+1024*1024*4;
      ptr_empty_end=ptr_top;
      ptr_full_end=ptr_push_pt=ptr_pop_pt=ptr_base;
    }

    status=pipe(pipes);
    if(status) {
      VERBOSE(VB_IMPORTANT, LOC + "StartRecording() -- cant start pipe.  ret=" + status);
      _error=true; 
    }
    if(!_error) {
      status=pthread_create(&hava_thread,NULL,&hava_loop,hava);
      if(status) {
        VERBOSE(VB_IMPORTANT, LOC + "StartRecording() -- cant start child thread.  ret=" + status);
        close(pipes[0]); 
        close(pipes[1]);
        hava_thread=NULL;
        _error=true; 
      }
    } 
    if(!_error) {
      while (_request_recording) {
          Pop();
          _frames_written_count=_frames_seen_count;
      }
//        unsigned char buf[10240];
//        status=read(pipes[0],buf,10240);
//        if(status) {
//          if(DoKeyFrame(buf,status)) {
//            CheckForRingBufferSwitch();
//          }
//          ringBuffer->Write(buf,status);
//          _frames_written_count=_frames_seen_count;
//        } else {
//          _error=1;
//        }
VERBOSE(VB_IMPORTANT, LOC + "StartRecording() -- preclosepipe0");
      close(pipes[0]); 
VERBOSE(VB_IMPORTANT, LOC + "StartRecording() -- preclosepipe1");
      close(pipes[1]);
VERBOSE(VB_IMPORTANT, LOC + "StartRecording() -- prejoin");
      status=pthread_join(hava_thread,NULL);
VERBOSE(VB_IMPORTANT, LOC + "StartRecording() -- postjoin");
      if(status) {
        VERBOSE(VB_IMPORTANT, LOC + "StartRecording() -- cant join child thread.  ret=" + status);
      }
      hava_thread=NULL;
      if(ptr_base) {
        free(ptr_base);
        ptr_base=ptr_top=ptr_empty_end=ptr_full_end=ptr_push_pt=ptr_pop_pt=0;
      }
    }

VERBOSE(VB_IMPORTANT, LOC + "StartRecording() -- prefinishup");
    // Finish up...
    FinishRecording();
VERBOSE(VB_IMPORTANT, LOC + "StartRecording() -- precloseclose");
    Close();

    VERBOSE(VB_IMPORTANT, LOC + "StartRecording() -- end");
    _recording = false;
}

void HavaRecorder::StopRecording(void)
{
    VERBOSE(VB_IMPORTANT, LOC + "StopRecording() -- begin");
    Pause();

    _request_recording = false;

    VERBOSE(VB_IMPORTANT, LOC + "StopRecording() -- end");
}


// ===================================================
// Push : feed data from Hava flow to staging buffer
// ===================================================
void HavaRecorder::Push(unsigned long now, const unsigned char *buf, unsigned int len) {
  unsigned char *pee;
  unsigned int rem;

  // PRODUCER CODE:  Runs in the thread that runs Hava_loop started by MythTV recorder
  //
  // We need to minimize the time in the thread reading off of the socket from hava.  This 
  // is because Hava writes data via UDP datagrams that are lossy.  We want to minimize our 
  // loss so we do as little as possible in that thread.  I previously tried to do frame
  // detection or mythtv ringbuffer writes without a separate thread but was getting 
  // significant packet drops.  Hence, I decided it was better to take the extra buffer
  // copy.

  pee=ptr_empty_end;   // save pee off so it does not change on us (we read, consumer writes)
fprintf(stderr,"Push ptr_base=%d ptr_top=%d ptr_push_pt=%d pee=%d\n",
                ptr_base-ptr_base,ptr_top-ptr_base,ptr_push_pt-ptr_base,pee-ptr_base);
  if(!pee) { 
    VERBOSE(VB_IMPORTANT, LOC_ERR + "Push() No myring!  Dropping " + QString::number(len) + " bytes!");
    return; 
  }

  if(pee<=ptr_push_pt) { 
    // we are writing upward and pee is below us (or at us)
    // This means smooth sailing to top of ringbuffer and then a bit more free at bottom 
    rem=ptr_top-ptr_push_pt;
    VERBOSE(VB_IMPORTANT, LOC + "Push(a) " + QString::number(len) + " " + QString::number(rem));
    if(rem > len) {
      VERBOSE(VB_IMPORTANT, LOC + "Push(a1) " + QString::number(len) + " " + QString::number(rem));
      // Whole thing fits.  Push it in
      memcpy(ptr_push_pt,buf,len);
      ptr_push_pt+=len;
      len=0;
      ptr_full_end=ptr_push_pt;  // update consumer end pointer
    } else {
      VERBOSE(VB_IMPORTANT, LOC + "Push(a2) " + QString::number(len) + " " + QString::number(rem));
      // Push part that fits and adjust pointers to continue
      memcpy(ptr_push_pt,buf,rem); 
      buf+=rem;
      len-=rem;
      ptr_push_pt=ptr_base; 
      ptr_full_end=ptr_top;      // update consumer end point
    }
  }
  if(len && pee>ptr_push_pt) { 
    // we are writing upward and pee is above us.  We cant write past the pee 
    rem=pee-ptr_push_pt;
    VERBOSE(VB_IMPORTANT, LOC + "Push(b) " + QString::number(len) + " " + QString::number(rem));
    if(rem >= len) {
      VERBOSE(VB_IMPORTANT, LOC + "Push(b1) " + QString::number(len) + " " + QString::number(rem));
      // Whole thing fits.  Push it in
      memcpy(ptr_push_pt,buf,len);
      ptr_push_pt+=len;
      len=0;
      ptr_full_end=ptr_push_pt;  // update consumer end point
    }
  } 
  if(len) {
    // NO ROOM AT THE INN AIEEEEEE
    VERBOSE(VB_IMPORTANT, LOC_ERR + "Push() No room in myring.  Dropping " + QString::number(len) + " bytes!");
  }
}

// ===================================================
// Pop : feed data from staging buffer into MythTV RingBuffer
// ===================================================
void HavaRecorder::Pop() {
  unsigned char *pfe;
  unsigned int rem;

  // CONSUMER CODE:  Runs in the MythTV Recorder Thread
  //
  // We need to minimize the time in the thread reading off of the socket from hava.  This 
  // is because Hava writes data via UDP datagrams that are lossy.  We want to minimize our 
  // loss so we do as little as possible in that thread.  I previously tried to do frame
  // detection or mythtv ringbuffer writes without a separate thread but was getting 
  // significant packet drops.  Hence, I decided it was better to take the extra buffer
  // copy.

  pfe=ptr_full_end;   // save pfe off so it does not change on us (we read, producer writes)
  if(pfe==ptr_pop_pt) {  
    // Nothing is there.  Wait a bit...
//    VERBOSE(VB_IMPORTANT, LOC + "Pop(a)");
    usleep(2000);
    return;
  }
fprintf(stderr,"Pop ptr_base=%d ptr_top=%d ptr_pop_pt=%d pfe=%d\n",
                ptr_base-ptr_base,ptr_top-ptr_base,ptr_pop_pt-ptr_base,pfe-ptr_base);

  if(pfe>ptr_pop_pt) { 
    // we are reading upward and pfe is above us.  We cant read past the pfe 
    rem=pfe-ptr_pop_pt;
    VERBOSE(VB_IMPORTANT, LOC + "Pop(b) " + QString::number(rem));
    if(rem>127) { DoKeyFrame(ptr_pop_pt,rem); }
    ringBuffer->Write(ptr_pop_pt,rem);
    ptr_pop_pt=pfe;
  }
  if(pfe<ptr_pop_pt) { 
    // we are reading upward and pfe is below us
    // This means smooth sailing to top of ringbuffer and then a bit more full at bottom 
    // We will catch the stuff at the bottom next time around (maybe with even more content)
    rem=ptr_top-ptr_pop_pt;
    VERBOSE(VB_IMPORTANT, LOC + "Pop(c) " + QString::number(rem));
// NOTE MIGHT WANT TO PULL A FEW BYTES FROM ptr_base FOR KEYFRAME DETECTION ACCURACY!!!
    if(rem>127) { DoKeyFrame(ptr_pop_pt,rem); }
    ringBuffer->Write(ptr_pop_pt,rem);
    ptr_pop_pt+=rem;
  }
  // Update so producer knows how far free space has come
  ptr_empty_end=ptr_pop_pt;
  if(ptr_pop_pt==ptr_top) { ptr_pop_pt=ptr_base; }
}

bool HavaRecorder::DoKeyFrame(const unsigned char *buf, unsigned int len)
{
  bool ret=false, oops;
  unsigned int i;
  unsigned long hr,min,sec,fr,x;
  // Loop thru buffer
  for(i=0;i<(len-8);i++) {
    // find what looks like a GOP header
    if(buf[i]==0x00 && buf[i+1]==0x00 && buf[i+2]==0x01 && buf[i+3]==0xb8) {

      // http://dvd.sourceforge.net/dvdinfo/mpeghdrs.html
      //  00 00 01 b8    xx          xx        xx        xx
      // xx xx xx xx: b|bbbbb|bb bbbb|1|bbb bbb|bbbbb b|bb00000
      //               |hour |minute |1|second | frame |
      // Extract Hour, minute, second, frame number
      hr =((buf[i+4]>>2))&0x1f;
      min=((buf[i+4]&0x03)<<4)|(buf[i+5]>>4);
      sec=((buf[i+5]&0x07)<<3)|(buf[i+6]>>5);
      fr =((buf[i+6]&0x1f)<<1)|(buf[i+7]>>7);

      // compute frame number using integer math
      x=(((((hr*60)+min)*60+sec)*2997))/100+fr;

      // If we have never done it, set the base of the keyframe
      // DANGER!  IF THIS ONE IS BOGUS, WE ARE HOSED...
      if(key_base==0xffffffff) { 
        key_base=x;
        key_bogus=2;
      }

      // Do Bogus GOP detection
      // The purpose of this is in case 000001b8 occurs in middle of something
      if(key_bogus==2) { key_last_s=sec; key_last_m=min; key_last_h=hr; } 
      oops=false;

      if(sec>key_last_s) {
        if(sec && sec!=key_last_s+1) { oops=true; } // assume bogus detection (>1sec passed)
      } else {
        if(sec<key_last_s) {
          if(min && min!=key_last_m+1) { oops=true; } // assume bogus detection (negative time)
        }
      }
      if(min>key_last_m) {
        if(min && min!=key_last_m+1) { oops=true; } // assume bogus detection (>1min passed)
      } else {
        if(min<key_last_m) {
          if(hr && hr!=key_last_h+1) { oops=true; } // assume bogus detection (negative time)
        }
      }
      if(oops) {
        key_bogus++;
      } else {
        // REAL GOP...  DUMP A RECORD TIME
        //
        key_bogus=0;
        key_last_s=sec;
        key_last_m=min;
        key_last_h=hr;
        // HAVA runs counter forever so we might hit a wraparound.  Handle it
        if(x<key_base) { x+=(31*60*60*2997/100); } 

        _frames_seen_count=x-key_base;

        positionMapLock.lock();
        if (!positionMap.contains(_frames_seen_count))
        {
            long long startpos = ringBuffer->GetWritePosition();
            positionMapDelta[_frames_seen_count] = startpos+i;
            positionMap[_frames_seen_count]      = startpos+i;
        }
        positionMapLock.unlock();
        ret=true;
        i=len;
      } 
    }
  }
  if(ret) { CheckForRingBufferSwitch(); }
  return ret;
}

/* vim: set expandtab tabstop=4 shiftwidth=4: */
