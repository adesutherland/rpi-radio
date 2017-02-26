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

using namespace std;

class LocalDisplayPrivate: public AbstractDisplay {
  public:
  
    static AbstractDisplay* Factory() {
      if (!singleton) singleton = new LocalDisplayPrivate;
      return singleton;
    }
        
    void heartbeatPrepare() {
      
      if (needsUpdating || statusLen > DISP_LEN || alertLen > DISP_LEN) {
        ticks++;
#ifdef RPI
        isolatedDisplayMode = displayMode; 
        isolatedDisplayHour = displayHour;
        isolatedDisplayMin = displayMin; 
        strcpy(isolatedStatus, status);
        isolatedStatusLen = statusLen;
        strcpy(isolatedAlert, alert);
        isolatedAlertLen = alertLen;
        isolatedTicks = ticks;
        isolatedNeedsUpdating = true;
#else
        updateDisplay(displayMode, displayHour, displayMin, status, statusLen, alert, alertLen, ticks);
#endif
        needsUpdating = false;
      }
    }
    
    void heartbeatIsolated() {
#ifdef RPI
      if (isolatedNeedsUpdating) {
        updateDisplay(isolatedDisplayMode, isolatedDisplayHour, isolatedDisplayMin, 
          isolatedStatus, isolatedStatusLen, isolatedAlert, isolatedAlertLen, isolatedTicks);
      }
#endif    
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
        needsUpdating = true;
      }
    }
    
    void setClock(int hour, int min) {
      if ( (displayHour != hour) || (displayMin != min) ) {
        displayHour = hour;
        displayMin = min;
        needsUpdating = true;
      }
    }
    
    void setStatusLine(const char* text) {
      if ( strcmp(status, text) != 0 ) {
        strncpy(status, text, MAXCOMMANDLENTH-2);
        statusLen = strlen(status); 
        needsUpdating = true;
      } 
    }     

    void setAlertLine(const char* text) {
      if ( strcmp(alert, text) != 0 ) {
        strncpy(alert, text, MAXCOMMANDLENTH-2);
        alertLen = strlen(alert);
        needsUpdating = true;
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
    bool needsUpdating;

#ifdef RPI
    Mode isolatedDisplayMode;    
    int isolatedDisplayHour;
    int isolatedDisplayMin;
    char isolatedStatus[MAXCOMMANDLENTH];
    char isolatedAlert[MAXCOMMANDLENTH];
    int isolatedStatusLen;
    int isolatedAlertLen;
    int isolatedTicks;
    bool isolatedNeedsUpdating;
#endif

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
      needsUpdating = true;
    }
    
    void updateDisplay(Mode mode, int hour, int min, 
      const char* sText, int sLen, const char* aText, int aLen, int tk) {    
          
      char displayTime[6];
      snprintf(displayTime, 6, "%02d:%02d", hour, min);
            
      display.clearDisplay();
      display.setRotation(2); 
      display.setTextColor(WHITE);

      switch (mode) {
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
            angle = (hour * 30) + (min / 2);
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
            if (sLen > DISP_LEN) {
              // Animate it
              animate(sText, sLen, tk);
            }
            else {
              // Centre it
              centre(sText, sLen);
            }

            // Alert
            display.setTextSize(2);  
            display.setCursor(2,50);
            if (aLen > DISP_LEN) {
              // Animate it
              animate(aText, aLen, tk);
            }
            else {
              // Centre it
              centre(aText, aLen);
            }
          }
          break;
      }
      display.display();
    }
    
    void animate(const char* text, int len, int tk) {

      int step = tk % (len + DISP_GAP);

      for (int i=0; i<DISP_LEN; i++) {
        int x = i + step;
        if (x >= len + DISP_GAP) {
          x -= len + DISP_GAP;
          display.write(text[x]);
        }
        else if (x >= len) {
          display.write(' ');
        }
        else {
         display.write(text[x]);
        }
      }
    }
    
    void centre(const char* text, int len) {
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
