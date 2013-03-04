#include <audio/audio.h>
#include <audio/mp3.h>

/******************************************************************************************
 * Event fifo
 ******************************************************************************************/

static inline audio_event_t* copy_event(audio_event_t *e) 
{
  audio_event_t* ee = (audio_event_t*) mc_malloc(sizeof(audio_event_t));
  *ee = *e;
  return ee;
}

static inline void destroy_event(audio_event_t *e) 
{
  mc_free(e);
}

IMPLEMENT_FIFO(audio_event_fifo, audio_event_t, copy_event, destroy_event);

el_bool media_peek_event(audio_worker_t *worker) 
{
  return (audio_event_fifo_peek(worker->fifo) != NULL);
}

audio_event_t *media_get_event(audio_worker_t *worker)
{
  return audio_event_fifo_dequeue(worker->fifo);
}

void audio_event_destroy(audio_event_t *event)
{
  destroy_event(event);
}

audio_state_t audio_event_state(audio_event_t* event) 
{
  return event->state;
}

long audio_event_ms(audio_event_t* event)
{
  return event->position_in_ms;
}


/******************************************************************************************
 * Audio worker implementation
 ******************************************************************************************/
 
static el_bool can_seek(void* w) { return el_false; }
static audio_result_t play(void* w) { return AUDIO_OK; }
static audio_result_t seek(void* w, long p) { return AUDIO_OK; }
static audio_result_t guard(void* w, long p) { return AUDIO_OK; }
static audio_result_t pause(void* w) { return AUDIO_OK; }
static audio_result_t load_file(void* w, const char* p) { return AUDIO_OK; }
static audio_result_t load_url(void* w, const char* p) { return AUDIO_OK; }
static long length_in_ms(void* w) { return 0; }
static void destroy(void* w) { }
 
audio_worker_t* media_new_from_file(const char *local_path, audio_result_t *err) 
{
  audio_worker_t* worker = media_new();
  *err = media_load_file(worker, local_path);
  return worker;
}

audio_worker_t* media_new_from_url(const char* url,  audio_result_t* err) 
{
  *err = AUDIO_NOT_IMPLEMENTED;
  return NULL;
}

audio_worker_t* media_new(void)
{
  audio_worker_t* worker = (audio_worker_t*) mc_malloc(sizeof(audio_worker_t));
  worker->fifo = audio_event_fifo_new();
  worker->worker_data = NULL;

  worker->can_seek = can_seek;
  worker->play = play;
  worker->pause = pause;
  worker->seek = seek;
  worker->guard = guard;
  worker->destroy = destroy;
  worker->length_in_ms = length_in_ms;
  worker->load_file = load_file;
  worker->load_url = load_url;
  worker->worker_data = NULL;
  worker->decoder = AUDIO_DECODER_NONE;
  worker->source = AUDIO_SOURCE_NONE;
  return worker;
}

static audio_decoder_t get_decoder(file_info_t* info)
{
  const char* ext = file_info_ext(info);
  if (strcasecmp(ext,"mp3") == 0) {
    return AUDIO_DECODER_MPG123;
  } else if (strcasecmp(ext,"flac") == 0) {
    return AUDIO_DECODER_FLAC;
  } else if (strcasecmp(ext,"ape") == 0) {
    return AUDIO_DECODER_APE;
  } else if (strcasecmp(ext,"ogg") == 0) {
    return AUDIO_DECODER_OGG;
  } else if (strcasecmp(ext,"aac") == 0) {
    return AUDIO_DECODER_AAC;
  } else if (strcasecmp(ext,"m4a") == 0) {
    return AUDIO_DECODER_M4A;
  } else {
    return AUDIO_DECODER_NONE;
  }
}

audio_result_t media_load_file(audio_worker_t* worker, const char* local_path) {
  file_info_t* info = file_info_new(local_path);
  
  audio_result_t result = AUDIO_OK;
  audio_decoder_t decoder = get_decoder(info);
  
  file_info_destroy(info);
  
  if (decoder == AUDIO_DECODER_NONE) {
    result = AUDIO_NOT_SUPPORTED;
    if (worker->worker_data != NULL) {
      worker->destroy(worker->worker_data);
      worker->worker_data = NULL;
      audio_event_fifo_clear(worker->fifo); 
    }
    worker->decoder = AUDIO_DECODER_NONE;
    worker->source = AUDIO_SOURCE_FILE;
  } else if (decoder == worker->decoder && worker->source == AUDIO_SOURCE_FILE) {
    printf("loading %s\n",local_path);
    worker->load_file(worker->worker_data, local_path);
  } else {
    if (worker->worker_data != NULL) {
      worker->destroy(worker->worker_data);
      worker->worker_data = NULL;
      audio_event_fifo_clear(worker->fifo); 
    }
    worker->decoder = decoder;
    worker->source = AUDIO_SOURCE_FILE;
    switch (decoder) {
      case AUDIO_DECODER_MPG123: 
        result = mp3_new_from_file(worker, local_path);
      break;
      default: 
        result = AUDIO_NOT_SUPPORTED;
        worker->decoder = AUDIO_DECODER_NONE;
      break;
    }
  }
  
  return result;
}

audio_result_t media_load_url(audio_worker_t* worker, const char* url) {
  if (worker->worker_data != NULL) {
    worker->destroy(worker->worker_data);
    worker->worker_data = NULL;
    audio_event_fifo_clear(worker->fifo);
  }
  
  return AUDIO_NOT_IMPLEMENTED;
}

audio_result_t media_play(audio_worker_t* worker)
{
  return worker->play(worker->worker_data);
}

el_bool media_can_seek(audio_worker_t* worker) 
{
  return worker->can_seek(worker->worker_data);
}

audio_result_t media_seek(audio_worker_t* worker, long position_in_ms)
{
  return worker->seek(worker->worker_data, position_in_ms);
}

audio_result_t media_guard(audio_worker_t* worker, long position_in_ms)
{
  return worker->guard(worker->worker_data, position_in_ms);
}

audio_result_t media_pause(audio_worker_t* worker)
{
  return worker->pause(worker->worker_data);
}

long media_length_in_ms(audio_worker_t* worker)
{
  return worker->length_in_ms(worker->worker_data);
}

void media_destroy(audio_worker_t* worker)
{
  worker->destroy(worker->worker_data);
  audio_event_fifo_destroy(worker->fifo);
  mc_free(worker);
}

audio_source_t audio_source(audio_worker_t* worker)
{
  return worker->source;
}
