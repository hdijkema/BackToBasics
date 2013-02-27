#include <elementals.h>
#include <audio/mp3.h>

/******************************************************************************************
 * mp3 worker initialization
 ******************************************************************************************/

static audio_result_t play(void* mp3_info);
static audio_result_t pause(void* mp3_info);
static el_bool can_seek(void* mp3_info);
static audio_result_t seek(void* mp3_info, long position_in_ms);
static audio_result_t gard(void* mp3_info, long position_in_ms);
static void destroy(void* mp3_info);

static void* player_thread(void* mp3_info);
 
static audio_worker_t* init(const char* file_or_url, el_bool file, audio_result_t* res)
{
  static int mp3_initialized = 0;
  static int mp3_error = 0;
  
  if (!mp3_initialized) {
    mp3_initialized = 1;
    if (mpg123_init() != MPG123_OK) {
      mp3_error = 1;
    } else {
      atexit(mpg123_exit);
    }
  }

  if (mp3_error) {
    *res = AUDIO_CANNOT_INITIALIZE;
    return NULL;
  }
  
  *res = AUDIO_OK;
  
  audio_worker_t* worker=(audio_worker_t*) mc_malloc(sizeof(audio_worker_t));
  mp3_t* mp3=(mp3_t*) mc_malloc(sizeof(mp3_t));
  
  worker->can_seek = can_seek;
  worker->play = play;
  worker->pause = pause;
  worker->seek = seek;
  worker->gard = gard;
  worker->destroy = destroy;
  worker->fifo = audio_event_fifo_new();
  worker->worker_data = (void*) mp3;
    
  int error;
  mp3->handle = mpg123_new(NULL, &error);
  mp3->buffer_size = mpg123_outblock(mp3->handle);
  mp3->buffer = mc_malloc(mp3->buffer_size * sizeof(char));
  mp3->client_notification = worker->fifo;
  mp3->player_control = audio_event_fifo_new();
  
  mp3->ao_handle = aodev_new();

  if (file) {
    mp3->can_seek = el_true;
    mp3->is_file = el_true;
    mpg123_open(mp3->handle, file_or_url);
    mpg123_getformat(mp3->handle, &mp3->rate, &mp3->channels, &mp3->encoding);
    
    aodev_set_format(mp3->ao_handle, mpg123_encsize(mp3->encoding) * 8, mp3->rate, mp3->channels);
    aodev_open(mp3->ao_handle);
    
  } else { // URL
    mp3->can_seek = el_false;
    mp3->is_file = el_false;
    mpg123_open_feed(mp3->handle);
  }
  
  int thread_id = pthread_create(&mp3->player_thread, NULL, player_thread, mp3);
  
  return worker;
}

audio_worker_t* mp3_new_from_file(const char* localpath, audio_result_t* res)
{
  return init(localpath, el_true, res);
}

audio_worker_t* mp3_new_from_url(const char* url, audio_result_t* res)
{
  return init(url, el_false, res);
}

/******************************************************************************************
 * mp3 worker thread
 ******************************************************************************************/

void post_event(audio_event_fifo* fifo, audio_state_t state, long position_in_ms)
{
  audio_event_t e = {state, position_in_ms};
  audio_event_fifo_enqueue(fifo, &e);
}

void* player_thread(void* _mp3_info) 
{
  mp3_t* mp3_info = (mp3_t* ) _mp3_info;
  long current_position_in_ms = 0;
  long gard_position_in_ms = -1;
  el_bool playing = el_false;
  
  post_event(mp3_info->client_notitication, AUDIO_READY, current_position_in_ms);
 
  audio_event_t *event;
  while((event = audio_event_fifo_dequeue(mp3_info->player_control)) != INTERNAL_CMD_DESTROY) {
    audio_state_t event_state = event->state;
    long event_position = event->position_in_ms;
    audio_event_destroy(event);
    
    switch(event_state) {
      case INTERNAL_CMD_SEEK: {
        off_t pos = mpg123_timeframe(mp3_info->handle, (event_position / 1000.0));
        mpg123_seek_frame(mp3_info->handle, pos, SEEK_SET);
      }
      break;
      case INTERNAL_CMD_PLAY: {
        playing = el_true;
      }
      break;
      case INTERNAL_CMD_PAUSE: {
        playing = el_false;
      }
      break;
      case INTERNAL_CMD_GARD: {
        gard_position_in_ms = event_position;
      }
      break;
      case INTERNAL_CMD_NONE:
      break;
    }
    
    if (gard_position_in_ms >= 0 && current_position_in_ms >= gard_position_in_ms) {
    
      gard_position_in_ms = -1;
      post_event(mp3_info->client_notification, AUDIO_GARD_REACHED, current_position_in_ms);
      
    } else if (playing) {

      if (mp3_info->is_file) {  
        size_t bytes;
        int res = mpg123_read(mp3_info->handle, mp3_info->buffer, mp3_info->buffer_size, &bytes);
        if (res == MPG123_OK) {
          aodev_play_buffer(mp3_info->ao_handle, mp3_info->buffer, bytes);
          off_t frame = mpg123_tellframe(mp3_info->handle);
          current_position_in_ms = frame * 26;    // 1 frame is 26 milliseconds
          post_event(mp3_info->client_notification, AUDIO_PLAYING, current_position_in_ms);
        } else {
          post_event(mp3_info->client_notification, AUDIO_DECODER_ERROR, current_position_in_ms);
        }
      } else { // Stream playing
      } 
    
      post_event(mp3_info->player_control, INTERNAL_CMD_NONE, current_position_in_ms);
    }
    
  }

  // exit thread  
  return NULL;
}

/******************************************************************************************
 * mp3 worker commands
 ******************************************************************************************/

static audio_result_t play(void* _mp3_info) 
{
  mp3_t* mp3_info = (mp3_t* ) _mp3_info;
  post_event(mp3_info->player_control, INTERNAL_CMD_PLAY, -1);
  return AUDIO_OK;
}

static audio_result_t pause(void* _mp3_info)
{
  mp3_t* mp3_info = (mp3_t* ) _mp3_info;
  post_event(mp3_info->player_control, INTERNAL_CMD_PAUSE, -1);
  return AUDIO_OK;
}

static el_bool can_seek(void* _mp3_info)
{
  mp3_t* mp3_info = (mp3_t* ) _mp3_info;
  return mp3_info->can_seek;
}

static audio_result_t seek(void* _mp3_info, long position_in_ms)
{
  mp3_t* mp3_info = (mp3_t* ) _mp3_info;
  if (mp3_info->can_seek) {
    post_event(mp3_info->player_control, INTERNAL_CMD_SEEK, position_in_ms);
    return AUDIO_OK;
  } else {
    return AUDIO_NOT_SUPPORTED;
  }
}

static audio_result_t gard(void* _mp3_info, long position_in_ms) 
{
  mp3_t* mp3_info = (mp3_t* ) _mp3_info;
  post_event(mp3_info->player_control, INTERNAL_CMD_GARD, position_in_ms);
  return AUDIO_OK;
}

static void destroy(void* _mp3_info) 
{
  mp3_t* mp3_info = (mp3_t* ) _mp3_info;
  post_event(mp3_info->player_control, INTERNAL_CMD_DESTROY, -1);
  pthread_join(mp3_info->player_thread, NULL);
  
  aodev_close(mp3_info->ao_handle);
  aodev_destroy(mp3_info->ao_handle);
  
  mpg123_close(mp3_info->handle);
  mpg123_delete(mp3_info->handle);
  
  audio_event_fifo_destroy(mp3_info->player_control);
  
}

