/*
 * Main RPI Radio Program
 */
 
#include <iostream>

#include <sys/select.h>
#include <stdio.h>
#include <unistd.h>

#include "rpi-radio.h"
#include "shared/displaylogic.h"
#include "remote.h"

using namespace std;

AbstractDisplay *display;
RemoteControl *remote;

int rotarySocket;
int remoteSocket;

void handleRotary(char* buffer) {
  static bool playing = false;
  
  for (int i=0; buffer[i]; i++) {
	  switch (buffer[i]) {
		  case '.':
        {
          //			printf(".\n");
          AbstractModule* module = AbstractModule::getCurrent();
          if (module) {
            if (playing) {
              module->stop();               
              playing = false;
            }
            else {
              module->play();              
              playing = true;
            }
          }
        }
        break;

		  case '^':
//			printf("^\n");
        break;

		  case '>':
//			printf(">\n");
        VolumeControl::increaseVolume();
        break;
			
		  case '<':
//			printf("<\n");
        VolumeControl::decreaseVolume();
        break;

		  default:
		    ;
	  }
  }
}

int readResponse(int secs)
{
  fd_set set;
  struct timeval timeout;
  int rc;
  char buff[100];
  int len = 100;

  FD_ZERO(&set); /* clear the set */
  FD_SET(remoteSocket, &set); /* add file descriptor to the set - slave device */
  FD_SET(rotarySocket, &set); /* add file descriptor of our local rotary device */

  timeout.tv_sec = secs;
  timeout.tv_usec = 0;
  
  int maxFD = remoteSocket;
  if (rotarySocket > maxFD) maxFD = rotarySocket;

  rc = select(maxFD + 1, &set, NULL, NULL, &timeout);
  if (rc == -1) {
    perror("select\n"); /* an error accured */
    return -1;
  }
  
  if (rc == 0) {
    return 0;
  }
  
  if (FD_ISSET(remoteSocket, &set)) { // Slave Response available
    rc = read( remoteSocket, buff, len ); 
    buff[rc] = 0;
//    printf("slave response: [%s]\n", buff);
    handleRotary(buff);
  }

  if (FD_ISSET(rotarySocket, &set)) { // Rotary events available
    rc = read( rotarySocket, buff, len ); 
    buff[rc] = 0;
//    printf("master response: [%s]\n", buff);
    handleRotary(buff);
  }

  return 1;
}

int main(int argc, char **argv)
{	
    time_t rawtime;
    struct tm * timeinfo;
    char timeText[6];


  // Local Display
  display = LocalDisplay::Factory();
  if (display->initiate()) {
    cerr << "Error starting local display" << endl;
    return 1;
	}

  // Setup the local rotary control
  AbstractRotaryControl* localRotary = LocalRotaryControl::Factory();
  rotarySocket = localRotary->connect();
  if (rotarySocket < 0) {
    cerr << "Error starting local control" << endl;
    return 1;
  }
  
  // Connect go the slave
  remote = RemoteControl::Factory();
  if (!remote) {
    cerr << "Error starting remote" << endl;
    return 1;
  }
  remoteSocket = remote->connect();
  if (remoteSocket < 0) {
    cerr << "Error starting remote control" << endl;
    return 1;
  }
  if (remote->initiate()) {
    cerr << "Error starting remote display" << endl;
    return 1;
	}
  
  // Initialise Modules
  if (AbstractModule::initalise()) {
    cerr << "Error starting modules" << endl;
    return 1;
  }
  
  if (AbstractModule::getSize() < 1) {
    cerr << "Error no modules!" << endl;
    return 1;
  }

  // Loop to display local time every minute
  int respRC = 0;
  while (true) {
    time ( &rawtime );
    timeinfo = localtime ( &rawtime );
    sprintf(timeText, "%02d:%02d", timeinfo->tm_hour, timeinfo->tm_min);

	  if (respRC == 0 || (60-timeinfo->tm_sec == 0)) { // respRC==0 means readResponse() timed out so the minute is up and we need to update the clock	
         
      if ( (timeinfo->tm_hour >= 7 && timeinfo->tm_hour < 20) ) {
        // Daytime
        display->setMode(AbstractDisplay::DayClock); 
        remote->setMode(AbstractDisplay::DayClock); 
	    }
        
      else if ( (timeinfo->tm_hour >= 6 && timeinfo->tm_hour < 7) ) {
        // Dawn
        display->setMode(AbstractDisplay::DuskClock);
        remote->setMode(AbstractDisplay::DuskClock);
      }
         
      else if ( (timeinfo->tm_hour >= 20 && timeinfo->tm_hour < 22) ) {
        // Dusk
        display->setMode(AbstractDisplay::DuskClock);
        remote->setMode(AbstractDisplay::DuskClock);
      }
         
      else {
        // Night 
        display->setMode(AbstractDisplay::NightClock);
        remote->setMode(AbstractDisplay::Blank);
//        remore->setMode(AbstractDisplay::NightClock);
      }
      display->setClock(timeinfo->tm_hour, timeinfo->tm_min);
      remote->setClock(timeinfo->tm_hour, timeinfo->tm_min);
	  } 
     
    // sleep until the next minute starts (i.e. likely after 60 seconds)
    respRC = readResponse(60-timeinfo->tm_sec); // Also processes and rotary events
	}
	
	// Never get here - but anyway - (todo - could catch a signal)

  AbstractModule::shutdown(); 
  delete display;
  delete remote;
      
  // Kill the Rotary Controller
  delete localRotary;
}
