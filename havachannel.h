/**
 *  HavaChannel
 */


#ifndef HAVACHANNEL_H
#define HAVACHANNEL_H

#include <QString>
#include "dtvchannel.h"

using namespace std;

class HavaChannel : public DTVChannel
{
  public:
    HavaChannel(TVRec *parent): DTVChannel(parent) { 
           // m_curchannelname.clear(); 
           // curinputname.clear(); 
           myopen=false;
           return; 
    }
    ~HavaChannel(void) { return; }

    bool IsTunable(const QString &input, const QString &channum) const
        { return true; }

    bool Open(void)     { myopen=true; return InitializeInputs(); }
    void Close(void)    { myopen=false; return; }

    virtual bool IsExternalChannelChangeSupported(void) { return true; }

    // Sets

    // Must use external tuner!
    virtual bool Tune(const DTVMultiplex&, QString) { return false; }

    // Currently uses the parent tuner functionality
    // bool SetChannelByString(const QString &chan);

    // Gets

    bool    IsOpen(void)             const { return myopen; }
    
    // Currently uses the parent tuner functionality
    // QString GetDevice(void)          const { return "/dev/hava"; }
    // QString GetCurrentInput(void)    const { return "MPEG2TS"; }
    // uint    GetCurrentSourceID(void) const { return 0; }

    private:

    bool myopen;

};

#endif
