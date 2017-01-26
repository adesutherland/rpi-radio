/* */
#include <iostream>

#include <libspotify/api.h>
#include "spotifywrapper.h"
#include "dummydriver.h"

using namespace std;

int DummyDriver::init(SessionWrapper* session) {
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
}

int DummyDriver::music_delivery(sp_session *session, const sp_audioformat *format, const void *frames, int num_frames) {
 
 return num_frames;
}

void DummyDriver::start_playback(sp_session *session) {
  cout << "Dummy Audio Playback Started" << endl;
}

void DummyDriver::stop_playback(sp_session *session) {
  cout << "Dummy Audio Playback Stopped" << endl;
}
