/* */
#include <iostream>

#include <libspotify/api.h>
#include "spotifywrapper.h"
#include "dummydriver.h"

using namespace std;

DummyDriver* DummyDriver::singleton = NULL;

DummyDriver* DummyDriver::SingletonDriverFactory() {
  if (singleton) {
    return singleton;
  }
  else return new DummyDriver();
}

DummyDriver::DummyDriver() {
  singleton = this;
}

DummyDriver::~DummyDriver() {
  done();
  singleton = NULL;
}

int DummyDriver::init() {
  cout << "Dummy Audio Driver Loaded" << endl;
  return 0;
}

int DummyDriver::done() {
  cout << "Dummy Audio Driver Unloaded" << endl;
  return 0;
}

void DummyDriver::get_audio_buffer_stats(sp_session *session, sp_audio_buffer_stats *stats) {
  stats->samples = 0;
  stats->stutter = 0;
  static bool done = false;
  if (!done) {
    cout << "Dummy Audio - First get_audio_buffer_stats()" << endl;
    done = true;
  }

}

int DummyDriver::music_delivery(sp_session *session, const sp_audioformat *format, const void *frames, int num_frames) {
  static bool done = false;
  if (!done) {
    cout << "Dummy Audio - First music_delivery()" << endl;
    done = true;
  }
 return num_frames;
}

void DummyDriver::start_playback(sp_session *session) {
  cout << "Dummy Audio - Playback Started" << endl;
}

void DummyDriver::stop_playback(sp_session *session) {
  cout << "Dummy Audio - Playback Stopped" << endl;
}

void DummyDriver::heartBeat() {
  static bool done = false;
  if (!done) {
    cout << "Dummy Audio - First heartbeat()" << endl;
    done = true;
  }
}
