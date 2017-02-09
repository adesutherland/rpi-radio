/*
 * Provides the rotary controller (attached to specific GPIO line
 */

#include "rpi-radio.h"
#include <iostream>

#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <wiringPi.h>
#include <pthread.h>
#include <sys/time.h>
#include <sys/socket.h>

using namespace std;

#define BOUNCETIME 2 // 2 ms
#define BUTTON_PIN 6 // Button - Purple - GPIO25 - physical 4 - wiringpi 6
#define CLOCK_PIN 4  // Clockwise  - Grey - GPIO23 - physical 1 - wiringpi 4
#define ANTI_PIN 5   // Anti-clockwise - Yellow - GPIO24 - physical 3 - wiringpi 5

class LocalRotaryControlPrivate: public AbstractRotaryControl {
  public:

    static AbstractRotaryControl* Factory() {
      if (!singleton) singleton = new LocalRotaryControlPrivate;
      return singleton;
    }

    static void staticButtonInterrupt() {
      if (singleton) singleton->buttonInterrupt();
    }   

    static void staticClockInterrupt() {
      if (singleton) singleton->clockInterrupt();
    }

    static void staticAntiInterrupt() {
      if (singleton) singleton->antiInterrupt();
    }
    
    static void* staticReadAndDebounceRotor(void* param) {
      if (singleton) return singleton->readAndDebounceRotor(param);
      return NULL;
    }

    void buttonInterrupt() {
      // Record Value (using mutex's of course)	
      pthread_mutex_lock(&valuesMutex);
      buttonValue = digitalRead(BUTTON_PIN);
      pthread_mutex_unlock(&valuesMutex);
  
      // Kick the debounce thread
      pthread_mutex_lock(&debounceConditionMutex);
      pthread_cond_signal(&debounceThreadCondition);
      pthread_mutex_unlock(&debounceConditionMutex);
    }   

    void clockInterrupt() {
      // Record Value (using mutex's of course)	
      pthread_mutex_lock(&valuesMutex);
      clockValue = digitalRead(CLOCK_PIN);
      pthread_mutex_unlock(&valuesMutex);
  
      // Kick the debounce thread
      pthread_mutex_lock(&debounceConditionMutex);
      pthread_cond_signal(&debounceThreadCondition);
      pthread_mutex_unlock(&debounceConditionMutex);
    }

    void antiInterrupt() {
      // Record Value (using mutex's of course)	
      pthread_mutex_lock(&valuesMutex);
      antiValue = digitalRead(ANTI_PIN);
      pthread_mutex_unlock(&valuesMutex);
  
      // Kick the debounce thread
      pthread_mutex_lock(&debounceConditionMutex);
      pthread_cond_signal(&debounceThreadCondition);
      pthread_mutex_unlock(&debounceConditionMutex);
    }
    
    void *readAndDebounceRotor(void*) {
      bool timedOut = true;
      int lastButtonValue = HIGH;
      int lastClockValue = HIGH;
      int lastAntiValue = HIGH;

      pthread_mutex_lock(&debounceConditionMutex);
      while (!pleaseFinish) {
        if (timedOut) { // Before we timed out so are not waiting for debounce noise 
          pthread_cond_wait(&debounceThreadCondition, &debounceConditionMutex); 
          timedOut = false; // We must have got a signal
        }
        else { // We got a value change - need to wait a bit to see if it is just noise
          struct timespec timeToWait;
          struct timeval now;
          gettimeofday(&now,NULL);
          timeToWait.tv_sec = now.tv_sec;
          timeToWait.tv_nsec = 1000UL * (now.tv_usec+(1000UL * BOUNCETIME)); // tv_nsec is nano, tv_usec is micro {sigh}!	  
          // Carry Seconds ...
          #define BILLION 1000000000
          if (timeToWait.tv_nsec >= BILLION) {
            timeToWait.tv_nsec -= BILLION;
            timeToWait.tv_sec++;
          }
          int rc = pthread_cond_timedwait(&debounceThreadCondition, &debounceConditionMutex, &timeToWait);
          if (rc == ETIMEDOUT) timedOut = true;
          else timedOut = false;
        }

        if (timedOut) {
          pthread_mutex_lock(&valuesMutex); // I don't THINK this will cause a deadlock!

          // OK things have settled down - whats changed?
          if (lastButtonValue != buttonValue) {
            lastButtonValue = buttonValue;
            if (lastButtonValue == HIGH) write(rotarySocket[1], "^", 1); 
            else write(rotarySocket[1], ".", 1);
          }

          if (lastAntiValue != antiValue) {
            lastAntiValue = antiValue;
          }

          if (lastClockValue != clockValue) {
            lastClockValue = clockValue;
            if (lastClockValue == LOW) {
              if (lastAntiValue == LOW) write(rotarySocket[1], ">", 1);
              else write(rotarySocket[1], "<", 1);
            }
          }

          pthread_mutex_unlock( &valuesMutex );
        }
      }
  
      pthread_mutex_unlock( &debounceConditionMutex );
      return NULL;
    }
    
    ~LocalRotaryControlPrivate() {
      pleaseFinish = true; // ask the thread to die
      // And wait for it to end
      singleton = NULL;
      pthread_join(debounceThread, NULL);
      close(rotarySocket[0]);
      close(rotarySocket[1]);

      pthread_mutex_destroy(&valuesMutex);
      pthread_mutex_destroy(&debounceConditionMutex);
      pthread_cond_destroy(&debounceThreadCondition);

      // TODO - There seems to be no way to kill the interupts in wiringPI
      // so the static handlers will just use CPU cycles forever (until I exit)
    }
    
    int connect() {
      // sets up the wiringPi library
      if (wiringPiSetup () < 0) {
        cerr << "Unable to setup wiringPi: " << strerror(errno) << endl;
        return -1;
      }

      // set pins to generate an interrupt
      pinMode(BUTTON_PIN, INPUT);
      pullUpDnControl(BUTTON_PIN, PUD_UP);
      if ( wiringPiISR (BUTTON_PIN, INT_EDGE_BOTH, &staticButtonInterrupt) < 0 ) {
        cerr << "Unable to setup Button ISR: " << strerror(errno) << endl;
        return -1;
      }
 
      pinMode(CLOCK_PIN, INPUT);
      pullUpDnControl(CLOCK_PIN, PUD_UP);
      if ( wiringPiISR (CLOCK_PIN, INT_EDGE_BOTH, &staticClockInterrupt) < 0 ) {
        cerr << "Unable to setup Clockwise ISR: " << strerror(errno) << endl;
        return -1;
      }
 
      pinMode(ANTI_PIN, INPUT);
      pullUpDnControl(ANTI_PIN, PUD_UP);
      if ( wiringPiISR (ANTI_PIN, INT_EDGE_BOTH, &staticAntiInterrupt) < 0 ) {
        cerr << "Unable to setup Anticlockwise ISR: " << strerror(errno) << endl;
        return -1;
      }
    
      // We need a socket to communicate to our thread
      socketpair(PF_LOCAL, SOCK_STREAM, 0, rotarySocket);

      // Thread to read and debounce the rotary control
      pthread_create( &debounceThread, NULL, &staticReadAndDebounceRotor, NULL);

      return rotarySocket[0]; // This socket end receives the output from the controller thread
    }

  private:
    static LocalRotaryControlPrivate* singleton;
    bool pleaseFinish;
    pthread_mutex_t valuesMutex;
    pthread_mutex_t debounceConditionMutex;
    pthread_cond_t debounceThreadCondition;
    pthread_t debounceThread;
    int rotarySocket[2]; // FDs
    int buttonValue;
    int clockValue;
    int antiValue;

    LocalRotaryControlPrivate() {
      pleaseFinish = false;
      buttonValue = HIGH;
      clockValue = HIGH;
      antiValue = HIGH;

      pthread_mutex_init(&valuesMutex, NULL);
      pthread_mutex_init(&debounceConditionMutex, NULL);
      pthread_cond_init(&debounceThreadCondition, NULL);
    }
};

LocalRotaryControlPrivate* LocalRotaryControlPrivate::singleton = NULL;

AbstractRotaryControl* LocalRotaryControl::Factory() {
  return LocalRotaryControlPrivate::Factory();
}
