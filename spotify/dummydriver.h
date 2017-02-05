/* */
#ifndef DUMMY_DRIVER_H
#define DUMMY_DRIVER_H

class DummyDriver : public BaseAudioDriver {
public:
  static DummyDriver* SingletonDriverFactory();
  ~DummyDriver();
  
  int init();
  int done();

  void get_audio_buffer_stats(sp_session *session, sp_audio_buffer_stats *stats);
  int music_delivery(sp_session *session, const sp_audioformat *format, const void *frames, int num_frames);
  void start_playback(sp_session *session);
  void stop_playback(sp_session *session);
  
  void heartBeat();
private:
  static DummyDriver *singleton;
  DummyDriver();
};

#endif
