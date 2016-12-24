/*
 * Displays the time on a Monochrome OLEDs based on SSD1306 driver
 * Used the adafruit provided library
 */

#include <fstream>
#include <iostream>

#include <stdio.h>
#include <time.h>
#include <math.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <termios.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/select.h>
#include <string.h>
#include <errno.h>
#include <wiringPi.h>
#include <pthread.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/un.h>

#include "ArduiPi_OLED_lib.h"
#include "Adafruit_GFX.h"
#include "ArduiPi_OLED.h"

#include <vlc/vlc.h>

#include <getopt.h>

// Display type 3 = Adafruit I2C 128x64
#define MYOLED 3

using namespace std;

// Rotary Stuff
#define BOUNCETIME 2 // 2 ms
#define BUTTON_PIN 6 // Button - Purple - GPIO25 - physical 4 - wiringpi 6
#define CLOCK_PIN 4  // Clockwise  - Grey - GPIO23 - physical 1 - wiringpi 4
#define ANTI_PIN 5   // Anti-clockwise - Yellow - GPIO24 - physical 3 - wiringpi 5
pthread_mutex_t valuesMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t debounceConditionMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t debounceThreadCondition = PTHREAD_COND_INITIALIZER;
pthread_t debounceThread;
int rotarySocket[2]; //FDs
int buttonValue = HIGH;
int clockValue = HIGH;
int antiValue = HIGH;

ArduiPi_OLED display;
libvlc_instance_t *inst;
libvlc_media_player_t *mp;
libvlc_media_t *m;

void handleRotary(char* buffer) {
  static bool playing = false;
  
  for (int i=0; buffer[i]; i++) {
	  switch (buffer[i]) {
		  case '.':
//			printf(".\n");
			if (playing) {
               // stop playing
               libvlc_media_player_stop(mp);
               playing = false;
		     }
		     else {
               // play the media_player
               libvlc_media_player_play(mp);
               playing = true;
		     }
			break;

		  case '^':
//			printf("^\n");
			break;

		  case '>':
//			printf(">\n");
			if (playing) {
				int volume = libvlc_audio_get_volume(mp);	
				volume += 5;
				if (volume > 100) volume = 100;
				libvlc_audio_set_volume(mp, volume);
			}
			break;
			
		  case '<':
//			printf("<\n");
			if (playing) {
				int volume = libvlc_audio_get_volume(mp);	
				volume -= 5;
				if (volume < 0) volume = 0;
				libvlc_audio_set_volume(mp, volume);
			}
			break;

		  default:
		    ;
	  }
  }
}

int readResponse(int filedesc, int secs)
{
  fd_set set;
  struct timeval timeout;
  int rc;
  char buff[100];
  int len = 100;

  FD_ZERO(&set); /* clear the set */
  FD_SET(filedesc, &set); /* add file descriptor to the set - slave device */
  FD_SET(rotarySocket[0], &set); /* add file descriptor of our local rotary device */

  timeout.tv_sec = secs;
  timeout.tv_usec = 0;

  rc = select(filedesc + 1, &set, NULL, NULL, &timeout);
  if (rc == -1) {
    perror("select\n"); /* an error accured */
    return -1;
  }
  
  if (rc == 0) {
    return 0;
  }
  
  if (FD_ISSET(filedesc, &set)) { // Slave Response available
    rc = read( filedesc, buff, len ); 
    buff[rc] = 0;
//    printf("slave response: [%s]\n", buff);
    handleRotary(buff);
  }

  if (FD_ISSET(rotarySocket[0], &set)) { // Rotary events available
    rc = read( rotarySocket[0], buff, len ); 
    buff[rc] = 0;
//    printf("master response: [%s]\n", buff);
    handleRotary(buff);
  }

  return 1;
}

int sendCommand(int filedesc, const char* command) {
   write(filedesc, command, strlen(command));
   write(filedesc, "\n", 1);
   return readResponse(filedesc, 3);
}

int sendDayTime(int filedesc, const char* command) {
   write(filedesc, "C", 1);
   write(filedesc, command, strlen(command));
   write(filedesc, "\n", 1);
   return readResponse(filedesc, 3);
}

int sendNightTime(int filedesc, const char* command) {
   write(filedesc, "N", 1);
   write(filedesc, command, strlen(command));
   write(filedesc, "\n", 1);
   return readResponse(filedesc, 3);
}

int connect(const char* device) {
   struct termios tio;
   int tty_fd;

   memset(&tio,0,sizeof(tio));
   tio.c_iflag=0;
   tio.c_oflag=0;
   tio.c_cflag=CS8|CREAD|CLOCAL;
   tio.c_lflag=0;
   tio.c_cc[VMIN]=1;
   tio.c_cc[VTIME]=5;
        
   tty_fd=open(device, O_RDWR | O_NONBLOCK);      
   cfsetospeed(&tio,B19200);
   cfsetispeed(&tio,B19200);

   tcsetattr(tty_fd,TCSANOW,&tio);

   sendCommand(tty_fd, "Q");
   return tty_fd;
}

void ButtonInterrupt() {
  // Record Value (using mutex's of course)	
  pthread_mutex_lock(&valuesMutex);
  buttonValue = digitalRead(BUTTON_PIN);
  pthread_mutex_unlock(&valuesMutex);
  
  // Kick the debounce thread
  pthread_mutex_lock(&debounceConditionMutex);
  pthread_cond_signal(&debounceThreadCondition);
  pthread_mutex_unlock(&debounceConditionMutex);
}

void ClockInterrupt() {
  // Record Value (using mutex's of course)	
  pthread_mutex_lock(&valuesMutex);
  clockValue = digitalRead(CLOCK_PIN);
  pthread_mutex_unlock(&valuesMutex);
  
  // Kick the debounce thread
  pthread_mutex_lock(&debounceConditionMutex);
  pthread_cond_signal(&debounceThreadCondition);
  pthread_mutex_unlock(&debounceConditionMutex);
}

void AntiInterrupt() {
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
  while (true) {
    if (timedOut) { // Before we timed out so are not waiting for debounce noise 
      pthread_cond_wait(&debounceThreadCondition, &debounceConditionMutex); 
      timedOut = false; // We must have got a signal
    }
    else { // We got a value change - need to wait a bit to see if it is just noise
  	  struct timespec timeToWait;
      struct timeval now;
      gettimeofday(&now,NULL);
      timeToWait.tv_sec = now.tv_sec;
      timeToWait.tv_nsec = 1000UL * (now.tv_usec+BOUNCETIME); // tv_nsec is nano, tv_usec is micro {sigh}!	  
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
  
  // Wont get here - but anyway!
  pthread_mutex_unlock( &debounceConditionMutex );
  return 0;
}

int main(int argc, char **argv)
{	
    time_t rawtime;
    struct tm * timeinfo;
    char timeText[6];

    // Media Player
    // load the vlc engine
    
    // NOTE TO SELF
    // If you are running under sudo then it does not know where its config files are
    // Hence the vlcrc below. 
    // BUT YOU ALSO NEED A /etc/asound.conf file else you get an alsa error
    const char *vlcArgs[] = {
//		"--verbose=2",
		"--file-logging",
		"--logfile=/home/pi/vlc-log.txt",
		"--config=/home/pi/.config/vlc/vlcrc"
		};
    
    inst = libvlc_new(sizeof(vlcArgs)/sizeof(vlcArgs[0]), vlcArgs);

    // create a new item - Radio 6 music
    m = libvlc_media_new_location(inst, "http://a.files.bbci.co.uk/media/live/manifesto/audio/simulcast/hls/uk/sbr_high/ak/bbc_6music.m3u8");

    // create a media play playing environment
    mp = libvlc_media_player_new_from_media(m);

    // no need to keep the media now
    libvlc_media_release(m);

    // Equaliser
    /* This code gets a preset - example for future changes
    int bands = libvlc_audio_equalizer_get_band_count();
    int presets = libvlc_audio_equalizer_get_preset_count();
    for (int p=0; p<presets; p++) {
		const char* name = libvlc_audio_equalizer_get_preset_name(p);
		printf("Preset=%s\n", name);
		if (strcmp("Soft rock", name) == 0) {
		  libvlc_equalizer_t* eq = libvlc_audio_equalizer_new_from_preset(p);
          for (int b=0; b<bands; b++) {
            float freq = libvlc_audio_equalizer_get_band_frequency(b);
            float val = libvlc_audio_equalizer_get_amp_at_index	(eq, b);
            printf("Freq=%f Value=%f\n", freq, val);
	      }
	      printf("preamp=%f\n", libvlc_audio_equalizer_get_preamp(eq));
	      libvlc_audio_equalizer_release(eq);
	    }      
	}
	*/
	
	// This code just sets up a hard coded equalser
	float eqVals[] = {8, 8, 2.4, 0, -4, -5.6, -3.2, 0, 2.4, 8.8};
	float eqPreamp = 7;
	int bands = libvlc_audio_equalizer_get_band_count();
	if (bands != 10) {
      perror("Number of bands is not 10 - vlc version change?");
      exit(-1);
	}
    libvlc_equalizer_t* eq = libvlc_audio_equalizer_new();
    for (int b=0; b<bands; b++) {
//	  float freq = libvlc_audio_equalizer_get_band_frequency(b);
      float val = eqVals[b];
      libvlc_audio_equalizer_set_amp_at_index(eq, val, b);
//      printf("Freq=%f Value=%f\n", freq, val);
	}
	libvlc_audio_equalizer_set_preamp(eq, eqPreamp);
//	printf("preamp=%f\n", eqPreamp);
	libvlc_media_player_set_equalizer(mp, eq);
	libvlc_audio_equalizer_release(eq);

    // Local Display
	if (display.oled_is_spi_proto(MYOLED))
	{
		// SPI parameters
		if ( !display.init(OLED_SPI_DC,OLED_SPI_RESET,OLED_SPI_CS, MYOLED) )
			exit(EXIT_FAILURE);
	}
	else
	{
		// I2C parameters
		if ( !display.init(OLED_I2C_RESET,MYOLED) )
			exit(EXIT_FAILURE);
	}

	display.begin();

    // Setup the local rotary control

    // sets up the wiringPi library
    if (wiringPiSetup () < 0) {
       fprintf (stderr, "Unable to setup wiringPi: %s\n", strerror (errno));
       return 1;
    }

    // set pins to generate an interrupt
    pinMode(BUTTON_PIN, INPUT);
    pullUpDnControl(BUTTON_PIN, PUD_UP);
    if ( wiringPiISR (BUTTON_PIN, INT_EDGE_BOTH, &ButtonInterrupt) < 0 ) {
       fprintf (stderr, "Unable to setup Button ISR: %s\n", strerror (errno));
       return 1;
    }
 
    pinMode(CLOCK_PIN, INPUT);
    pullUpDnControl(CLOCK_PIN, PUD_UP);
    if ( wiringPiISR (CLOCK_PIN, INT_EDGE_BOTH, &ClockInterrupt) < 0 ) {
       fprintf (stderr, "Unable to setup Clockwise ISR: %s\n", strerror (errno));
       return 1;
    }
 
    pinMode(ANTI_PIN, INPUT);
    pullUpDnControl(ANTI_PIN, PUD_UP);
    if ( wiringPiISR (ANTI_PIN, INT_EDGE_BOTH, &AntiInterrupt) < 0 ) {
       fprintf (stderr, "Unable to setup Anticlockwise ISR: %s\n", strerror (errno));
       return 1;
    }
    
    // We need a socket to communicate to our thread
    socketpair(PF_LOCAL, SOCK_STREAM, 0, rotarySocket);

    // Thread to read and debounce the rotary control
    pthread_create( &debounceThread, NULL, &readAndDebounceRotor, NULL);

    // Connect go the slave
    int tty_fd = connect("/dev/ttyACM0");

    // Loop to display local time every minute
    int respRC = 0;
    while (true) {
      time ( &rawtime );
      timeinfo = localtime ( &rawtime );
      sprintf(timeText, "%02d:%02d", timeinfo->tm_hour, timeinfo->tm_min);

	  if (respRC == 0 || (60-timeinfo->tm_sec == 0)) { // respRC==0 means readResponse() timed out so the minute is up and we need to update the clock	
	    display.clearDisplay();
//	    display.setBrightness(0);
//	    display.setBrightness(0x80);
	    display.setRotation(2); 
        display.setTextColor(WHITE);
        if ( (timeinfo->tm_hour > 6 && timeinfo->tm_hour < 21) ) {
		  // Daytime
		  display.setBrightness(0x80);
          display.setTextSize(4);  
          display.setCursor(5,18);
          display.print(timeText);
          sendDayTime(tty_fd, timeText);
		}
		else {
		  // Nighttime
		  display.setBrightness(0);
          display.setTextSize(2);  
          display.setCursor(20,50);
          display.print(timeText);
          sendNightTime(tty_fd, timeText);
		}



//        #define CLOCK_X 100
//        #define CLOCK_Y 56
//        #define CLOCK_MRAD 7
//        #define CLOCK_HRAD 4
 //       float angle;
 //       int x;
 //       int y;
        
        // clock ticks
        /*
        for( int z=0; z < 360; z= z + 90 ) {
          angle=((float)z/57.29577951) ; //Convert degrees to radians
          x = (CLOCK_X + (sin(angle)*CLOCK_MRAD));
          y = (CLOCK_Y - (cos(angle)*CLOCK_MRAD));
          display.drawPixel(x, y, WHITE);
        }
        */
//        display.drawCircle(CLOCK_X, CLOCK_Y, CLOCK_MRAD, WHITE);
        
        // display minute hand
        /*
        angle = timeinfo->tm_min * 6 ;
        angle = (angle/57.29577951); // radians  
        x = (CLOCK_X + (sin(angle)*CLOCK_MRAD));
        y = (CLOCK_Y - (cos(angle)*CLOCK_MRAD));
        display.drawLine(CLOCK_X, CLOCK_Y, x, y, WHITE);
        */
        
        // display hour hand
 //       angle = timeinfo->tm_hour * 30 + int((timeinfo->tm_min / 12) * 6 )   ;
 //       angle = (angle/57.29577951) ; // radians  
//        x = (CLOCK_X + (sin(angle)*CLOCK_HRAD));
//        y = (CLOCK_Y - (cos(angle)*CLOCK_HRAD));
//        x = (CLOCK_X + (sin(angle)*CLOCK_MRAD));
//        y = (CLOCK_Y - (cos(angle)*CLOCK_MRAD));
//        display.drawLine(CLOCK_X, CLOCK_Y, x, y, WHITE);

        display.display();

	  } 
     
      // sleep until the next minute starts (i.e. likely after 60 seconds)
      respRC = readResponse(tty_fd, 60-timeinfo->tm_sec); // Also processes and rotary events
	}
	
	// Never get here - but anyway - (todo - could catch a signal)
    display.close();
    close(tty_fd);
    
    // free the media_player
    libvlc_media_player_release(mp);
    libvlc_release(inst);
  
    // Wait for the rotary thread to end
    pthread_join(debounceThread, NULL);
    close(rotarySocket[0]);
    close(rotarySocket[1]);
}



