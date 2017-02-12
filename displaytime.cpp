/*
 * Main RPI Radio Program
 */
#include <vlc/vlc.h>

#include "rpi-radio.h"
#include "shared/displaylogic.h"
#include "remote.h"

using namespace std;

AbstractDisplay *display;
RemoteControl *remote;

libvlc_instance_t *inst;
libvlc_media_player_t *mp;
libvlc_media_t *m;
int rotarySocket;
int remoteSocket;

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

int readResponse(int secs)
{
  fd_set set;
  struct timeval timeout;
  int rc;
  char buff[100];
  int len = 100;

  FD_ZERO(&set); /* clear the set */
  FD_SET(remoteSocket, &set); /* add file descriptor to the set - slave device */
  FD_SET(rotarySocket, &set); /* add file descriptor of our local rotary device */

  timeout.tv_sec = secs;
  timeout.tv_usec = 0;
  
  int maxFD = remoteSocket;
  if (rotarySocket > maxFD) maxFD = rotarySocket;

  rc = select(maxFD + 1, &set, NULL, NULL, &timeout);
  if (rc == -1) {
    perror("select\n"); /* an error accured */
    return -1;
  }
  
  if (rc == 0) {
    return 0;
  }
  
  if (FD_ISSET(remoteSocket, &set)) { // Slave Response available
    rc = read( remoteSocket, buff, len ); 
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
    return 1;
	}

  // Setup the local rotary control
  AbstractRotaryControl* localRotary = LocalRotaryControl::Factory();
  rotarySocket = localRotary->connect();
  if (rotarySocket < 0) {
    return 1;
  }
  
  // Connect go the slave
  remote = RemoteControl::Factory();
  remoteSocket = remote->connect();
  if (remoteSocket < 0) {
    return 1;
  }
  if (remote->initiate()) {
    return 1;
	}

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
        remote->setMode(AbstractDisplay::DayClock); 
	    }
        
      else if ( (timeinfo->tm_hour >= 6 && timeinfo->tm_hour < 7) ) {
        // Dawn
        display->setMode(AbstractDisplay::DuskClock);
        remote->setMode(AbstractDisplay::DuskClock);
      }
         
      else if ( (timeinfo->tm_hour >= 20 && timeinfo->tm_hour < 22) ) {
        // Dusk
        display->setMode(AbstractDisplay::DuskClock);
        remote->setMode(AbstractDisplay::DuskClock);
      }
         
      else {
        // Night 
        display->setMode(AbstractDisplay::NightClock);
        remote->setMode(AbstractDisplay::Blank);
//        remore->setMode(AbstractDisplay::NightClock);
      }
      display->setClock(timeinfo->tm_hour, timeinfo->tm_min);
      remote->setClock(timeinfo->tm_hour, timeinfo->tm_min);
	  } 
     
    // sleep until the next minute starts (i.e. likely after 60 seconds)
    respRC = readResponse(60-timeinfo->tm_sec); // Also processes and rotary events
	}
	
	// Never get here - but anyway - (todo - could catch a signal)

  // free the media_player
  libvlc_media_player_release(mp);
  libvlc_release(inst);

  delete display;
  delete remote;
      
  // Kill the Rotary Controller
  delete localRotary;
}
