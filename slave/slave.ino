#include <SPI.h>
#include <Wire.h>
#include <stdlib.h>

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define OLED_TYPE 4

#if (SSD1306_LCDHEIGHT != 64)
#error("Height incorrect, please fix Adafruit_SSD1306.h!");
#endif

#include "displaylogic.h"

AbstractDisplay *slavedisplay;

const byte maxSize = MAXCOMMANDLENTH - 1;
char receivedCommand[maxSize+1];
boolean newCommand = false;

int buttonPin = 10;
int encoderPinA = 11;
int encoderPinB = 12;
int buttonPinLast = HIGH;
int encoderPinALast = HIGH;
int encoderPinBLast = HIGH;
int buttonPinState = HIGH;
int encoderPinAState = HIGH;
int encoderPinBState= HIGH;
int a = HIGH;
int b = HIGH;
int c = HIGH;

unsigned long debounceTime = 0;
unsigned long heartbeatTime = 0;
unsigned long debounceDelay = 2;

/*
void setBrightness(Adafruit_SSD1306 display, uint8_t brightness)
{
  display.ssd1306_command(SSD1306_SETCONTRAST);
  display.ssd1306_command(brightness);
}
*/

void setup()   {                
  Serial.begin(19200);
  
  pinMode (encoderPinA,INPUT);
  digitalWrite(encoderPinA, HIGH);

  pinMode (encoderPinB,INPUT);
  digitalWrite(encoderPinB, HIGH);

  pinMode (buttonPin,INPUT);
  digitalWrite(buttonPin, HIGH);
  
  // Local Display
  slavedisplay = LocalDisplay::Factory();
  slavedisplay->initiate();
  
  debounceTime = millis();
  heartbeatTime = millis();
  
  // Timer0 is already used for millis() - we'll just interrupt somewhere
  // in the middle and call the "Compare A" function below
  OCR0A = 0xAF;
  TIMSK0 |= _BV(OCIE0A);  
}

void loop() {
     
   readCommand();

   if (newCommand) {
      switch (receivedCommand[0]) {
        
        case 'M': // Set Mode
        {
          int m = atoi(receivedCommand+1);
          AbstractDisplay::Mode mode = static_cast<AbstractDisplay::Mode>(m);
          slavedisplay->setMode(mode);          
        }
        break;

        case 'C': // Set Clock
        {
          receivedCommand[3]=0;
          int h = atoi(receivedCommand+1);
          int m = atoi(receivedCommand+4);
          slavedisplay->setClock(h,m);
        }
        break;
        
        case 'S': // Set Status
        {
          slavedisplay->setStatusLine(receivedCommand+1);
        }
        break;

        case 'A': // Set Alert
        {
          slavedisplay->setAlertLine(receivedCommand+1);
        }
        break;
        
        case 'Q': // If the board was still running the bootloader Q would exit and load [this] program
                  // So this is just to make a friendly response - as I am already running
          slavedisplay->setMode(AbstractDisplay::DayClock); // Master must be restarting 
        case 0:   // Empty commands - likely syncing when booting         
          Serial.println("K");
          break;
 
        default:
          Serial.println("E"); // Invalid Command
      }
   }
   
   // Heartbeat
   if ( millis() < heartbeatTime ) {
     // Just in case it overflows (after 50 days!) 
     heartbeatTime = millis();
   }
   if ( millis() - heartbeatTime >= HEARTBEAT_MS) {
     heartbeatTime = millis();
     slavedisplay->heartbeatPrepare();
     slavedisplay->heartbeatIsolated(); 
   }
}


// Interrupt is called once a millisecond - rotar control
SIGNAL(TIMER0_COMPA_vect) {
  a = digitalRead(encoderPinA);
  if (a != encoderPinALast) {
    debounceTime = millis();
  }
  
  b = digitalRead(encoderPinB);
  if (b != encoderPinBLast) {
    debounceTime = millis();
  }
  
  c = digitalRead(buttonPin);
  if (c != buttonPinLast) {
    debounceTime = millis();
  }
  
  if ( millis() < debounceTime ) {
    // Just in case it overflows (after 50 days) 
    debounceTime = millis();
  }
  
  if ((millis() - debounceTime) > debounceDelay) {
    if ((encoderPinAState == HIGH) && (a == LOW)) {
      if (Serial.availableForWrite()) {
        if (b == LOW) {
          Serial.print ("<");
        } else {
          Serial.print (">");
        }
      }
    } 
    encoderPinAState = a;
    encoderPinBState = b;
  
    if ((buttonPinState == HIGH) && (c == LOW)) {
      if (Serial.availableForWrite()) {
        Serial.print (".");
      }
    } 
    else if ((buttonPinState == LOW) && (c == HIGH)) {
      if (Serial.availableForWrite()) {
        Serial.print ("^");
      }
    } 
    buttonPinState = c;
  }
  
  encoderPinALast = a;
  encoderPinBLast = b;
  buttonPinLast = c;
}

void readCommand() {
  static byte ndx = 0;
  char endMarker = '\n';
  char rc;
  
  newCommand = false;

  while (Serial.available() > 0 && !newCommand) {
    rc = Serial.read();

    if (rc != endMarker) {
      receivedCommand[ndx] = rc;
      ndx++;
      if (ndx > maxSize) {
        ndx = maxSize;
      }
    }
    else {
      receivedCommand[ndx] = '\0';
      ndx = 0;
      newCommand = true;
    }
  }
}

// Shared Display Logic - the is the best way I could get it work!
#include "displaylogic_imp.h"

