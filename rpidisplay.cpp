/* RPI Display Wrapper */

#include <pthread.h>
#include <time.h>
#include "shared/displaylogic.h"
#include <pthread.h>

class RPIDisplayPrivate: public AbstractDisplay {
  public:
  
    static AbstractDisplay* Factory() {
      if (!singleton) singleton = new RPIDisplayPrivate;
      return singleton;
    }
      
    int initiate() {
      int rc = wrappedDisplay->initiate();
      if (!rc) { 
        pthread_create( &heartbeatThread, NULL, &staticHeartbeatFunction, NULL);
        initiated = true;
      }
      return rc;
    }
     
    void setMode(Mode mode) {
      pthread_mutex_lock(&mutex);
      wrappedDisplay->setMode(mode);
      pthread_mutex_unlock(&mutex);
    }
    
    void setClock(int hour, int min) {
      pthread_mutex_lock(&mutex);
      wrappedDisplay->setClock(hour, min);
      pthread_mutex_unlock(&mutex);
    }

    void setStatusLine(const char* text) {
      pthread_mutex_lock(&mutex);
      wrappedDisplay->setStatusLine(text);      
      pthread_mutex_unlock(&mutex);
    }

    void setAlertLine(const char* text) {
      pthread_mutex_lock(&mutex);
      wrappedDisplay->setAlertLine(text);      
      pthread_mutex_unlock(&mutex);
    }

    ~RPIDisplayPrivate() {
      if (initiated) {
        pleaseFinish = true; // ask the thread to die
        pthread_join(heartbeatThread, NULL); // And wait for it to end
      }
      pthread_mutex_destroy(&mutex);
      singleton = NULL;
      delete wrappedDisplay;
    }
    
  private:
    static RPIDisplayPrivate* singleton;
    AbstractDisplay* wrappedDisplay;
    pthread_mutex_t mutex;
    bool pleaseFinish;
    pthread_t heartbeatThread;
    struct timespec timeToWait;
    bool initiated;

    RPIDisplayPrivate()     
    {
      pthread_mutex_init(&mutex, NULL);
      wrappedDisplay = LocalDisplay::Factory();
      pleaseFinish = false;
      initiated = false;
    }
    
    static void *staticHeartbeatFunction(void*) {
      if (singleton) singleton->heartbeatFunction();
      return NULL;
    }
    
    void heartbeatFunction() {
      clock_gettime(CLOCK_MONOTONIC, &timeToWait);

      while (!pleaseFinish) {
        // Wait 'till the next heartbeat is due - avoiding drift 
        timeToWait.tv_nsec += 1000000L * HEARTBEAT_MS;	  
        // Carry Seconds ...
        #define BILLION 1000000000L
        if (timeToWait.tv_nsec >= BILLION) {
          timeToWait.tv_nsec -= BILLION;
          timeToWait.tv_sec++;
        }
        clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &timeToWait, NULL);

        // Call hearbeat
        pthread_mutex_lock(&mutex);
        wrappedDisplay->heartbeatPrepare();      
        pthread_mutex_unlock(&mutex);
        wrappedDisplay->heartbeatIsolated(); 
         
        // Repeat!
      }
    }
};

RPIDisplayPrivate* RPIDisplayPrivate::singleton = NULL;

AbstractDisplay* RPIDisplay::Factory() {
  return RPIDisplayPrivate::Factory();
}
