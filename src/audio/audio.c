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
  return audio_event_fifo_peek(worker->fifo);
}

audio_event_t *media_get_event(audio_worker_t *worker)
{
  return audio_event_fifo_dequeue(worker->fifo);
}

void audio_event_destroy(audio_event_t *event)
{
  destroy_event(e);
}

/******************************************************************************************
 * Audio worker implementation
 ******************************************************************************************/

audio_worker_t* media_new_from_file(const char *local_path, audio_callback_t cb, audio_result_t *err) 
{
  file_info_t* info = file_info_new(local_path);
  if (strcasecmp(file_info_ext(info), "mp3") == 0) {
    return mp3_new_from_file(local_path, cb, err);
  } else {
    *err = AUDIO_NOT_SUPPORTED;
    return NULL;
  }
}

audio_worker_t* media_new_from_url(const char* url, audio_callback_t cb, audio_result_t* err) 
{
  *err = AUDIO_NOT_IMPLEMENTED;
  return NULL;
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

audio_result_t media_pause(audio_worker_t* worker)
{
  return worker->pause(worker->worker_data);
}

void media_destroy(audio_worker* worker)
{
  worker->destroy(worker->worker_data);
  mc_free(worker);
}

