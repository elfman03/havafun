// -*- Mode: c++ -*-
/*
 * HAVARecorder * HavaFun code component
 * Header file for a MythTV recorder class that is a client of havafun
 *          Hope to donate to MythTV at some point
 *          Used IPTVRecorder.h as a template early on
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

#ifndef _HAVA_RECORDER_H_
#define _HAVA_RECORDER_H_

#include <pthread.h>

#include "dtvrecorder.h"
class HavaChannel;

struct Hava;

/** \brief Processes data from a Hava and writes it to disk.
 */
class HavaRecorder : public DTVRecorder
{
  private: 
    Hava *hava;
    pthread_t hava_thread;
    long long offset;
    unsigned long key_base, 
                  key_last_s, 
                  key_last_m, 
                  key_last_h, 
                  key_last_f, 
                  key_bogus;

    // ptr_base                                                  ptr_top
    // |                                                               |
    // -----------------------------------------------------------------
    // |fffffffffffffffffeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeffffffffffffffff|
    // ----------------------------------------------------------------
    //                 / |                           / |
    //     ptr_full_end  ptr_push_pt     ptr_empty_end ptr_pop_pt

    unsigned char *ptr_base;     // base of ring
    unsigned char *ptr_top;      // top of ring
    unsigned char *ptr_empty_end; // ptr to last ring byte that is empty
    unsigned char *ptr_full_end;  // ptr to last ring byte that is full
    unsigned char *ptr_push_pt;   // ptr to where producer will push new stuff
    unsigned char *ptr_pop_pt;    // ptr to where consumere will pop old stuff

    HavaChannel *channel;

    int record_byte_ct,
        record_seq_ct,
        aspect,
        width,
        height;

    bool DoKeyFrame(const unsigned char *data, unsigned int dataSize);
    void Pop();

  public:

    HavaRecorder(TVRec *rec, HavaChannel*);
    ~HavaRecorder();

    bool Open(void);
    void Close(void);

    virtual void Pause(bool clear = true);

    virtual void StartRecording(void);
    virtual void StopRecording(void);

    virtual void SetOptionsFromProfile(RecordingProfile*, const QString&,
                                       const QString&, const QString&) {}

    void Push(unsigned long now, const unsigned char *data, unsigned int dataSize);

    virtual void SetStreamData(void) {}

  private:
    HavaRecorder &operator=(const HavaRecorder&); //< avoid default impl
    HavaRecorder(const HavaRecorder&);            //< avoid default impl
    HavaRecorder();                                  //< avoid default impl
};

#endif // _HAVA_RECORDER_H_

/* vim: set expandtab tabstop=4 shiftwidth=4: */
