/* 
  RPI Header
*/
#ifndef RPI_RADIO_H
#define RPI_RADIO_H

#include <string>
#include <map>

#define MAXCOMMANDLENTH 70 // Arduino memory constrants - I have 3 of these arrays ...

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

class AbstractModule;
typedef AbstractModule* (*ModuleFactory)();

class AbstractModule {
  public:
    static int initalise();
    static void shutdown();
    static int getSize();
    static int getCurrentIndex();
    static  AbstractModule* getCurrent();
    static int setCurrentIndex(int index);
    static const std::string* getDescriptions();
    

    virtual ~AbstractModule();

    const std::string moduleName;
    const std::string moduleDesc;
    virtual void play() = 0;
    virtual void stop() = 0;
  
  protected:
    static void addModule(std::string name, ModuleFactory factory);
    AbstractModule(std::string name, std::string desc);
    virtual int intialiseModule() = 0;
    
  private:
    static std::map<std::string, ModuleFactory> moduleMap;
    static int currentIndex;
    static ModuleFactory currentModule;

};

class AbstractDisplay;

class Controller {
public:
  Controller(AbstractDisplay *dsp, int socket);
  void handleRotary(char* buffer);
  
  static int handleEvent();
  static void setDisplayMode();
  static void setStatus(const char* text);
  static void clearStatus();
  static void setAlert(const char* text);
  static void clearAlert();
  static void setClock();
  static int mainLoop();
  
  static bool playing;
  static Controller *localConsole;
  static Controller *remoteConsole;
  bool hasFocus;  
  
  static void *clockFunction(void*);
  static void startClock();
  static void stopClock();
  
private:
  AbstractDisplay *display;
  int controlSocket;
};

#endif
