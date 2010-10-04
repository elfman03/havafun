// -*- Mode: c++ -*-
/** 
 * HAVARecorder.cpp
 *
 *  Plan to Distribe as part of MythTV under GPL v2 and later.
 *
 * Used IPTVRecorder.h as a template early on
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
    if(Hava_isbound(hava)) {
      Hava_set_bonus(hava, (void *)this);
      Hava_set_videocb(hava, &my_callback);
      Hava_set_videoquality(hava, 0x030);
      Hava_sendcmd(hava, HAVA_INIT, 0, 0);
      if(Hava_loop(hava, HAVA_MAGIC_INIT,0)!=1) {
        VERBOSE(VB_IMPORTANT, LOC + "Open() -- bad Hava init response");
        _error = true;
      }
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
    _frames_written_count=0;
    key_base=0xffffffff;

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
        unsigned char buf[10240];
        status=read(pipes[0],buf,10240);
        if(status) {
          if(DoKeyFrame(buf,status)) {
            CheckForRingBufferSwitch();
          }
          ringBuffer->Write(buf,status);
          _frames_written_count=_frames_seen_count;
        } else {
          _error=1;
        }
      }
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
// AddData : feed data from Hava flow to mythtv
// ===================================================
void HavaRecorder::Push(unsigned long now, const unsigned char *buf, unsigned int len) {
  write(GetPipe(1),buf,len);
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
  return ret;
}

/* vim: set expandtab tabstop=4 shiftwidth=4: */
