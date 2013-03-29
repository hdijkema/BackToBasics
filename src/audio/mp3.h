#ifndef __MP3__BTB
#define __MP3__BTB

#include <audio/audio.h>
#include <audio/aodev.h>
#include <mpg123.h>
#include <elementals.h>
//#include <semaphore.h>

DECLARE_FIFO(mp3_stream_fifo, memblock_t);

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
  el_bool is_open;
  char* file_or_url;    // This is an intermediate buffer. Don't free at destroy
  long length;
  psem_t* length_set;
  
  // http stream
  mp3_stream_fifo* stream_fifo;
  el_bool continue_streaming;
  psem_t* stream_ready;
  memblock_t* current_block;
  el_bool streaming;
  
} mp3_t;

audio_result_t mp3_new_from_file(audio_worker_t* worker, const char* localpath);
audio_result_t mp3_new_from_url(audio_worker_t* worker, const char* url);

#endif

