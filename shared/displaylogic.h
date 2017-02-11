/* RPI / Arduino Shared Display Logic */
#ifndef RPI_ARDUINO_DISPLAY_LOGIC_H
#define RPI_ARDUINO_DISPLAY_LOGIC_H

#ifdef RPI

#include "ArduiPi_OLED_lib.h"
#include "Adafruit_GFX.h"
#include "ArduiPi_OLED.h"

// Display type 3 = Adafruit I2C 128x64
#define MYOLED 3

#endif

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

class RemoteDisplay: public AbstractDisplay {
  public:
    static AbstractDisplay* Factory();

  private:
    RemoteDisplay();
};

#endif
