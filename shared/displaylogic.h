/* RPI / Arduino Shared Display Logic */
#ifndef RPI_ARDUINO_DISPLAY_LOGIC_H
#define RPI_ARDUINO_DISPLAY_LOGIC_H

class AbstractDisplay {
  public:
    enum Mode { 
      Blank,
      NightClock,
      DuskClock,
      DayClock,
      On
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
     * Set the status text (blue line - middle)
     */
    virtual void setStatusLine(const char* text) = 0;

    /**
     * Set the alert text (yellow line - bottom)
     */
    virtual void setAlertLine(const char* text) = 0;
    
    /**
     * This function needs to be called 10 times a second to handle screen updates and
     * anamation.
     * heartbeatPrepare() should be called first - caller needs to take care of concurrency
     * heartbeatIsolated() should be called straigth after and
     * is garanteed to be isolated so does not need locking (assuming only one thread 
     * calls the heartbeat). 
     * 
     * The idea is that as the screen takes time to update, having 
     * two calls can be used to minimse the time the object is locked for other threads.
     */
    virtual void heartbeatPrepare() {};
    
    
    /** See heartbeatPrepare()
     */
    virtual void heartbeatIsolated() {};

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

/**
 * This display class is thread safe and automatically calls the heartbeat.
 * It wraps the RemoteDisplay.
 */
class RPIDisplay: public AbstractDisplay {
  public:
    static AbstractDisplay* Factory();

  private:
    RPIDisplay();
};

#endif
