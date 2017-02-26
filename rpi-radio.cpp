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


int main(int argc, char **argv)
{	
  // Local Display (wrapped - threadsafe with a heartbeat thread
  AbstractDisplay* localDisplay = RPIDisplay::Factory();
  if (localDisplay->initiate()) {
    cerr << "Error starting local display" << endl;
    return 1;
	}
  // Setup the local rotary control
  AbstractRotaryControl* localRotary = LocalRotaryControl::Factory();
  int rotarySocket = localRotary->connect();
  if (rotarySocket < 0) {
    cerr << "Error starting local control" << endl;
    return 1;
  }
  Controller::localConsole = new Controller(localDisplay, rotarySocket);
  
  // Connect go the slave / remote
  RemoteControl* remote = RemoteControl::Factory();
  if (!remote) {
    cerr << "Error starting remote" << endl;
    return 1;
  }
  int remoteSocket = remote->connect();
  if (remoteSocket < 0) {
    cerr << "Error starting remote control" << endl;
    return 1;
  }
  if (remote->initiate()) {
    cerr << "Error starting remote display" << endl;
    return 1;
	}
  Controller::remoteConsole = new Controller(remote, remoteSocket);
  
  // Initialise Modules
  if (AbstractModule::initalise()) {
    cerr << "Error starting modules" << endl;
    return 1;
  }  
  if (AbstractModule::getSize() < 1) {
    cerr << "Error no modules!" << endl;
    return 1;
  }
  
  Controller::startClock();

  // Mainloop
  Controller::mainLoop();
  	
	// Never get here - but anyway - (todo - could catch a signal)
  Controller::stopClock();
  AbstractModule::shutdown(); 
  delete localDisplay;
  delete remote;
  delete localRotary;
  // TODO delete consoles/controlers?
}
