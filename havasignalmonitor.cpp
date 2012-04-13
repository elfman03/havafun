// -*- Mode: c++ -*-
// Copyright (c) 2006, Daniel Thor Kristjansson

#include <cstring>

#include "mythlogging.h"
#include "havasignalmonitor.h"

#include "havachannel.h"

#define LOC QString("HAVASM(%1): ").arg(channel->GetDevice())

/**
 *  \brief Initializes the Hava signal monitor.  Its really dumb right now
 *
 *  \param db_cardnum Recorder number to monitor,
 *                    if this is less than 0, SIGNAL events will not be
 *                    sent to the frontend even if SetNotifyFrontend(true)
 *                    is called.
 *  \param _channel HavaChannel for card
 *  \param _flags   Flags to start with
 */
HavaSignalMonitor::HavaSignalMonitor(
    int db_cardnum, HavaChannel* _channel, uint64_t _flags) :
    DTVSignalMonitor(db_cardnum, _channel, _flags)
{
    LOG(VB_CHANNEL, LOG_INFO, LOC + "ctor");
    signalStrength.SetThreshold(50);
}

/** \fn HavaSignalMonitor::~HavaSignalMonitor()
 *  \brief Stops signal monitoring 
 */
HavaSignalMonitor::~HavaSignalMonitor()
{
    LOG(VB_CHANNEL, LOG_INFO, LOC + "dtor");
    Stop();
}

/** \fn HavaSignalMonitor::UpdateValues(void)
 *  \brief Fills in frontend stats and emits status Qt signals.
 *
 *   This is automatically called by MonitorLoop(), after Start()
 *   has been used to start the signal monitoring thread.
 */
void HavaSignalMonitor::UpdateValues(void)
{
    if (!running || exit)
        return;

    signalStrength.SetValue(50);
    EmitStatus();
    if (IsAllGood())
          SendMessageAllGood();

    update_done = true;
    return;
}
