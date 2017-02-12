/* 
  Remote / Slave Controller - Header
*/
#ifndef REMOTE_CONTROL_H
#define REMOTE_CONTROL_H

class RemoteControl: public AbstractRotaryControl, public AbstractDisplay {
  public:
    static RemoteControl* Factory();

  protected:
    RemoteControl() {};
};

#endif
