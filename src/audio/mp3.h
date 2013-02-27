#ifndef __MP3__BTB
#define __MP3__BTB

#include <audio/audio.h>
#include <ao/ao.h>
#include <mpg123.h>
#include <elementals.h>

typedef struct {
  mpg123_handle* handle;
  long rate;
  int channels;
  int encoding;
  el_bool can_seek;
  ao_t* ao_handle;
  audio_event_fifo* client_notification;
  audio_event_fifo* player_control;
  pthread_t player_thread;
  void* buffer;
  size_t buffer_size;
  el_bool is_file;
} mp3_t;

audio_worker_t* mp3_new_from_file(const char* localpath, audio_result_t *res);
audio_worker_t* mp3_new_from_url(const char* url, audio_result_t *res);

#endif

