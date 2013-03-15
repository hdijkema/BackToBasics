#include <elementals.h>
#include <audio/ogg.h>
#include <vorbis/codec.h>
#include <vorbis/vorbisfile.h>


/******************************************************************************************
 * ogg worker initialization
 ******************************************************************************************/

static audio_result_t play(void* ogg_info);
static audio_result_t pause(void* ogg_info);
static el_bool can_seek(void* ogg_info);
static audio_result_t seek(void* ogg_info, long position_in_ms);
static audio_result_t guard(void* ogg_info, long position_in_ms);
static long length_in_ms(void* ogg_info);
static audio_result_t load_file(void* ogg_info, const char* file);
static audio_result_t load_url(void* ogg_info, const char* url);
static audio_result_t set_volume(void* ogg_info, double percentage);
static void destroy(void* ogg_info);

static void* player_thread(void* ogg_info);

static void post_event(audio_event_fifo* fifo, audio_state_t state, long position_in_ms)
{
  audio_event_t e = {state, position_in_ms};
  audio_event_fifo_enqueue(fifo, &e);
}
 
#define BUFFER_SIZE(ogg) 4096

static audio_result_t init(audio_worker_t* worker, const char* file_or_url, el_bool file)
{
  //static int ogg_initialized = 0;
  //static int ogg_error = 0;
  
  ogg_t* ogg=(ogg_t*) mc_malloc(sizeof(ogg_t));
  
  worker->can_seek = can_seek;
  worker->play = play;
  worker->pause = pause;
  worker->seek = seek;
  worker->guard = guard;
  worker->destroy = destroy;
  worker->length_in_ms = length_in_ms;
  worker->load_file = load_file;
  worker->load_url = load_url;
  worker->set_volume = set_volume;
  worker->worker_data = (void*) ogg;
  
  ogg->volume_scale = 1.0;
    
  ogg->client_notification = worker->fifo;
  ogg->player_control = audio_event_fifo_new();
  
  //int error;
  
  ogg->is_open = el_false;
  ogg->length  = -1;
  sem_init(&ogg->length_set, 0, 0);
  ogg->buffer = (char*) mc_malloc(BUFFER_SIZE(ogg));
  
  ogg->ao_handle = aodev_new();

  if (file) {
    ogg->can_seek = el_true;
    ogg->is_file = el_true;
    ogg->file_or_url = mc_strdup(file_or_url);
    post_event(ogg->player_control, INTERNAL_CMD_LOAD_FILE, -1); 
  } else { // URL
    ogg->can_seek = el_false;
    ogg->is_file = el_false;
    ogg->file_or_url = mc_strdup(file_or_url);
    post_event(ogg->player_control, INTERNAL_CMD_LOAD_URL, -1);
  }
  
  int thread_id = pthread_create(&ogg->player_thread, NULL, player_thread, ogg);

  // wait until fully loaded (length is set)
  sem_wait(&ogg->length_set);

  return AUDIO_OK;
}

audio_result_t ogg_new_from_file(audio_worker_t* worker, const char* localpath)
{
  return init(worker, localpath, el_true);
}

audio_result_t ogg_new_from_url(audio_worker_t* worker, const char* url)
{
  return init(worker, url, el_false);
}

/******************************************************************************************
 * ogg worker thread
 ******************************************************************************************/

#define STATE_REPORT_THRESHOLD 10   // every 1 hundred of a second

static void adjust_volume(float **pcm, long channels, long samples, void* _minfo)
{
   ogg_t* minfo = (ogg_t*) _minfo;
   float scale = minfo->volume_scale;
   int i, j;
   for(i = 0;i < channels; ++i) {
     for(j = 0;j < samples; ++j) {
       pcm[i][j] *= scale;
     }
   }
}
 
static void* player_thread(void* _minfo) 
{
  ogg_t* minfo = (ogg_t*) _minfo;
  
  log_debug("ogg player started");
  
  long current_position_in_ms = 0;
  long previous_position_in_ms = -1; 
  long guard_position_in_ms = -1;
  el_bool playing = el_false;
  
  post_event(minfo->client_notification, AUDIO_READY, current_position_in_ms);
 
  audio_event_t *event;
  event = audio_event_fifo_dequeue(minfo->player_control);
  while (event->state != INTERNAL_CMD_DESTROY) {
    
    audio_state_t event_state = event->state;
    long event_position = event->position_in_ms;
    audio_event_destroy(event);
    
    switch (event_state) {
      case INTERNAL_CMD_LOAD_FILE: {
        playing = el_false;
        if (minfo->is_open) {
          ov_clear(&minfo->vf);
          fclose(minfo->fh);
          aodev_close(minfo->ao_handle);
        }

        minfo->fh = fopen(minfo->file_or_url, "rb");
        if (minfo->fh != NULL) {
          if (ov_open_callbacks(minfo->fh , &minfo->vf, NULL, 0, OV_CALLBACKS_NOCLOSE) < 0) {
            fclose(minfo->fh);
            post_event(minfo->client_notification, AUDIO_NOT_SUPPORTED, -1);
            minfo->is_open = el_false;
          } else {
            minfo->is_open = el_true;
            minfo->can_seek = ov_seekable(&minfo->vf);
            minfo->length = (long) (ov_time_total(&minfo->vf, -1) * 1000.0);
            sem_post(&minfo->length_set);
            minfo->current_section = 0;
            vorbis_info* vi = ov_info(&minfo->vf, -1);
            aodev_set_format(minfo->ao_handle, 16, vi->rate, vi->channels);
            aodev_set_endian(minfo->ao_handle, AO_FMT_LITTLE);
            aodev_open(minfo->ao_handle);
          }
        } else {
          post_event(minfo->client_notification, AUDIO_IO_ERROR, -1);
          minfo->is_open = el_false;
        }
        
        current_position_in_ms = 0;
        guard_position_in_ms = -1; 
        
        log_debug("Stream initialized");
          
      }
      break;
      case INTERNAL_CMD_LOAD_URL: {
      }
      break;
      case INTERNAL_CMD_SEEK: {
        ov_time_seek_lap(&minfo->vf, ((double) event_position / 1000.0));
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
      case INTERNAL_CMD_GUARD: {
        guard_position_in_ms = event_position;
      }
      break;
      case INTERNAL_CMD_SET_VOLUME: {
        double scale = ((double) event_position) / 1000.0;
        minfo->volume_scale = scale;
        log_debug2("setting volume to %lf", scale);
      }
      break;
      case INTERNAL_CMD_NONE:
      break;
      default:
      break;
    }
    
    if (guard_position_in_ms >= 0 && current_position_in_ms >= guard_position_in_ms) {

      guard_position_in_ms = -1;
      post_event(minfo->client_notification, AUDIO_GUARD_REACHED, current_position_in_ms);
      
    } else if (playing) {
      if (minfo->is_file) {
        int n = ov_read_filter(&minfo->vf, minfo->buffer, BUFFER_SIZE(minfo), 0, 2, 1, &minfo->current_section,
                               adjust_volume, minfo
                              );
        if (n > 0) {
          aodev_play_buffer(minfo->ao_handle, minfo->buffer, n);
          //log_debug("buffer played");
          
          double tm = ov_time_tell(&minfo->vf);
          current_position_in_ms = (long) (tm * 1000.0);
          
          if ((current_position_in_ms - previous_position_in_ms) >= STATE_REPORT_THRESHOLD) {
            post_event(minfo->client_notification, AUDIO_PLAYING, current_position_in_ms);
          }
          previous_position_in_ms = current_position_in_ms;
          
        } else {
          post_event(minfo->client_notification, AUDIO_EOS, current_position_in_ms);
          playing = el_false;
        } 
      } else { // Stream playing
        post_event(minfo->client_notification, AUDIO_STATE_ERROR, -1);
        playing = el_false;
      } 
    
    }
    
    if (playing) {
      if (audio_event_fifo_peek(minfo->player_control) != NULL) {
        event = audio_event_fifo_dequeue(minfo->player_control);
      } else {
        event = (audio_event_t*) mc_malloc(sizeof(audio_event_t));
        event->state = INTERNAL_CMD_NONE;
        event->position_in_ms = -1;
      }
    } else {
      event = audio_event_fifo_dequeue(minfo->player_control);
    }
  }

  // destroy event received
  audio_event_destroy(event);

  // exit thread  
  return NULL;
}

/******************************************************************************************
 * mp3 worker commands
 ******************************************************************************************/

static audio_result_t load_file(void* _minfo, const char* file)
{
  ogg_t* minfo = (ogg_t*) _minfo;
  mc_free(minfo->file_or_url);
  minfo->file_or_url = mc_strdup(file);
  post_event(minfo->player_control, INTERNAL_CMD_LOAD_FILE, -1);
  sem_wait(&minfo->length_set);
  return AUDIO_OK;
}

static audio_result_t load_url(void* _minfo, const char* url)
{
  ogg_t* minfo = (ogg_t*) _minfo;
  minfo->file_or_url = mc_strdup(url);
  post_event(minfo->player_control, INTERNAL_CMD_LOAD_URL, -1);
  sem_wait(&minfo->length_set);
  return AUDIO_OK;
}

static audio_result_t play(void* _minfo) 
{
  ogg_t* minfo = (ogg_t*) _minfo;
  post_event(minfo->player_control, INTERNAL_CMD_PLAY, -1);
  return AUDIO_OK;
}

static audio_result_t pause(void* _minfo)
{
  ogg_t* minfo = (ogg_t*) _minfo;
  post_event(minfo->player_control, INTERNAL_CMD_PAUSE, -1);
  return AUDIO_OK;
}

static audio_result_t set_volume(void* _minfo, double percentage) 
{
  ogg_t* minfo = (ogg_t*) _minfo;
  long vol = (long) (percentage * 10.0);
  post_event(minfo->player_control, INTERNAL_CMD_SET_VOLUME, vol);
  return AUDIO_OK;
}

static el_bool can_seek(void* _minfo)
{
  ogg_t* minfo = (ogg_t*) _minfo;
  return minfo->can_seek;
}

static audio_result_t seek(void* _minfo, long position_in_ms)
{
  ogg_t* minfo = (ogg_t*) _minfo;
  if (minfo->can_seek) {
    post_event(minfo->player_control, INTERNAL_CMD_SEEK, position_in_ms);
    return AUDIO_OK;
  } else {
    return AUDIO_NOT_SUPPORTED;
  }
}

static audio_result_t guard(void* _minfo, long position_in_ms) 
{
  ogg_t* minfo = (ogg_t*) _minfo;
  post_event(minfo->player_control, INTERNAL_CMD_GUARD, position_in_ms);
  return AUDIO_OK;
}

static long length_in_ms(void* _minfo)
{
  ogg_t* minfo = (ogg_t*) _minfo;
  return minfo->length;
}

static void destroy(void* _minfo) 
{
  ogg_t* minfo = (ogg_t*) _minfo;
  post_event(minfo->player_control, INTERNAL_CMD_DESTROY, -1);
  pthread_join(minfo->player_thread, NULL);

  if (minfo->is_open) {
    ov_clear(&minfo->vf);
    fclose(minfo->fh);
    aodev_close(minfo->ao_handle);
    aodev_destroy(minfo->ao_handle);
  }
  
  audio_event_fifo_destroy(minfo->player_control);
  
  mc_free(minfo->file_or_url);
  mc_free(minfo->buffer);
  
  mc_free(minfo);
}

