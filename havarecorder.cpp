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
#include "mythlogging.h"
#include "ringbuffer.h"
#include "havarecorder.h"

//#define DEBUGGING_MODE 1

#define LOC QString("HavaRec: ")
#define LOC_ERR QString("HavaRec, Error: ")

static void my_callback(Hava *h,unsigned long now,const unsigned char *buf, int len) {
  HavaRecorder *r=(HavaRecorder *)Hava_get_bonus(h);
  r->Push(now,buf,len);
}

// ============================================================================
// HavaRecorder : Processes data from Monsoon Hava UDP and writes it to RingBuffer
// ============================================================================

HavaRecorder::HavaRecorder(TVRec *rec, HavaChannel *chan) :
    DTVRecorder(rec),
    channel(chan)
{
    hava=NULL;
    ptr_base=ptr_top=ptr_empty_end=ptr_full_end=ptr_push_pt=ptr_pop_pt=0;
    aspect=width=height=0;
    positionMapType = MARK_GOP_BYFRAME;
    hava_thread=NULL;
    Hava_startup(stderr);
}

HavaRecorder::~HavaRecorder()
{
    StopRecording();
    Hava_finishup();
}

bool HavaRecorder::Open(void)
{
    _error = QString();
    bool err=false;

    // should not happen but just in case
    if(hava) { 
      LOG(VB_RECORD, LOG_ERR, LOC + "Open() -- close unexpected preexisting hava");
      Hava_close(hava); 
      hava=0;
    }

    hava=Hava_alloc("192.168.1.253",1,1,stderr,0);
    if(Hava_isbound(hava)) {
      Hava_set_bonus(hava, (void *)this);
      Hava_set_videocb(hava, &my_callback);
      Hava_set_videoquality(hava, 0x030);
      Hava_sendcmd(hava, HAVA_INIT, 0, 0);
      if(Hava_loop(hava, HAVA_MAGIC_INIT,0)!=1) {
        LOG(VB_RECORD, LOG_ERR, LOC + "Open() -- bad Hava init response");
        _error = "Open() -- bad Hava init response";
        err=true;
      }
    } else {
      LOG(VB_RECORD, LOG_ERR, LOC + "Open() -- Cant bind socket");
      _error = "Open() -- Cant bind socket";
      err=true;
    }

    if(err) {
      if(hava) { Hava_close(hava); }
      hava=0;
    }

    offset=0;

    return !err;
}

void HavaRecorder::Close(void)
{
    Hava_close(hava);
    hava=0;
}

void HavaRecorder::Pause(bool clear)
{
    DTVRecorder::Pause(clear);

    if(hava) { Hava_set_videoendtime(hava, Hava_getnow()); }
}

//void HavaRecorder::Unpause(void)
//{
//    LOG(VB_RECORD, LOG_INFO, LOC + "Unpause() -- begin");
//
//    DTVRecorder::Unpause();
//
//    LOG(VB_RECORD, LOG_INFO, LOC + "Unpause() -- end");
//}

static void *hava_loop(void *p) {
  Hava *hava=(Hava*)p;
  Hava_sendcmd(hava, HAVA_START_VIDEO, 0, 0);
  Hava_loop(hava, HAVA_MAGIC_RECORD, 0);
  pthread_exit(NULL);
}

void HavaRecorder::run(void)
{
    int status;
    bool err=false;
    if (!Open())
    {
        _error = "HAVA - starting recording but not open!";
        err=true;
        return;
    }

    // Start up...
    _error = QString();
    recording = true;
    request_recording = true;
    _frames_seen_count=0;
    _frames_written_count=0;
    key_base=0xffffffff;
    if(!ptr_base) {
      ptr_base=(unsigned char *)malloc(1024*1024*4);
      ptr_top=ptr_base+1024*1024*4;
      ptr_empty_end=ptr_top;
      ptr_full_end=ptr_push_pt=ptr_pop_pt=ptr_base;
    }

    if(!err) {
      status=pthread_create(&hava_thread,NULL,&hava_loop,hava);
      if(status) {
        LOG(VB_RECORD, LOG_ERR, LOC + "StartRecording() -- cant start child thread.  ret=" + status);
        hava_thread=NULL;
        _error="HAVA StartRecording() -- cant start child thread!"; 
        err=true;
      }
    } 
    if(!err) {
      while (IsRecordingRequested() && !IsErrored()) {
          Pop();
          _frames_written_count=_frames_seen_count;
      }
      status=pthread_join(hava_thread,NULL);
      if(status) {
        LOG(VB_RECORD, LOG_ERR, LOC + "StartRecording() -- cant join child thread.  ret=" + status);
      }
      hava_thread=NULL;
      if(ptr_base) {
        free(ptr_base);
        ptr_base=ptr_top=ptr_empty_end=ptr_full_end=ptr_push_pt=ptr_pop_pt=0;
      }
    }

    // Finish up...
    FinishRecording();
    Close();

    recording = false;
}

void HavaRecorder::StopRecording(void)
{
    Pause();

    request_recording = false;
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

#ifdef DEBUGGING_MODE 
  LOG(VB_RECORD, LOG_INFO, LOC + "Push len="         + QString::number(len)
                                + " ptr_base="    + QString::number(ptr_base-ptr_base)
                                + " ptr_top="     + QString::number(ptr_top-ptr_base)
                                + " ptr_push_pt=" + QString::number(ptr_push_pt-ptr_base)
                                + " to_top="      + QString::number(ptr_top-ptr_push_pt)
                                + " pee="         + QString::number(pee-ptr_base));
#endif

  if(!pee) { 
    LOG(VB_RECORD, LOG_ERR, LOC_ERR + "Push() No myring!  Dropping " + QString::number(len) + " bytes!");
    return; 
  }

  if(pee<=ptr_push_pt) { 
    // we are writing upward and pee is below us (or at us)
    // This means smooth sailing to top of ringbuffer and then a bit more free at bottom 
    rem=ptr_top-ptr_push_pt;
#ifdef DEBUGGING_MODE 
    LOG(VB_RECORD, LOG_INFO, LOC + "Push(a) " + QString::number(len) + " " + QString::number(rem));
#endif
    if(rem > len) {
#ifdef DEBUGGING_MODE 
      LOG(VB_RECORD, LOG_INFO, LOC + "Push(a1) " + QString::number(len) + " " + QString::number(rem));
#endif
      // Whole thing fits.  Push it in
      memcpy(ptr_push_pt,buf,len);
      ptr_push_pt+=len;
      len=0;
      ptr_full_end=ptr_push_pt;  // update consumer end pointer
    } else {
#ifdef DEBUGGING_MODE 
      LOG(VB_RECORD, LOG_INFO, LOC + "Push(a2) " + QString::number(len) + " " + QString::number(rem));
#endif
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
#ifdef DEBUGGING_MODE 
    LOG(VB_RECORD, LOG_INFO, LOC + "Push(b) " + QString::number(len) + " " + QString::number(rem));
#endif
    if(rem >= len) {
#ifdef DEBUGGING_MODE 
      LOG(VB_RECORD, LOG_INFO, LOC + "Push(b1) " + QString::number(len) + " " + QString::number(rem));
#endif
      // Whole thing fits.  Push it in
      memcpy(ptr_push_pt,buf,len);
      ptr_push_pt+=len;
      len=0;
      ptr_full_end=ptr_push_pt;  // update consumer end point
    }
  } 
  if(len) {
    // NO ROOM AT THE INN AIEEEEEE
    LOG(VB_RECORD, LOG_ERR, LOC_ERR + "Push() No room in myring.  Dropping " + QString::number(len) + " bytes!");
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
//#ifdef DEBUGGING_MODE 
//      LOG(VB_RECORD, LOG_INFO, LOC + "Pop(0)");
//#endif
    usleep(2000);
    return;
  }

  // If we were previously at the top of the buffer, we are now at the bottom...
  if(ptr_pop_pt==ptr_top) { ptr_pop_pt=ptr_base; }

#ifdef DEBUGGING_MODE 
  LOG(VB_RECORD, LOG_INFO, , LOC + "Pop ptr_base="   + QString::number(ptr_base-ptr_base)
                               + " ptr_top="    + QString::number(ptr_top-ptr_base)
                               + " ptr_pop_pt=" + QString::number(ptr_pop_pt-ptr_base)
                               + " pfe="        + QString::number(pfe-ptr_base));
#endif

  if(pfe>ptr_pop_pt) { 
    // we are reading upward and pfe is above us.  We cant read past the pfe 
    rem=pfe-ptr_pop_pt;
#ifdef DEBUGGING_MODE 
    LOG(VB_RECORD, LOG_INFO, LOC + "Pop(a) " + QString::number(rem));
#endif
    DoKeyFrame(ptr_pop_pt,rem); 
    ringBuffer->Write(ptr_pop_pt,rem);
    ptr_pop_pt=pfe;
  }
  if(pfe<ptr_pop_pt) { 
    // we are reading upward and pfe is below us
    // This means smooth sailing to top of ringbuffer and then a bit more full at bottom 
    // We will catch the stuff at the bottom next time around (maybe with even more content)
    rem=ptr_top-ptr_pop_pt;
#ifdef DEBUGGING_MODE 
    LOG(VB_RECORD, LOG_INFO, LOC + "Pop(b) " + QString::number(rem));
#endif
    DoKeyFrame(ptr_pop_pt,rem); 
    ringBuffer->Write(ptr_pop_pt,rem);
    ptr_pop_pt+=rem;
  }
  // Update so producer knows how far free space has come
  ptr_empty_end=ptr_pop_pt;
}

bool HavaRecorder::DoKeyFrame(const unsigned char *buf, unsigned int len)
{
  bool ret=false, oops;
  unsigned int i;
  unsigned long hr,min,sec,fr,x;
  long long startpos;

  if(len<128) { return ret; }
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

      // compute frame number using integer math (29.97fps)
      x=(((((hr*60)+min)*60+sec)*2997))/100+fr;

      // If we have never done it, set the base of the keyframe
      // DANGER!  IF THIS ONE IS BOGUS, WE ARE HOSED...
      if(key_base==0xffffffff) { 
        key_base=x;
        key_bogus=1;
        key_last_f=x; key_last_s=sec; key_last_m=min; key_last_h=hr;
      }

      if(key_bogus>3) {
        // reset key detection if we got more than 3 bogus ones in a row
        //
//#ifdef DEBUGGING_MODE 
        LOG(VB_RECORD, LOG_WARNING, LOC + "Reset Key: hr="   + QString::number(hr) 
                                       +     " min="   + QString::number(min) 
                                       +     " sec="   + QString::number(sec) 
                                       +      " fr="   + QString::number(fr) 
                                       + "; lst_hr="   + QString::number(key_last_h)
                                       + " lst_min="   + QString::number(key_last_m)
                                       + " lst_sec="   + QString::number(key_last_s)
                                       +   " lst_F="   + QString::number(key_last_f));
//#endif
        key_last_f=x; key_last_s=sec; key_last_m=min; key_last_h=hr;
      }

      if(x<key_base) { x+=(31*60*60*2997/100); } 
      // HAVA runs counter forever so we might hit a wraparound.  Handle it

      // Do Bogus GOP detection
      // The purpose of this is in case 000001b8 occurs in middle of something
      oops=false;

      if(sec!=key_last_s) {
        if(sec>key_last_s+3)                       { oops=true; } // >3s with no GOPs
        if(sec<2 && key_last_s>2 && key_last_s<58) { oops=true; } // minute wrap with no GOPs in several secs
        if(sec<key_last_s && key_last_s<58)        { oops=true; } // negative time?
        if(sec==0 && min!=(key_last_m+1)%60)       { oops=true; } // sec wrap with no minute wrap
      }
      if(min!=key_last_m) {
        if(min!=(key_last_m+1)%60)           { oops=true; }   // jump >1min?
        if(sec>5)                            { oops=true; }   // no samples in first 5 secs of minute?
        if(key_last_s<55)                    { oops=true; }   // no samples in last 5 secs of previous minute?
        if(min==0 && hr!=(key_last_h+1)%32)  { oops=true; }   // minute wrap without hour increment?
      }
      if(hr!=key_last_h) {
        if(hr!=(key_last_h+1)%32) { oops=true; } // jump >1hr?
        if(min>0)                 { oops=true; } // no samples in first minute of hour?
        if(key_last_m<59)         { oops=true; } // no samples in last minute of previous hour?
      }

      if(oops) {
        key_bogus++;
#ifdef DEBUGGING_MODE 
        LOG(VB_RECORD, LOG_WARNING, LOC + "Skip BogusKey: hr="   + QString::number(hr) 
                                           +     " min="   + QString::number(min) 
                                           +     " sec="   + QString::number(sec) 
                                           +      " fr="   + QString::number(fr) 
                                           + "; lst_hr="   + QString::number(key_last_h)
                                           + " lst_min="   + QString::number(key_last_m)
                                           + " lst_sec="   + QString::number(key_last_s)
                                           +   " lst_F="   + QString::number(key_last_f));
#endif
      } else {
        // REAL GOP...  DUMP A MARKER
        //
        key_bogus=0;

        startpos = ringBuffer->GetWritePosition();
        _frames_seen_count=x-key_base;

#ifdef DEBUGGING_MODE 
        LOG(VB_RECORD, LOG_INFO, LOC + "KeyFrame"
                                  +       " F=" + QString::number(_frames_seen_count)
                                  +     " pos=" + QString::number(startpos+i)
                                  +      " hr=" + QString::number(hr) 
                                  +     " min=" + QString::number(min) 
                                  +     " sec=" + QString::number(sec) 
                                  +      " fr=" + QString::number(fr) 
                                  + "; lst_hr=" + QString::number(key_last_h)
                                  + " lst_min=" + QString::number(key_last_m)
                                  + " lst_sec=" + QString::number(key_last_s)
                                  +   " lst_F=" + QString::number(key_last_f));
#endif

        positionMapLock.lock();
        if (!positionMap.contains(_frames_seen_count))
        {
            positionMapDelta[_frames_seen_count] = startpos+i;
            positionMap[_frames_seen_count]      = startpos+i;
        }
        positionMapLock.unlock();

        key_last_h=hr;
        key_last_m=min;
        key_last_s=sec;
        key_last_f=_frames_seen_count;

        ret=true;
        i=len;
      } 
    }
  }
  if(ret) { CheckForRingBufferSwitch(); }
  return ret;
}

/* vim: set expandtab tabstop=4 shiftwidth=4: */
