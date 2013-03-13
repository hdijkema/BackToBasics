#ifndef __FLAC__BTB
#define __FLAC__BTB

#include <audio/audio.h>
#include <audio/aodev.h>

#include <FLAC/stream_decoder.h>

#include <elementals.h>

typedef struct {
  ao_t* ao_handle;
  
  FLAC__StreamDecoder *vf;
  
  audio_event_fifo* client_notification;
  audio_event_fifo* player_control;
  
  el_bool is_open;
  long length;
  sem_t length_set;
  
  el_bool can_seek;
  el_bool is_file;
  char* file_or_url;
  int current_section;
  
  double volume_fraction;
  
  pthread_t player_thread;
  
  int bits;
  int channels;
  int rate;
  unsigned long total_samples;
  unsigned long current_sample;
  double total_time_in_s;
  double current_time_in_s;
  
} flac_t;

audio_result_t flac_new_from_file(audio_worker_t* worker, const char* localpath);
audio_result_t flac_new_from_url(audio_worker_t* worker, const char* url);


#endif
