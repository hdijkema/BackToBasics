#include <elementals.h>
#include <audio/flac.h>
#include <vorbis/codec.h>
#include <vorbis/vorbisfile.h>


/******************************************************************************************
 * flac worker initialization
 ******************************************************************************************/

static audio_result_t play(void* flac_info);
static audio_result_t pause(void* flac_info);
static el_bool can_seek(void* flac_info);
static audio_result_t seek(void* flac_info, long position_in_ms);
static audio_result_t guard(void* flac_info, long position_in_ms);
static long length_in_ms(void* flac_info);
static audio_result_t load_file(void* flac_info, const char* file);
static audio_result_t load_url(void* flac_info, const char* url);
static void destroy(void* flac_info);
static audio_result_t set_volume(void* flac_info, double percentage);

static void* player_thread(void* flac_info);

static void post_event(audio_event_fifo* fifo, audio_state_t state, long position_in_ms)
{
  audio_event_t e = {state, position_in_ms};
  audio_event_fifo_enqueue(fifo, &e);
}
 
static audio_result_t init(audio_worker_t* worker, const char* file_or_url, el_bool file)
{
  flac_t* flac=(flac_t*) mc_malloc(sizeof(flac_t));
  
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
  worker->worker_data = (void*) flac;
    
  flac->volume_fraction = 1.0;
  
  flac->client_notification = worker->fifo;
  flac->player_control = audio_event_fifo_new();
  
  flac->is_open = el_false;
  flac->length  = -1;
  //sem_init(&flac->length_set, 0, 0);
  flac->length_set = psem_new(0);
  
  flac->ao_handle = aodev_new();

  if (file) {
    flac->can_seek = el_true;
    flac->is_file = el_true;
    flac->file_or_url = mc_strdup(file_or_url);
    post_event(flac->player_control, INTERNAL_CMD_LOAD_FILE, -1); 
  } else { // URL
    flac->can_seek = el_false;
    flac->is_file = el_false;
    flac->file_or_url = mc_strdup(file_or_url);
    post_event(flac->player_control, INTERNAL_CMD_LOAD_URL, -1);
  }
  
  int thread_id = pthread_create(&flac->player_thread, NULL, player_thread, flac);

  // wait until fully loaded (length is set)
  psem_wait(flac->length_set);

  return AUDIO_OK;
}

audio_result_t flac_new_from_file(audio_worker_t* worker, const char* localpath)
{
  return init(worker, localpath, el_true);
}

audio_result_t flac_new_from_url(audio_worker_t* worker, const char* url)
{
  return init(worker, url, el_false);
}

/******************************************************************************************
 * mp3 worker thread
 ******************************************************************************************/
 
static 
FLAC__StreamDecoderWriteStatus flac_write(const FLAC__StreamDecoder* dec, const FLAC__Frame* frame,
                                          const FLAC__int32* const buf[], void* _minfo)
{
  flac_t* minfo = (flac_t*) _minfo;
  unsigned long samples = frame->header.blocksize;
  int channels = frame->header.channels;
  int bytes = minfo->bits / 8;
#ifdef OSX
  bytes = 2;
#endif
  unsigned long decoded_size = (samples * channels * bytes);
  char* abuf = (char*) mc_malloc(decoded_size);
  unsigned short* buf16 = (unsigned short*) abuf;
  unsigned char* buf8 = (unsigned char*) abuf;
  //double elapsed_in_s;
  
  if (minfo->bits == 8) {
    int i, sample;
    for(sample = 0, i = 0; sample < samples; ++sample) {
      int channel;
      for(channel = 0; channel < channels; ++channel, ++i) {
#ifdef OSX
		    /* macosx libao expects 16 bit samples */
		    buf16[i] = (unsigned short) (buf[channel][sample] << 8);
#else
		    buf8[i] = buf[channel][sample];
#endif
      }
    }
  } else if (minfo->bits == 16) {
    int i, sample;
    for(sample = 0, i = 0; sample < samples; ++sample) {
      int channel;
      for(channel = 0; channel < channels; ++channel, ++i) {
        buf16[i] = (unsigned short) (buf[channel][sample] * minfo->volume_fraction); 
      }
    }
  }
  
  aodev_play_buffer(minfo->ao_handle, abuf, decoded_size);
  
  minfo->current_sample += samples;
  //elapsed_in_s = ((double) samples) / minfo->rate;
  //minfo->current_time_in_s += elapsed_in_s;
  minfo->current_time_in_s = ((double) minfo->current_sample) / ((double) minfo->rate);
  
  log_debug("freeing buf");
  mc_free(abuf);
  
  return FLAC__STREAM_DECODER_WRITE_STATUS_CONTINUE;
}

static
void flac_meta(const FLAC__StreamDecoder *dec, const FLAC__StreamMetadata *meta, void* _minfo)
{
  flac_t* minfo = (flac_t*) _minfo;
  log_debug("meta");
  if (meta->type == FLAC__METADATA_TYPE_STREAMINFO) {
    log_debug("set meta");
    minfo->bits = meta->data.stream_info.bits_per_sample;
    minfo->channels = meta->data.stream_info.channels;
    minfo->rate = meta->data.stream_info.sample_rate;
    minfo->total_samples = (unsigned long) (meta->data.stream_info.total_samples & 0xffffffff);
    minfo->current_sample = 0;
    minfo->current_time_in_s = 0.0;
    minfo->total_time_in_s = (((double) minfo->total_samples) / ((double) (minfo->rate)));
  }
}

static
void flac_error(const FLAC__StreamDecoder *dec, FLAC__StreamDecoderErrorStatus status, void *_minfo)
{
  flac_t* minfo = (flac_t*) minfo;
  post_event(minfo->client_notification, AUDIO_DECODER_ERROR, -1);
}


#define STATE_REPORT_THRESHOLD 10   // every 1 hundred of a second
 
static void* player_thread(void* _minfo) 
{
  log_debug("player thread started");
  
  flac_t* minfo = (flac_t*) _minfo;
  
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
          FLAC__stream_decoder_delete(minfo->vf);
          aodev_close(minfo->ao_handle);
        }
        minfo->vf = FLAC__stream_decoder_new();
        if (minfo->vf != NULL) {
          FLAC__stream_decoder_set_md5_checking(minfo->vf, el_true);
          FLAC__StreamDecoderInitStatus init_status;
          init_status = FLAC__stream_decoder_init_file(minfo->vf, 
                                                       minfo->file_or_url, 
                                                       flac_write, flac_meta, flac_error, 
                                                       minfo);
          if (init_status != FLAC__STREAM_DECODER_INIT_STATUS_OK) {
            FLAC__stream_decoder_delete(minfo->vf);
            post_event(minfo->client_notification, AUDIO_CANNOT_INITIALIZE, -1);
          } else {
            if (!FLAC__stream_decoder_process_until_end_of_metadata(minfo->vf)) {
              FLAC__stream_decoder_delete(minfo->vf);
              post_event(minfo->client_notification, AUDIO_CANNOT_INITIALIZE, -1);
            } else {
              minfo->is_open = el_true;
              minfo->can_seek = el_true;
              psem_post(minfo->length_set);
              
              aodev_set_format(minfo->ao_handle, minfo->bits, minfo->rate, minfo->channels);
              aodev_open(minfo->ao_handle);
            }
          }
            
        } else {
          post_event(minfo->client_notification, AUDIO_CANNOT_INITIALIZE, -1);
          minfo->is_open = el_false;
        }
        
        current_position_in_ms = 0;
        guard_position_in_ms = -1; 
          
      }
      break;
      case INTERNAL_CMD_LOAD_URL: {
      }
      break;
      case INTERNAL_CMD_SEEK: {
        if (minfo->is_open) {
          minfo->current_time_in_s = ((double) event_position) / 1000.0;
          unsigned long frame = (unsigned long) (minfo->current_time_in_s * minfo->rate);
          minfo->current_sample = frame;
          FLAC__stream_decoder_seek_absolute(minfo->vf, minfo->current_sample); 
        }
      }
      break;
      case INTERNAL_CMD_PLAY: {
        if (minfo->is_open) {
          playing = el_true;
        }
      }
      break;
      case INTERNAL_CMD_PAUSE: {
        if (minfo->is_open) {
          playing = el_false;
        }
      }
      break;
      case INTERNAL_CMD_GUARD: {
        if (minfo->is_open) {
          guard_position_in_ms = event_position;
        }
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
        el_bool result = FLAC__stream_decoder_process_single(minfo->vf);
        el_bool ok = FLAC__stream_decoder_get_state(minfo->vf) < FLAC__STREAM_DECODER_END_OF_STREAM;
        
        if (result && ok) {
          // sample has already been played
          current_position_in_ms = (long) (minfo->current_time_in_s * 1000.0);
          
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

static audio_result_t set_volume(void* _minfo, double percentage)
{
  flac_t* minfo = (flac_t*) _minfo;
  if (percentage > 100.0) percentage = 100.0;
  if (percentage < 0.0) percentage = 0.0;
  minfo->volume_fraction = percentage / 100.0;
  return AUDIO_OK;
}
 
static audio_result_t load_file(void* _minfo, const char* file)
{
  flac_t* minfo = (flac_t*) _minfo;
  minfo->file_or_url = mc_strdup(file);
  post_event(minfo->player_control, INTERNAL_CMD_LOAD_FILE, -1);
  psem_wait(minfo->length_set);
  return AUDIO_OK;
}

static audio_result_t load_url(void* _minfo, const char* url)
{
  flac_t* minfo = (flac_t*) _minfo;
  minfo->file_or_url = mc_strdup(url);
  post_event(minfo->player_control, INTERNAL_CMD_LOAD_URL, -1);
  psem_wait(minfo->length_set);
  return AUDIO_OK;
}

static audio_result_t play(void* _minfo) 
{
  flac_t* minfo = (flac_t*) _minfo;
  post_event(minfo->player_control, INTERNAL_CMD_PLAY, -1);
  return AUDIO_OK;
}

static audio_result_t pause(void* _minfo)
{
  flac_t* minfo = (flac_t*) _minfo;
  post_event(minfo->player_control, INTERNAL_CMD_PAUSE, -1);
  return AUDIO_OK;
}

static el_bool can_seek(void* _minfo)
{
  flac_t* minfo = (flac_t*) _minfo;
  return minfo->can_seek;
}

static audio_result_t seek(void* _minfo, long position_in_ms)
{
  flac_t* minfo = (flac_t*) _minfo;
  if (minfo->can_seek) {
    post_event(minfo->player_control, INTERNAL_CMD_SEEK, position_in_ms);
    return AUDIO_OK;
  } else {
    return AUDIO_NOT_SUPPORTED;
  }
}

static audio_result_t guard(void* _minfo, long position_in_ms) 
{
  flac_t* minfo = (flac_t*) _minfo;
  post_event(minfo->player_control, INTERNAL_CMD_GUARD, position_in_ms);
  return AUDIO_OK;
}

static long length_in_ms(void* _minfo)
{
  flac_t* minfo = (flac_t*) _minfo;
  return minfo->length;
}

static void destroy(void* _minfo) 
{
  flac_t* minfo = (flac_t*) _minfo;
  post_event(minfo->player_control, INTERNAL_CMD_DESTROY, -1);
  pthread_join(minfo->player_thread, NULL);

  if (minfo->is_open) {
    FLAC__stream_decoder_delete(minfo->vf);
    aodev_close(minfo->ao_handle);
    aodev_destroy(minfo->ao_handle);
  }
  
  audio_event_fifo_destroy(minfo->player_control);

  psem_destroy(minfo->length_set);  
  mc_free(minfo);
}

