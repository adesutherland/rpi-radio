/* Remote Display */

#include "rpi-radio.h"
#include "shared/displaylogic.h"


#include <stdio.h>
#include <math.h>

using namespace std;

class RemoteDisplayPrivate: public AbstractDisplay {
  public:
  
    static AbstractDisplay* Factory() {
      if (!singleton) singleton = new RemoteDisplayPrivate;
      return singleton;
    }
      
    int initiate() {
      return 0;
    }
    
    void setMode(Mode mode) {
      if (mode != displayMode) {
        displayMode = mode;
        displayHour = -1; // Force a display reset
        displayMin = -1;
      }
      // TODO Remote
    }
    
    void setClock(int hour, int min) {
      if ( (displayHour == hour) && (displayMin == min) ) return; // No change
      
      displayHour = hour;
      displayMin = min;
      
      char displayTime[6];
      snprintf(displayTime, 6, "%02d:%02d", displayHour, displayMin);

      // TODO Remote            

    }

    ~RemoteDisplayPrivate() {

    }
    
  private:
    static RemoteDisplayPrivate* singleton;

    Mode displayMode;    
    int displayHour;
    int displayMin;

    RemoteDisplayPrivate()     
    {
      displayMode = DayClock;
      displayHour = -1;
      displayMin = -1;
    }
};

RemoteDisplayPrivate* RemoteDisplayPrivate::singleton = NULL;


AbstractDisplay* RemoteDisplay::Factory() {
  return RemoteDisplayPrivate::Factory();
}







class RemoteRotaryControlPrivate: public AbstractRotaryControl {
  public:

    static AbstractRotaryControl* Factory() {
      if (!singleton) singleton = new RemoteRotaryControlPrivate;
      return singleton;
    }

    ~RemoteRotaryControlPrivate() {
    }
    
    int connect() {


      return -1;
    }

  private:
    static RemoteRotaryControlPrivate* singleton;

    RemoteRotaryControlPrivate() {
    }
};

RemoteRotaryControlPrivate* RemoteRotaryControlPrivate::singleton = NULL;

AbstractRotaryControl* RemoteRotaryControl::Factory() {
  return RemoteRotaryControlPrivate::Factory();
}
