/* 
  RPI Header
*/
#ifndef RPI_RADIO_H
#define RPI_RADIO_H

#include <string>

class Pulseaudio;

class VolumeControl {
public:
  static int increaseVolume();
  static int decreaseVolume();
private:
  VolumeControl() {};
  static Pulseaudio pulse;
};

class AbstractRotaryControl {
  public:
    /**
     * Desctructor which also closes the controller
     */
     virtual ~AbstractRotaryControl() {};
    
    /**
     * Initiated and gets a socket for the control output
     * @return the file descriptor (FD) for the socket
     */ 
    virtual int connect() = 0;
  
  protected:
    AbstractRotaryControl() {};
};

class LocalRotaryControl: public AbstractRotaryControl {
  public:
    static AbstractRotaryControl* Factory();

  private:
    LocalRotaryControl();
};

class AbstractDisplay {
  public:
    enum Mode { 
      Blank,
      NightClock,
      DuskClock,
      DayClock
    };
    
    /**
     * Initiate and setup display
     */
    virtual int initiate() = 0;
    
    /** 
     * Set Display mode
     */
    virtual void setMode(Mode mode) = 0;
    
    /**
     * Set the time and displays it
     */
    virtual void setClock(int hour, int min) = 0;

    /**
     * Destructor which also closes any resources
     */
    virtual ~AbstractDisplay() {};
    
  protected:
    AbstractDisplay() {};
};

class LocalDisplay: public AbstractDisplay {
  public:
    static AbstractDisplay* Factory();

  private:
    LocalDisplay();
};

#endif
