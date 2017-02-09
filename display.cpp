/*
 * Displays on a Monochrome OLEDs based on SSD1306 driver
 * Used the adafruit provided library
 */

#include "rpi-radio.h"

#include <stdio.h>
#include <math.h>

#include "ArduiPi_OLED_lib.h"
#include "Adafruit_GFX.h"
#include "ArduiPi_OLED.h"

// Display type 3 = Adafruit I2C 128x64
#define MYOLED 3

using namespace std;

class LocalDisplayPrivate: public AbstractDisplay {
  public:
  
    static AbstractDisplay* Factory() {
      if (!singleton) singleton = new LocalDisplayPrivate;
      return singleton;
    }
      
    int initiate() {
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
      return 0;
    }
    
    void setMode(Mode mode) {
      if (mode != displayMode) {
        displayMode = mode;
        displayHour = -1; // Force a display reset
        displayMin = -1;
      }
    }
    
    void setClock(int hour, int min) {
      if ( (displayHour == hour) && (displayMin == min) ) return; // No change
      displayHour = hour;
      displayMin = min;
      
      char displayTime[6];
      snprintf(displayTime, 6, "%02d:%02d", displayHour, displayMin);
            
      display.clearDisplay();
	    display.setRotation(2); 
      display.setTextColor(WHITE);

      switch (displayMode) {
        case DayClock:         
          display.setBrightness(0x80);
          display.setTextSize(4);  
          display.setCursor(5,18);
          display.print(displayTime);
          break;
          
        case DuskClock:
          display.setBrightness(0);
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

            display.setBrightness(0);
            display.drawLine(CLOCK_X - x, CLOCK_Y + y, CLOCK_X + x, CLOCK_Y - y, WHITE);          
          }
          break;
          
        case Blank:
          break;
      }
      display.display();
    }

    ~LocalDisplayPrivate() {
      display.close();
    }
    
  private:
    static LocalDisplayPrivate* singleton;
    ArduiPi_OLED display;
    Mode displayMode;    
    int displayHour;
    int displayMin;

    LocalDisplayPrivate() {
      displayMode = DayClock;
      displayHour = -1;
      displayMin = -1;
    }
};

LocalDisplayPrivate* LocalDisplayPrivate::singleton = NULL;

AbstractDisplay* LocalDisplay::Factory() {
  return LocalDisplayPrivate::Factory();
}
