/* Remote Display */

#include <termios.h>
#include <fcntl.h>

#include "rpi-radio.h"
#include "shared/displaylogic.h"
#include "remote.h"

#include <stdio.h>
#include <math.h>

using namespace std;

class RemoteControlPrivate: public RemoteControl {
  public:
  
    static RemoteControl* Factory() {
      if (!singleton) singleton = new RemoteControlPrivate;
      return singleton;
    }
      
    int initiate() {
      return 0;
    }
 
    int connect() {
      return filedesc;
    }
    
    void setMode(Mode mode) {
      if (mode != displayMode) {
        displayMode = mode;
        displayHour = -1; // Force a display reset
        displayMin = -1;
      }
      
      int modeID = static_cast<int>(mode);
      char detail[6];
      snprintf(detail, 6, "%d", modeID);
      sendCommand("M", detail);
    }
    
    void setClock(int hour, int min) {
      if ( (displayHour == hour) && (displayMin == min) ) return; // No change
      
      displayHour = hour;
      displayMin = min;
      
      char detail[6];
      snprintf(detail, 6, "%02d:%02d", displayHour, displayMin);
      sendCommand("C", detail);
    }

    ~RemoteControlPrivate() {
      close(filedesc);
    }
    
  private:
    static RemoteControlPrivate* singleton;

    Mode displayMode;    
    int displayHour;
    int displayMin;
    int filedesc;

    RemoteControlPrivate()     
    {
      displayMode = DayClock;
      displayHour = -1;
      displayMin = -1;
      
      struct termios tio;

      memset(&tio,0,sizeof(tio));
      tio.c_iflag=0;
      tio.c_oflag=0;
      tio.c_cflag=CS8|CREAD|CLOCAL;
      tio.c_lflag=0;
      tio.c_cc[VMIN]=1;
      tio.c_cc[VTIME]=5;
        
      filedesc=open("/dev/ttyACM0", O_RDWR | O_NONBLOCK);      
      cfsetospeed(&tio,B19200);
      cfsetispeed(&tio,B19200);

      tcsetattr(filedesc,TCSANOW,&tio);

      sendCommand("Q");
      sendCommand("Q");
      sendCommand("Q");
    }
    
    void sendCommand(const char* command, const char* detail) {
      write(filedesc, command, 1);
      if (detail) {
        write(filedesc, detail, strlen(detail));
      }
      write(filedesc, "\n", 1);
    }
    
    void sendCommand(const char* command) {
      write(filedesc, command, 1);
      write(filedesc, "\n", 1);
    }
    
};

RemoteControlPrivate* RemoteControlPrivate::singleton = NULL;

RemoteControl* RemoteControl::Factory() {
  return RemoteControlPrivate::Factory();
}
