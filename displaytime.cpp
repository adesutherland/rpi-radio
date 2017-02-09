/*
 * Main RPI Radio Program
 */
#include <unistd.h>
#include <stdlib.h>
#include <termios.h>
#include <fcntl.h>
#include <sys/un.h>
#include <vlc/vlc.h>
#include <getopt.h>

#include "rpi-radio.h"

using namespace std;

AbstractDisplay *display;
libvlc_instance_t *inst;
libvlc_media_player_t *mp;
libvlc_media_t *m;
int rotarySocket;

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
               // Set Volume
               libvlc_audio_set_volume(mp, 100);
               playing = true;
        }
        break;

		  case '^':
//			printf("^\n");
        break;

		  case '>':
//			printf(">\n");
        VolumeControl::increaseVolume();
        break;
			
		  case '<':
//			printf("<\n");
        VolumeControl::decreaseVolume();
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
  FD_SET(rotarySocket, &set); /* add file descriptor of our local rotary device */

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

  if (FD_ISSET(rotarySocket, &set)) { // Rotary events available
    rc = read( rotarySocket, buff, len ); 
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

int sendHourHand(int filedesc, const char* command) {
   write(filedesc, "H", 1);
   write(filedesc, command, strlen(command));
   write(filedesc, "\n", 1);
   return readResponse(filedesc, 3);
}

int sendDark(int filedesc) {
   write(filedesc, "D", 1);
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
  
  // Set Volume
  libvlc_audio_set_volume(mp, 100);

  // Local Display
  display = LocalDisplay::Factory();
  if (display->initiate()) {
			exit(EXIT_FAILURE);
	}

  // Setup the local rotary control
  AbstractRotaryControl* localRotary = LocalRotaryControl::Factory();
  rotarySocket = localRotary->connect();
  if (rotarySocket < 0) {
    return 1;
  }
  
  // Connect go the slave
  int tty_fd = connect("/dev/ttyACM0");

  // Loop to display local time every minute
  int respRC = 0;
  while (true) {
    time ( &rawtime );
    timeinfo = localtime ( &rawtime );
    sprintf(timeText, "%02d:%02d", timeinfo->tm_hour, timeinfo->tm_min);

	  if (respRC == 0 || (60-timeinfo->tm_sec == 0)) { // respRC==0 means readResponse() timed out so the minute is up and we need to update the clock	
         
      if ( (timeinfo->tm_hour >= 7 && timeinfo->tm_hour < 20) ) {
        // Daytime
        display->setMode(AbstractDisplay::DayClock); 
        sendDayTime(tty_fd, timeText);        
	    }
        
      else if ( (timeinfo->tm_hour >= 6 && timeinfo->tm_hour < 7) ) {
        // Dawn
        display->setMode(AbstractDisplay::DuskClock);
        sendNightTime(tty_fd, timeText);
      }
         
      else if ( (timeinfo->tm_hour >= 20 && timeinfo->tm_hour < 22) ) {
        // Dusk
        display->setMode(AbstractDisplay::DuskClock);
        sendNightTime(tty_fd, timeText);
      }
         
      else {
        // Night 
        display->setMode(AbstractDisplay::NightClock);
//        sendHourHand(tty_fd, timeText);
        sendDark(tty_fd);          
	    }
      display->setClock(timeinfo->tm_hour, timeinfo->tm_min);
	  } 
     
    // sleep until the next minute starts (i.e. likely after 60 seconds)
    respRC = readResponse(tty_fd, 60-timeinfo->tm_sec); // Also processes and rotary events
	}
	
	// Never get here - but anyway - (todo - could catch a signal)
  delete display;
  close(tty_fd);
    
  // free the media_player
  libvlc_media_player_release(mp);
  libvlc_release(inst);
  
  // Kill the Rotary Controller
  delete localRotary;
}
