#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define OLED_RESET 4
Adafruit_SSD1306 display(OLED_RESET);

#if (SSD1306_LCDHEIGHT != 64)
#error("Height incorrect, please fix Adafruit_SSD1306.h!");
#endif

const byte maxSize = 100;
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
unsigned long debounceDelay = 2;

void setup()   {                
  Serial.begin(19200);

  pinMode (encoderPinA,INPUT);
  digitalWrite(encoderPinA, HIGH);

  pinMode (encoderPinB,INPUT);
  digitalWrite(encoderPinB, HIGH);

  pinMode (buttonPin,INPUT);
  digitalWrite(buttonPin, HIGH);
  
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);  
  display.clearDisplay();
  display.display();
  debounceTime = millis();
}

void loop() {
  
   readCommand();

   if (newCommand) {
      newCommand = false;
      switch (receivedCommand[0]) {
        case 'C':
          display.clearDisplay();
          display.setTextSize(4);
          display.setTextColor(WHITE);
          display.setCursor(5,18);
          display.print(receivedCommand+1);
          display.display();
          Serial.println("K");
          break;
 
        case 'Q': // If the board was still running the bootloader Q would exit and load [this] program
                  // So this is just to make a friendly response - as I am already running
        case 0:   // Empty commands - likely syncing when booting         
          Serial.println("K");
          break;
 
        default:
          Serial.println("E"); // Invalid Command
          // Serial.println(receivedCommand);
      }
   }
}

void readRotar() {
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
  
  if ((millis() - debounceTime) > debounceDelay) {
    if ((encoderPinAState == HIGH) && (a == LOW)) {
      if (b == LOW) {
        Serial.print ("<");
      } else {
        Serial.print (">");
      }
    } 
    encoderPinAState = a;
    encoderPinBState = b;
  
    if ((buttonPinState == HIGH) && (c == LOW)) {
      Serial.print (".");
    } 
    else if ((buttonPinState == LOW) && (c == HIGH)) {
      Serial.print ("^");
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
  
  readRotar();

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
    
    readRotar();
  }
}

