/* */
#ifndef SOUNDIO_DRIVER_H
#define SOUNDIO_DRIVER_H

#include <soundio/soundio.h>

class SoundIODriver : public BaseAudioDriver {
public:
  static SoundIODriver* SingletonDriverFactory();
  ~SoundIODriver();
  
  int init();
  int done();

  void get_audio_buffer_stats(sp_session *session, sp_audio_buffer_stats *stats);
  int music_delivery(sp_session *session, const sp_audioformat *format, const void *frames, int num_frames);
  void start_playback(sp_session *session);
  void stop_playback(sp_session *session);
  
  void heartBeat();
  
  void write_callback(struct SoundIoOutStream *outstream, int frame_count_min, int frame_count_max);
  void underflow_callback(struct SoundIoOutStream *outstream);
  
  static void static_write_callback(struct SoundIoOutStream *outstream, int frame_count_min, int frame_count_max);
  static void static_underflow_callback(struct SoundIoOutStream *outstream);
  
private:
  static enum SoundIoFormat prioritized_formats[];
  static int prioritized_sample_rates[];
  static SoundIODriver *singleton;

  SoundIODriver();
  
  bool started;
  bool paused;
  int underflowCount;
  struct SoundIo *soundio;
  struct SoundIoOutStream *outstream;
  struct SoundIoDevice *out_device;
  struct SoundIoRingBuffer *ring_buffer; 

  void panic(std::string message);
  void panic(std::string message, std::string detail);
  int min_int(int a, int b);
};

#endif
