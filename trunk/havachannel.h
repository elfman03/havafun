/**
 *  HavaChannel
 */


#ifndef HAVACHANNEL_H
#define HAVACHANNEL_H

#include <QString>

#include "tv_rec.h"
#include "channelbase.h"

using namespace std;

class HavaChannel : public ChannelBase
{
  public:
    HavaChannel(const QString &addr, TVRec *parent): ChannelBase(parent) { 
          (void)parent; 
          m_curchannelname.clear(); 
          myopen=false; 
          myaddr=addr; 
          return; 
        }
    ~HavaChannel(void) { 
        return; 
    }

    bool IsTunable(const QString &input, const QString &channum) const { 
      return true; 
    }
    virtual bool IsExternalChannelChangeSupported(void) { 
      return true; 
    }

    bool Open(void) { 
      myopen=true; 
      return InitializeInputs(); 
    }
    void Close(void) { 
       myopen=false; 
       return; 
    }

    bool SetChannelByString(const QString &chan) { 
       m_curchannelname = chan; 
       HandleScript(chan /* HACK treat channum as freqid like iptvchannel */ ); 
       return true; 
    }

    bool IsOpen(void) const { 
      return myopen; 
    }
    QString GetDevice(void) const { 
      return myaddr; 
    }
    QString GetCurrentInput(void) const { 
      return "MPEG2TS"; 
    }

  private:
    bool myopen;
    QString myaddr;
};

#endif
