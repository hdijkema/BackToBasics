#ifndef __AUDIO__HOD
#define __AUDIO__HOD

#include <elementals.h>

typedef enum {
  AUDIO_DECODER_NONE,
  AUDIO_DECODER_MPG123,
  AUDIO_DECODER_FLAC,
  AUDIO_DECODER_APE,
  AUDIO_DECODER_OGG,
  AUDIO_DECODER_AAC,
  AUDIO_DECODER_M4A
} audio_decoder_t;

typedef enum {
  AUDIO_SOURCE_NONE,
  AUDIO_SOURCE_FILE,
  AUDIO_SOURCE_STREAM
} audio_source_t;

typedef enum {
  AUDIO_OK = 0,
  AUDIO_IO_ERROR = -1,
  AUDIO_DECODER_ERROR = -2,
  AUDIO_NOT_SUPPORTED = -3,
  AUDIO_NOT_IMPLEMENTED = -4,
  AUDIO_CANNOT_INITIALIZE = -5
} audio_result_t;

typedef enum {
  AUDIO_NONE = 0,
  AUDIO_PLAYING,
  AUDIO_SEEKED,
  AUDIO_GUARD_REACHED,
  AUDIO_PAUSED,
  AUDIO_EOS,
  AUDIO_READY, 
  AUDIO_STATE_ERROR,
  AUDIO_LENGTH,
  AUDIO_BUFFERING, 

  INTERNAL_CMD_DESTROY,
  INTERNAL_CMD_PLAY,
  INTERNAL_CMD_SEEK,
  INTERNAL_CMD_PAUSE,
  INTERNAL_CMD_GUARD,
  INTERNAL_CMD_LOAD_FILE,
  INTERNAL_CMD_LOAD_URL,
  INTERNAL_CMD_SET_VOLUME,
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
  audio_result_t (*guard)(void* w,long position_in_ms);
  audio_result_t (*pause)(void* w);
  audio_result_t (*load_file)(void* w, const char* local_path);
  audio_result_t (*load_url)(void* w, const char* url);
  audio_result_t (*set_volume)(void* w, double percentage);
  long (*length_in_ms)(void* w);
  void (*destroy)(void* worker_data);
  audio_event_fifo *fifo;
  audio_decoder_t decoder;
  audio_source_t source;
  void* worker_data;
} audio_worker_t;

audio_worker_t* media_new_from_file(const char* local_path, audio_result_t* err);
audio_worker_t* media_new_from_url(const char* url, audio_result_t* err);

audio_worker_t* media_new();
void media_destroy(audio_worker_t* worker);

audio_result_t media_load_file(audio_worker_t* worker, const char* local_path);
audio_result_t media_load_url(audio_worker_t* worker, const char* url);

audio_result_t media_play(audio_worker_t* worker);
el_bool media_can_seek(audio_worker_t* worker);
audio_result_t media_seek(audio_worker_t* worker, long position_in_ms);
audio_result_t media_guard(audio_worker_t* worker, long position_in_ms);
audio_result_t media_pause(audio_worker_t* worker);

// 0 - 100%
audio_result_t media_set_volume(audio_worker_t* worker, double percentage);

long media_length_in_ms(audio_worker_t* worker);

el_bool media_peek_event(audio_worker_t *worker);
audio_event_t *media_get_event(audio_worker_t *worker);
audio_event_t *media_get_event_when_available(audio_worker_t *worker, int timeout_in_ms);
void audio_event_destroy(audio_event_t *event);
audio_state_t audio_event_state(audio_event_t* event);
long audio_event_ms(audio_event_t* event);

audio_source_t audio_source(audio_worker_t* worker);

const char* audio_event_name(audio_state_t s);

#endif
