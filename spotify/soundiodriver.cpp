/* */
#include <iostream>
#include <cstring>
#include <cstdlib>
#include <sstream>
#include <pthread.h>
#include <libspotify/api.h>
#include "spotifywrapper.h"
#include <soundio/soundio.h>
#include "soundiodriver.h"

using namespace std;

enum SoundIoFormat SoundIODriver::prioritized_formats[] = {
    SoundIoFormatS16LE,
    SoundIoFormatInvalid,
};

int SoundIODriver::prioritized_sample_rates[] = {
//    48000,
    44100,
//    96000,
    0,
};

SoundIODriver* SoundIODriver::singleton = NULL;

SoundIODriver* SoundIODriver::SingletonDriverFactory() {
  if (singleton) {
    return singleton;
  }
  else return new SoundIODriver();
}

SoundIODriver::SoundIODriver() {
  singleton = this;
  started = false;
  paused = false;
  underflowCount = 0;
  soundio = NULL;
  outstream = NULL;
  out_device = NULL;
  ring_buffer = NULL; 
}

SoundIODriver::~SoundIODriver() {
  done();
  singleton = NULL;
}

void SoundIODriver::static_write_callback(struct SoundIoOutStream *outstream, int frame_count_min, int frame_count_max) {
  if (singleton) singleton->write_callback(outstream, frame_count_min, frame_count_max);
}

void SoundIODriver::static_underflow_callback(struct SoundIoOutStream *outstream) {
  if (singleton) singleton->underflow_callback(outstream);
}

int SoundIODriver::init() {  
  int err;
  double latency = 0.5; // seconds

  soundio = soundio_create();

  if (!soundio) {
    cerr << "SoundIO Driver: Out of memory" << endl;
    done();
    return -1;
  }
    
  enum SoundIoBackend backend = SoundIoBackendNone;
//  backend = SoundIoBackendDummy;
//  backend = SoundIoBackendAlsa;
  backend = SoundIoBackendPulseAudio;
//  backend = SoundIoBackendJack;
//  backend = SoundIoBackendCoreAudio;
//  backend = SoundIoBackendWasapi;
    
  err = soundio_connect_backend(soundio, backend);
  if (err) {
    cerr << "SoundIO Driver: Error connecting, " << soundio_strerror(err) << endl;
    done();
    return -1;
  }
  
  soundio_flush_events(soundio);
  int out_device_index = soundio_default_output_device_index(soundio);
  if (out_device_index < 0) {
    cerr << "SoundIO Driver: No output device found" << endl;
    done();
    return -1;
  }

  out_device = soundio_get_output_device(soundio, out_device_index);
  if (!out_device) {
    cerr << "SoundIO Driver: Could not get output device, out of memory" << endl;
    done();
    return -1;
  }
  
  cout << "SoundIO Driver: Output device is " << out_device->name << endl;

  int *sample_rate;
  for (sample_rate = prioritized_sample_rates; *sample_rate; sample_rate += 1) {
    if (soundio_device_supports_sample_rate(out_device, *sample_rate))
    {
      break;
    }
  }
  if (!*sample_rate) {
    cerr << "SoundIO Driver: Incompatible sample rate" << endl;
    done();
    return -1;
  }
  
  enum SoundIoFormat *fmt;
  for (fmt = prioritized_formats; *fmt != SoundIoFormatInvalid; fmt += 1) {
    if (soundio_device_supports_format(out_device, *fmt))
    {
      break;
    }
  }
  if (*fmt == SoundIoFormatInvalid) {
    cerr << "SoundIO Driver: Incompatible sample format" << endl;
    done();
    return -1;
  }
  
  outstream = soundio_outstream_create(out_device);
  if (!outstream) {
    cerr << "SoundIO Driver: Out of memory when opening stream" << endl;
    done();
    return -1;
  }
  
  outstream->format = *fmt;
  outstream->sample_rate = *sample_rate;
  outstream->software_latency = latency;
  outstream->write_callback = static_write_callback;
  outstream->underflow_callback = static_underflow_callback;
  outstream->name = "Spotify";
  
  if ((err = soundio_outstream_open(outstream))) {
    cerr << "SoundIO Driver: Unable to open output stream, " << soundio_strerror(err) << endl;
    done();
    return -1;
  }
  
  if (outstream->layout_error) {
    cerr << "SoundIO Driver: Unable to set channel layout, " << soundio_strerror(err) << endl;
    done();
    return -1;
  }

  int capacity = latency * 2 * outstream->sample_rate * outstream->bytes_per_frame;
  ring_buffer = soundio_ring_buffer_create(soundio, capacity);
  if (!ring_buffer) {
    cerr << "SoundIO Driver: Unable to create ring buffer, out of memory" << endl;
    done();
    return -1;
  }  
  char *buf = soundio_ring_buffer_write_ptr(ring_buffer);
  
  memset(buf, 0, capacity / 2); // Half-fill it
  soundio_ring_buffer_advance_write_ptr(ring_buffer, capacity / 2);

  return 0;
}

int SoundIODriver::done() {
  if (outstream) {
    soundio_outstream_destroy(outstream);
    outstream = NULL;
  }
  if (out_device) {
    soundio_device_unref(out_device);
    out_device = NULL;
  }
  if (soundio) {
    soundio_destroy(soundio);
    soundio = NULL;
  }
  return 0;
}

void SoundIODriver::get_audio_buffer_stats(sp_session *session, sp_audio_buffer_stats *stats) {
  if (outstream && ring_buffer) {
    int fill_bytes = soundio_ring_buffer_fill_count(ring_buffer);
    stats->samples = fill_bytes / outstream->bytes_per_sample;
  } 
  else {
    stats->samples = 0;
  }
  stats->stutter = underflowCount;
  underflowCount = 0;
}

int SoundIODriver::music_delivery(sp_session *session, const sp_audioformat *format, const void *frames, int num_frames) {
  if (!started) {
    cerr << "SoundIODriver: Music delivered before play started" << endl;
  }
  else if (paused) {
    cerr << "SoundIODriver: music delivered while paused" << endl;
  }
  
  // TODO Need to reoopen output if the sample rate or number of channels
  // change
    
  if (outstream->sample_rate != format->sample_rate) {
    stringstream message;
    message << "Incompatable sample rate. Output=";
    message << outstream->sample_rate;
    message << ". Input=";
    message << format->sample_rate;
    panic(message.str());
  }

  if (outstream->layout.channel_count != format->channels) {
    panic("Incompatable number of channels");
  }
  
  char *write_ptr = soundio_ring_buffer_write_ptr(ring_buffer);
  int free_bytes = soundio_ring_buffer_free_count(ring_buffer);
  int free_count = free_bytes / outstream->bytes_per_frame;
  int write_frames = min_int(free_count, num_frames);
  
  memcpy(write_ptr, frames, write_frames * outstream->bytes_per_frame);
  soundio_ring_buffer_advance_write_ptr(ring_buffer, write_frames * outstream->bytes_per_frame);

  return write_frames;
}

void SoundIODriver::start_playback(sp_session *session) {
  if (!started) {
    int err;
    if ((err = soundio_outstream_start(outstream))) {
      panic("SoundIO Driver: unable to start output device", soundio_strerror(err));
    }
    started = true;
    paused = false;
  }
  else if (paused) {
    paused = false;
    soundio_outstream_pause(outstream, paused);
  }
}

void SoundIODriver::stop_playback(sp_session *session) {    
  if (!started) return;
  if (paused) return;
  paused = true;
  soundio_outstream_pause(outstream, paused);
}

void SoundIODriver::heartBeat() {
  if (soundio) {
    soundio_flush_events(soundio);
  }
}

void SoundIODriver::write_callback(struct SoundIoOutStream *outstream, int frame_count_min, int frame_count_max) {

  struct SoundIoChannelArea *areas;
  int frame_count;
  int err;
  char *read_ptr = soundio_ring_buffer_read_ptr(ring_buffer);
  int fill_bytes = soundio_ring_buffer_fill_count(ring_buffer);
  int fill_count = fill_bytes / outstream->bytes_per_frame;

  int read_count = min_int(frame_count_max, fill_count);

  int frames_left = read_count;
  while (frames_left > 0) {
    frame_count = frames_left;
    if ((err = soundio_outstream_begin_write(outstream, &areas, &frame_count))) {
      panic("Begin write error", soundio_strerror(err));
    }
    if (frame_count <= 0) {
      break;
    }
    for (int frame = 0; frame < frame_count; frame += 1) {
      for (int ch = 0; ch < outstream->layout.channel_count; ch += 1) {
        memcpy(areas[ch].ptr, read_ptr, outstream->bytes_per_sample);
        areas[ch].ptr += areas[ch].step;
        read_ptr += outstream->bytes_per_sample;
      }
    }
    if ((err = soundio_outstream_end_write(outstream))) {
      panic("End write error: %s", soundio_strerror(err));
    }
    frames_left -= frame_count;
  }
  soundio_ring_buffer_advance_read_ptr(ring_buffer, read_count * outstream->bytes_per_frame);

  if (frame_count_min > read_count) {
    // Ring buffer did not have enough data, fill the rest with zeroes
    underflowCount++;
    cerr << "SoundIO Driver: Underflow" << endl;
    frames_left = frame_count_min - read_count;
    while (frames_left > 0) {
      frame_count = frames_left;
      if ((err = soundio_outstream_begin_write(outstream, &areas, &frame_count))) {
        panic("Begin write error", soundio_strerror(err));
      }
      if (frame_count <= 0) {
        break;
      }
      for (int frame = 0; frame < frame_count; frame += 1) {
        for (int ch = 0; ch < outstream->layout.channel_count; ch += 1) {
          memset(areas[ch].ptr, 0, outstream->bytes_per_sample);
          areas[ch].ptr += areas[ch].step;
        }
      }
      if ((err = soundio_outstream_end_write(outstream))) {
        panic("End write error", soundio_strerror(err));
      }
      frames_left -= frame_count;
    }
  }

}

void SoundIODriver::underflow_callback(struct SoundIoOutStream *outstream) {
  cerr << "SoundIO Driver: Underflow" << endl;
  underflowCount++;
}

void SoundIODriver::panic(string message) {
  cerr << "SoundIO Driver Panic: " << message << endl;
  abort();
}

void SoundIODriver::panic(string message, string detail) {
  cerr << "SoundIO Driver Panic: " << message << ", " << detail << endl;
  abort();
}

int SoundIODriver::min_int(int a, int b) {
  return (a < b) ? a : b;
}
