// -*- Mode: c++ -*-

#ifndef HAVASIGNALMONITOR_H
#define HAVASIGNALMONITOR_H

#include "dtvsignalmonitor.h"

class HavaChannel;

class HavaSignalMonitor: public DTVSignalMonitor
{
  public:
    HavaSignalMonitor(int db_cardnum, HavaChannel* _channel, uint64_t _flags = 0);
    virtual ~HavaSignalMonitor();

  protected:
    HavaSignalMonitor(void);
    HavaSignalMonitor(const HavaSignalMonitor&);

    virtual void UpdateValues(void);

};

#endif // HAVASIGNALMONITOR_H
