/* Remote Display */

#include <termios.h>
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>

#include "rpi-radio.h"
#include "shared/displaylogic.h"
#include "remote.h"

#include <stdio.h>
#include <math.h>
#include <cstring>

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
      pthread_mutex_lock(&mutex);
      
      // The display mode is always sent - just to ensure displays are synced
      displayMode = mode;
      int modeID = static_cast<int>(mode);
      char detail[6];
      snprintf(detail, 6, "%d", modeID);
      sendCommand("M", detail);
      
      pthread_mutex_unlock(&mutex);
    }
    
    void setClock(int hour, int min) {
      pthread_mutex_lock(&mutex);
      
      if ( (displayHour != hour) || (displayMin != min) ) {
        displayHour = hour;
        displayMin = min;
      
        char detail[6];
        snprintf(detail, 6, "%02d:%02d", displayHour, displayMin);
        sendCommand("C", detail);
      }
      
      pthread_mutex_unlock(&mutex);
    }

    void setStatusLine(const char* text) {
      pthread_mutex_lock(&mutex);
      
      if ( strcmp(status, text) != 0 ) {
        strncpy(status, text, MAXCOMMANDLENTH-2); 
        sendCommand("S", status);
      }
      
      pthread_mutex_unlock(&mutex);
    }

    void setAlertLine(const char* text) {
      pthread_mutex_lock(&mutex);
      if ( strcmp(alert, text) != 0 ) {
        strncpy(alert, text, MAXCOMMANDLENTH-2);      
        sendCommand("A", alert);
      }
      
      pthread_mutex_unlock(&mutex);
    }

    ~RemoteControlPrivate() {
      pthread_mutex_destroy(&mutex);
      close(filedesc);
    }
    
  private:
    static RemoteControlPrivate* singleton;
    
    pthread_mutex_t mutex;
    Mode displayMode;    
    int displayHour;
    int displayMin;
    int filedesc;
    char status[MAXCOMMANDLENTH];
    char alert[MAXCOMMANDLENTH];

    RemoteControlPrivate()     
    {
      pthread_mutex_init(&mutex, NULL);
      displayMode = DayClock;
      displayHour = 0;
      displayMin = 0;
      status[0] = 0;;
      alert[0] = 0;
      
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
