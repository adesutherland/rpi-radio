#include "rpi-radio.h"
#include <vlc/vlc.h>

using namespace std;

class VLCRadioModule : public AbstractModule
{
  public:
    static AbstractModule* Factory() {
      if (!singleton) singleton = new VLCRadioModule();
      return singleton;
    }
    
    int intialiseModule() {
      // load the vlc engine
    
      // NOTE TO SELF
      // If you are running under sudo then it does not know where its config files are
      // Hence the vlcrc below. 
      // BUT YOU ALSO NEED A /etc/asound.conf file else you get an alsa error
      const char *vlcArgs[] = {
//		    "--verbose=2",
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
        return -1;
      }
      libvlc_equalizer_t* eq = libvlc_audio_equalizer_new();
      for (int b=0; b<bands; b++) {
//	    float freq = libvlc_audio_equalizer_get_band_frequency(b);
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

      return 0;
    }

    ~VLCRadioModule() {
      // free the media_player
      libvlc_media_player_release(mp);
      libvlc_release(inst);
    }
    
    void play() {
      // play the media_player
      libvlc_media_player_play(mp);
      // Set Volume
      libvlc_audio_set_volume(mp, 100);
    }
    
    void stop() {
      libvlc_media_player_stop(mp);
    }

  private:
  
    static class initializerClass
    {
      public:
        initializerClass() { 
          addModule("vlcradio", Factory);
        }
    } initializer;
    
    static VLCRadioModule* singleton;
        
    VLCRadioModule() : AbstractModule("vlcradio", "Internet Radio") {
    }
    
    libvlc_instance_t *inst;
    libvlc_media_player_t *mp;
    libvlc_media_t *m;
};

VLCRadioModule::initializerClass VLCRadioModule::initializer;

VLCRadioModule* VLCRadioModule::singleton = NULL;
