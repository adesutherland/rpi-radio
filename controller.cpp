/**
 * Console - pairing of display and control
 */
 
#include <iostream>
#include <string>
#include <sstream>

#include <sys/select.h>
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>
 
#include "rpi-radio.h"
#include "shared/displaylogic.h"

using namespace std;

Controller *Controller::localConsole = NULL;
Controller *Controller::remoteConsole = NULL;
Controller *Controller::hasFocus = NULL;
bool Controller::playing = false;

Controller::Controller(AbstractDisplay *dsp, int socketForController) {
  display = dsp;
  controlSocket = socketForController;
  lastEventSecond = 0;
  downSecond = -1;
}

void Controller::grabFocus() {
  grabFocus(this);
}

void Controller::giveUpFocus() {
  hasFocus = NULL;
}

void Controller::grabFocus(Controller* controller) {
  hasFocus = controller;
}

void Controller::setDisplayMode() {
  time_t rawtime;
  struct tm * timeinfo;
  
  if (playing) {
    localConsole->display->setMode(AbstractDisplay::On); 
    remoteConsole->display->setMode(AbstractDisplay::On); 
  }
  
  else {
    time ( &rawtime );
    timeinfo = localtime ( &rawtime );

    if ( (timeinfo->tm_hour >= 7 && timeinfo->tm_hour < 20) ) {
      // Daytime
      localConsole->display->setMode(AbstractDisplay::DayClock); 
      remoteConsole->display->setMode(AbstractDisplay::DayClock); 
    }
        
    else if ( (timeinfo->tm_hour >= 6 && timeinfo->tm_hour < 7) ) {
      // Dawn
      localConsole->display->setMode(AbstractDisplay::DuskClock);
      remoteConsole->display->setMode(AbstractDisplay::DuskClock);
    }
         
    else if ( (timeinfo->tm_hour >= 20 && timeinfo->tm_hour < 22) ) {
      // Dusk
      localConsole->display->setMode(AbstractDisplay::DuskClock);
      remoteConsole->display->setMode(AbstractDisplay::DuskClock);
    }
         
    else {
      // Night 
      localConsole->display->setMode(AbstractDisplay::NightClock);
      remoteConsole->display->setMode(AbstractDisplay::Blank);
//        remoteConsole->display->setMode(AbstractDisplay::NightClock);
    }
  } 
}

void Controller::setClock() {
  time_t rawtime;
  struct tm * timeinfo;
  
  time ( &rawtime );
  timeinfo = localtime ( &rawtime );
  
  localConsole->display->setClock(timeinfo->tm_hour, timeinfo->tm_min);
  remoteConsole->display->setClock(timeinfo->tm_hour, timeinfo->tm_min);
}

void Controller::setStatus(const char* text) {
  localConsole->display->setStatusLine(text);
  remoteConsole->display->setStatusLine(text);
}

void Controller::clearStatus() {
  localConsole->display->setStatusLine("");
  remoteConsole->display->setStatusLine("");
}

void Controller::setAlert(const char* text) {
  localConsole->display->setAlertLine(text);
  remoteConsole->display->setAlertLine(text);
}

void Controller::clearAlert() {
  localConsole->display->setAlertLine("");
  remoteConsole->display->setAlertLine("");
}

void Controller::handleRotary(char* buffer) {

  AbstractModule* module = AbstractModule::getCurrent();
  
  if (!buffer) {
    if (time(NULL) - lastEventSecond > 2) {
      display->setAlertLine("");
      giveUpFocus();
      if (downSecond != -1) {
        downSecond = -1;
        // Process Press
        if (module) {
          if (playing) {
            grabFocus();
            display->setAlertLine("Turn Off");
            module->stop();               
            playing = false;
            setDisplayMode();
            clearStatus();
          }
        }
      }
    }
    return;
  }
  lastEventSecond = time(NULL);
  
  for (int i=0; buffer[i]; i++) {
	  switch (buffer[i]) {
		  case '.':
        downSecond = time(NULL);
        break;

		  case '^':
        {
          if (downSecond != -1) {
            bool longPress = false;
            if (time(NULL) - downSecond > 1) longPress = true;
            downSecond = -1;
          
            if (module) {
              if (playing) {
                if (longPress) {
                  grabFocus();
                  display->setAlertLine("Turn Off");
                  module->stop();               
                  playing = false;
                  setDisplayMode();
                  clearStatus();
                }
                else {
                  grabFocus();
                  int vol = VolumeControl::getVolume();
                  ostringstream s;            
                  s << " Vol " << vol;
                  display->setAlertLine(s.str().c_str());
                }
              }
              else {
                grabFocus();
                display->setAlertLine("Turn On");
                module->play();              
                playing = true;
                setDisplayMode();
                setStatus(module->moduleDesc.c_str());
              }
            }
            else {
              grabFocus();
              display->setAlertLine("No Module");
            }
          }
        }
        break;

		  case '>':
        {       
          if (playing) { 
            grabFocus();
            int vol = VolumeControl::increaseVolume();
            ostringstream s;            
            s << " Vol " << vol << ">";
            display->setAlertLine(s.str().c_str());
          }
        }
        break;
			
		  case '<':
        {    
          if (playing) {    
            grabFocus();
            int vol = VolumeControl::decreaseVolume();
            ostringstream s;          
            s << "<Vol " << vol << " ";  
            display->setAlertLine(s.str().c_str());
          }
        }
        break;
        

		  default:
		    ;
	  }
  }
}

int Controller::handleEvent() {
  fd_set set;
  struct timeval timeout;
  int rc;
  char buff[100];
  int len = 100;

  FD_ZERO(&set);
  FD_SET(localConsole->controlSocket, &set);
  FD_SET(remoteConsole->controlSocket, &set);

  timeout.tv_sec = 1; 
  timeout.tv_usec = 0;
  
  int maxFD = localConsole->controlSocket;
  if (remoteConsole->controlSocket > maxFD) maxFD = remoteConsole->controlSocket;

  rc = select(maxFD + 1, &set, NULL, NULL, &timeout);
  if (rc == -1) {
    perror("select\n");
    return -1;
  }
  
  if (rc == 0) {
    if (hasFocus) hasFocus->handleRotary(NULL);
    else {
      remoteConsole->handleRotary(NULL);
      localConsole->handleRotary(NULL);
    }
    return 0;
  }
  
  if (FD_ISSET(remoteConsole->controlSocket, &set)) { // Slave Response available
    rc = read( remoteConsole->controlSocket, buff, len ); 
    buff[rc] = 0;
    if (!hasFocus || remoteConsole == hasFocus) remoteConsole->handleRotary(buff);
  }

  if (FD_ISSET(localConsole->controlSocket, &set)) { // Rotary events available
    rc = read( localConsole->controlSocket, buff, len ); 
    buff[rc] = 0;
    if (!hasFocus || localConsole == hasFocus) localConsole->handleRotary(buff);
  }
  
  // Make sure we send an event to the focused controller
  if (hasFocus) hasFocus->handleRotary(NULL);

  return 0;
}

static pthread_t clockThread; // Not in the class - sigh!
static bool stopTheClock = false;

void* Controller::clockFunction(void*) {
  time_t rawtime;
  struct tm * timeinfo;

  while (!stopTheClock) {
    Controller::setDisplayMode(); 
    Controller::setClock();

    time ( &rawtime );
    timeinfo = localtime ( &rawtime );
    sleep(60-timeinfo->tm_sec);
	}
  return NULL;
}

void Controller::startClock() {
  pthread_create( &clockThread, NULL, &clockFunction, NULL);
}

void Controller::stopClock() {
  stopTheClock = true;
  pthread_join(clockThread, NULL);
}

int Controller::mainLoop() {
  int respRC = 0;
  while (!respRC) {
    respRC = Controller::handleEvent();
	}

  return respRC;
}
