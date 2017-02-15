/* RPI / Arduino Shared Display Logic */

#ifdef RPI
#include "displaylogic.h"
#include "../rpi-radio.h"

#include "ArduiPi_OLED_lib.h"
#include "Adafruit_GFX.h"
#include "ArduiPi_OLED.h"

// Display type 3 = Adafruit I2C 128x64
#define MYOLED 3

#endif

#include <stdio.h>
#include <math.h>

#define DISP_LEN 10    // 10 characters
#define BEGIN_PAUSE 10 // 10 Ticks - 1 second
#define END_PAUSE 5    // 5 Ticks - 1/2 second

using namespace std;

class LocalDisplayPrivate: public AbstractDisplay {
  public:
  
    static AbstractDisplay* Factory() {
      if (!singleton) singleton = new LocalDisplayPrivate;
      return singleton;
    }
    
    void heartbeat() {
      if (statusLen > DISP_LEN || alertLen > DISP_LEN) {
        ticks++;
        updateDisplay();
      }
    }
      
    int initiate() {
#ifdef RPI
      if (display.oled_is_spi_proto(MYOLED))
      {
        // SPI parameters
        if ( !display.init(OLED_SPI_DC,OLED_SPI_RESET,OLED_SPI_CS, MYOLED) )
        return -1;
      }
      else
      {
        // I2C parameters
        if ( !display.init(OLED_I2C_RESET,MYOLED) )
        return -1;
      }
      display.begin();
#else
      display.begin(SSD1306_SWITCHCAPVCC, 0x3C);  
#endif
      display.clearDisplay();
      display.display();
      return 0;
    }
    
    void setMode(Mode mode) {
      if (mode != displayMode) {
        displayMode = mode;
        updateDisplay();
      }
    }
    
    void setClock(int hour, int min) {
      if ( (displayHour != hour) || (displayMin != min) ) {
        displayHour = hour;
        displayMin = min;
        updateDisplay();
      }
    }
    
    void setStatusLine(const char* text) {
      if ( strcmp(status, text) != 0 ) {
        strncpy(status, text, MAXCOMMANDLENTH-2);
        statusLen = strlen(status); 
        updateDisplay();
      } 
    }     

    void setAlertLine(const char* text) {
      if ( strcmp(alert, text) != 0 ) {
        strncpy(alert, text, MAXCOMMANDLENTH-2);
        alertLen = strlen(alert);
        updateDisplay();
      } 
    }

    ~LocalDisplayPrivate() {
#ifdef RPI
      display.close();
#endif
    }
    
  private:
    static LocalDisplayPrivate* singleton;
#ifdef RPI
    ArduiPi_OLED display;
#else
    Adafruit_SSD1306 display;
#endif
    Mode displayMode;    
    int displayHour;
    int displayMin;
    char status[MAXCOMMANDLENTH];
    char alert[MAXCOMMANDLENTH];
    int ticks;
    int statusLen;
    int alertLen;

    LocalDisplayPrivate() 
#ifndef RPI
    : display(OLED_TYPE)
#endif
    
    {
      displayMode = DayClock;
      displayHour = 0;
      displayMin = 0;
      status[0] = 0;
      alert[0] = 0;
      ticks = 0;
      statusLen = 0;
      alertLen = 0;
    }
    
    void updateDisplay() {
      
      char displayTime[6];
      snprintf(displayTime, 6, "%02d:%02d", displayHour, displayMin);
            
      display.clearDisplay();
      display.setRotation(2); 
      display.setTextColor(WHITE);

      switch (displayMode) {
        case DayClock:                
          display.setTextSize(3);  
          display.setCursor(17,22);
          display.print(displayTime);
          break;
          
        case DuskClock:
          display.setTextSize(2);  
          display.setCursor(35,50);
          display.print(displayTime);
          break;
          
        case NightClock:
          {
            #define CLOCK_X 65
            #define CLOCK_Y 56
            #define CLOCK_HRAD 7
            float angle;
            int x;
            int y;
               
            // display hour hand
            angle = (displayHour * 30) + (displayMin / 2);
            angle = angle / 57.296; // radians  
            x = sin(angle)*CLOCK_HRAD;
            y = cos(angle)*CLOCK_HRAD;

            display.drawLine(CLOCK_X - x, CLOCK_Y + y, CLOCK_X + x, CLOCK_Y - y, WHITE);          
          }
          break;
          
        case Blank:
          break;
          
        case On:
          {
            // Clock
            display.setTextSize(3);  
            display.setCursor(17,0);
            display.print(displayTime);

            // Status
            display.setTextSize(2);  
            display.setCursor(2,28);
            if (statusLen > DISP_LEN) {
              // Animate it
              animate(status, statusLen);
            }
            else {
              // Centre it
              centre(status, statusLen);
            }

            // Alert
            display.setTextSize(2);  
            display.setCursor(2,50);
            if (alertLen > DISP_LEN) {
              // Animate it
              animate(alert, alertLen);
            }
            else {
              // Centre it
              centre(alert, alertLen);
            }
          }
          break;
      }
      display.display();
    }
    
    void animate(char* text, int len) {
      int step = ticks % (len - DISP_LEN + BEGIN_PAUSE + END_PAUSE);
      if (step <= BEGIN_PAUSE) {
        step = 0;
        // Draw the first characters 
      }
      else if (step > len - DISP_LEN + BEGIN_PAUSE) {
        // Draw the end characters
        step = len - DISP_LEN;
      }
      else {
        // Draw tme middle somewhere
        step -= BEGIN_PAUSE;
      }
      for (int i=0; i<DISP_LEN; i++) {
        display.write(text[ i + step ]);
      }
    }
    
    void centre(char* text, int len) {
      int pad=(DISP_LEN - len)/2;
      for (int i=0; i<pad; i++) {
        display.write(' ');
      }
      for (int i=0; text[i]; i++) {
        display.write(text[i]);
      }
    }
};

LocalDisplayPrivate* LocalDisplayPrivate::singleton = NULL;

AbstractDisplay* LocalDisplay::Factory() {
  return LocalDisplayPrivate::Factory();
}
