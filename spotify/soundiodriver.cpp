/* */
#include <libspotify/api.h>
#include "spotifywrapper.h"
#include "soundiodriver.h"

int SoundIODriver::init(SessionWrapper* session) {

return 0;
}

int SoundIODriver::done() {
  return 0;
}

void SoundIODriver::get_audio_buffer_stats(sp_session *session, sp_audio_buffer_stats *stats) {
}

int SoundIODriver::music_delivery(sp_session *session, const sp_audioformat *format, const void *frames, int num_frames) {
 return 0;
}

void SoundIODriver::start_playback(sp_session *session) {
}

void SoundIODriver::stop_playback(sp_session *session) {
}
