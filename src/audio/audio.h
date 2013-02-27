#ifndef __AUDIO__HOD
#define __AUDIO__HOD

#include <elementals.h>

typedef enum {
  AUDIO_OK = 0,
  AUDIO_IO_ERROR = -1,
  AUDIO_DECODER_ERROR = -2,
  AUDIO_NOT_SUPPORTED = -3,
  AUDIO_NOT_IMPLEMENTED = -4,
  AUDIO_CANNOT_INITIALIZE = -5
} audio_result_t;

typedef enum {
  AUDIO_PLAYING,
  AUDIO_SEEKED,
  AUDIO_GARD_REACHED,
  AUDIO_PAUSED,
  AUDIO_EOS,
  AUDIO_READY, 
  INTERNAL_CMD_DESTROY,
  INTERNAL_CMD_PLAY,
  INTERNAL_CMD_SEEK,
  INTERNAL_CMD_PAUSE,
  INTERNAL_CMD_GARD,
  INTERNAL_CMD_NONE
} audio_state_t;

typedef struct {
  audio_state_t state;
  long position_in_ms;
} audio_event_t;

DECLARE_FIFO(audio_event_fifo, audio_event_t);

typedef struct {
  el_bool (*can_seek)(void* w);
  audio_result_t (*play)(void* w);
  audio_result_t (*seek)(void* w,long position_in_ms);
  audio_result_t (*gard)(void* w,long position_in_ms);
  audio_result_t (*pause)(void* w);
  void (*destroy)(void* worker);
  audio_event_fifo *fifo;
  void* worker_data;
} audio_worker_t;

audio_worker_t* media_new_from_file(const char* local_path, audio_result_t* err);
audio_worker_t* media_new_from_url(const char* url, audio_result_t* err);
void media_destroy(audio_worker_t* worker);

audio_result_t media_play(audio_worker_t* worker);
el_bool media_can_seek(audio_worker_t* worker);
audio_result_t media_seek(audio_worker_t* worker, long position_in_ms);
audio_result_t media_gard_position(audio_worker_t* worker, long position_in_ms);
audio_result_t media_pause(audio_worker_t* worker);

el_bool media_peek_event(audio_worker_t *worker);
audio_event_t *media_get_event(audio_worker_t *worker);
void audio_event_destroy(audio_event_t *event);

#endif
