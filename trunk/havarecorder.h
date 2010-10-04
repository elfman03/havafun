// -*- Mode: c++ -*-
/** 
 * HAVARecorder
 *
 *  Plan to Distribe as part of MythTV under GPL v2 and later.
 *
 * Used IPTVRecorder.h as a template early on
 */

#ifndef _HAVA_RECORDER_H_
#define _HAVA_RECORDER_H_

#include <pthread.h>

#include "dtvrecorder.h"

struct Hava;

/** \brief Processes data from a Hava and writes it to disk.
 */
class HavaRecorder : public DTVRecorder
{
  private: 
    Hava *hava;
    pthread_t hava_thread;
    long long offset;
    unsigned long key_base, key_last_s, key_last_m, key_last_h, key_bogus;
    int pipes[2],
        record_byte_ct,
        record_seq_ct,
        aspect,
        width,
        height;

    bool DoKeyFrame(const unsigned char *data, unsigned int dataSize);

  public:

    HavaRecorder(TVRec *rec);
    ~HavaRecorder();

    int GetPipe(int which);

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
