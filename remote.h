/* 
  Remote / Slave Controller - Header
*/
#ifndef REMOTE_CONTROL_H
#define REMOTE_CONTROL_H

/**
 * This class is a proxy that sends commands to the remote / slave device 
 * It is thread safe. Does not need / ignores the display heartbeat 
 */
class RemoteControl: public AbstractRotaryControl, public AbstractDisplay {
  public:
    static RemoteControl* Factory();

  protected:
    RemoteControl() {};
};

#endif
