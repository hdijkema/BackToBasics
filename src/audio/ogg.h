#ifndef __OGG__BTB
#define __OGG__BTB

#include <audio/audio.h>
#include <audio/aodev.h>

#include <vorbis/codec.h>
#include <vorbis/vorbisfile.h>

#include <elementals.h>
#include <stdio.h>

typedef struct {
  ao_t* ao_handle;
  
  OggVorbis_File vf;
  FILE* fh;
  
  audio_event_fifo* client_notification;
  audio_event_fifo* player_control;
  
  el_bool is_open;
  long length;
  sem_t length_set;
  
  el_bool can_seek;
  el_bool is_file;
  char* file_or_url;
  int current_section;
  
  pthread_t player_thread;
  
  double volume_scale;
  
  char* buffer;
  
} ogg_t;

audio_result_t ogg_new_from_file(audio_worker_t* worker, const char* localpath);
audio_result_t ogg_new_from_url(audio_worker_t* worker, const char* url);

#endif

