#include <elementals.h>
#include <audio/mp3.h>
#include <time.h>
#include <curl/curl.h>
#include <unistd.h>

static void sleep_ms(int ms)
{
  struct timespec tm = { ms / 1000, 1000000 * (ms%1000) };
  nanosleep(&tm, NULL);
}

/******************************************************************************************
 * mp3 http stream fifo
 ******************************************************************************************/

static memblock_t* ms_copy(memblock_t* blk)
{
  return blk;
}

static void ms_destroy(memblock_t* blk)
{
  // does nothing
}
 
IMPLEMENT_FIFO(mp3_stream_fifo, memblock_t, ms_copy, ms_destroy);


/******************************************************************************************
 * mp3 worker initialization
 ******************************************************************************************/

static audio_result_t play(void* mp3_info);
static audio_result_t mp3_pause(void* mp3_info);
static el_bool can_seek(void* mp3_info);
static audio_result_t seek(void* mp3_info, long position_in_ms);
static audio_result_t guard(void* mp3_info, long position_in_ms);
static long length_in_ms(void* mp3_info);
static audio_result_t load_file(void* mp3_info, const char* file);
static audio_result_t load_url(void* mp3_info, const char* url);
static audio_result_t set_volume(void* mp3_info, double percentage);
static void destroy(void* mp3_info);

static void* player_thread(void* mp3_info);

static void post_event(audio_event_fifo* fifo, audio_state_t state, long position_in_ms)
{
  //log_debug2("posting event %d", state);
  audio_event_t e = {state, position_in_ms};
  audio_event_fifo_enqueue(fifo, &e);
}
 
static audio_result_t init(audio_worker_t* worker, const char* file_or_url, el_bool file)
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
    return AUDIO_CANNOT_INITIALIZE;
  }
  
  mp3_t* mp3=(mp3_t*) mc_malloc(sizeof(mp3_t));
  
  worker->can_seek = can_seek;
  worker->play = play;
  worker->pause = mp3_pause;
  worker->seek = seek;
  worker->guard = guard;
  worker->destroy = destroy;
  worker->length_in_ms = length_in_ms;
  worker->load_file = load_file;
  worker->load_url = load_url;
  worker->set_volume = set_volume;
  worker->worker_data = (void*) mp3;
    
  int error;
  mp3->handle = mpg123_new(NULL, &error);
  mp3->buffer_size = mpg123_outblock(mp3->handle);
  mp3->buffer = mc_malloc(mp3->buffer_size * sizeof(char));
  
  mpg123_volume(mp3->handle, 1.0);
  
  mp3->client_notification = worker->fifo;
  mp3->player_control = audio_event_fifo_new();
  
  // http streams
  mp3->stream_fifo = mp3_stream_fifo_new();
  mp3->continue_streaming = el_true;
  mp3->current_block = NULL;
  sem_init(&mp3->stream_ready, 0, 0);
  mp3->streaming = el_false;
  
  // etc.  
  mp3->is_open = el_false;
  mp3->length  = -1;
  sem_init(&mp3->length_set, 0, 0);
  
  mp3->ao_handle = aodev_new();

  if (file) {
    mp3->can_seek = el_true;
    mp3->is_file = el_true;
    mp3->file_or_url = mc_strdup(file_or_url);
    post_event(mp3->player_control, INTERNAL_CMD_LOAD_FILE, -1); 
  } else { // URL
    mp3->can_seek = el_false;
    mp3->is_file = el_true;   // we check this later 
    mp3->file_or_url = mc_strdup(file_or_url);
    post_event(mp3->player_control, INTERNAL_CMD_LOAD_URL, -1);
  }
  
  int thread_id = pthread_create(&mp3->player_thread, NULL, player_thread, mp3);

  // wait until fully loaded (length is set)
  sem_wait(&mp3->length_set);

  return AUDIO_OK;
}

audio_result_t mp3_new_from_file(audio_worker_t* worker, const char* localpath)
{
  return init(worker, localpath, el_true);
}

audio_result_t mp3_new_from_url(audio_worker_t* worker, const char* url)
{
  return init(worker, url, el_false);
}

/******************************************************************************************
 * mp3 worker thread
 ******************************************************************************************/

#define STATE_REPORT_THRESHOLD 10   // every 1 hundred of a second
#define MAX_STREAM_BUFFER 1000*4096  // 4000K

static size_t write_data(void *buffer, size_t size, size_t nmemb, void *userp)
{
  mp3_t* mp3_info = (mp3_t*) userp;
  
  
  
  if (mp3_info->current_block == NULL) {
    mp3_info->current_block = memblock_new();
  }
  
  if (mp3_info->continue_streaming) {
    memblock_write(mp3_info->current_block, buffer, size * nmemb);
    if (memblock_size(mp3_info->current_block) >= 4096) {
      mp3_stream_fifo_enqueue(mp3_info->stream_fifo, mp3_info->current_block);
      while (mp3_info->continue_streaming && 
                mp3_stream_fifo_count(mp3_info->stream_fifo) > (MAX_STREAM_BUFFER / 4096)) {
        log_debug("buffer full, sleeping");
        sleep(1);
      }
      mp3_info->current_block = memblock_new();
    }
    return nmemb;
  } else {
    // stop streaming
    memblock_destroy(mp3_info->current_block);
    mp3_info->current_block = NULL;
    // clear the fifo
    while (mp3_stream_fifo_peek(mp3_info->stream_fifo) != NULL) {
      memblock_t* b = mp3_stream_fifo_dequeue(mp3_info->stream_fifo);
      memblock_destroy(b);
    }
    
    return 0;
  }
  
}

void* stream_thread(void* data) 
{
  mp3_t* mp3_info = (mp3_t*) data;
  
  CURL *curl;
  mp3_info->streaming = el_true;
  
  curl = curl_easy_init();
  curl_easy_setopt(curl, CURLOPT_URL, mp3_info->file_or_url);
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, mp3_info);
  curl_easy_perform(curl); /* ignores error */ 
  curl_easy_cleanup(curl);
  
  sem_post(&mp3_info->stream_ready);
  mp3_info->streaming = el_false;
  
  return NULL;
}
 
void* player_thread(void* _mp3_info) 
{
  log_debug("player thread started");
  
  mp3_t* mp3_info = (mp3_t* ) _mp3_info;
  long current_position_in_ms = 0;
  long previous_position_in_ms = -1; 
  long guard_position_in_ms = -1;
  el_bool playing = el_false;
  pthread_t thread_id;
  
  post_event(mp3_info->client_notification, AUDIO_READY, current_position_in_ms);
 
  audio_event_t *event;
  event = audio_event_fifo_dequeue(mp3_info->player_control);
  while (event->state != INTERNAL_CMD_DESTROY) {
    
    //log_debug2("event = %d", event->state);
    
    audio_state_t event_state = event->state;
    long event_position = event->position_in_ms;
    audio_event_destroy(event);
    
    switch (event_state) {
      case INTERNAL_CMD_LOAD_FILE: {
        playing = el_false;
        
        // Stop stream, if playing
        if (!mp3_info->is_file) {
          mp3_info->continue_streaming = el_false;
          sem_wait(&mp3_info->stream_ready);
          mc_free(mp3_info->file_or_url);
        }
        
        if (mp3_info->is_open) {
          mpg123_close(mp3_info->handle);
          aodev_close(mp3_info->ao_handle);
        }
        mpg123_open(mp3_info->handle, mp3_info->file_or_url);
        mc_free(mp3_info->file_or_url);
        mpg123_getformat(mp3_info->handle, &mp3_info->rate, &mp3_info->channels, &mp3_info->encoding);
        mp3_info->buffer_size = mpg123_outblock(mp3_info->handle);
        mc_free(mp3_info->buffer);
        mp3_info->buffer = mc_malloc(mp3_info->buffer_size * sizeof(char) );
        aodev_set_format(mp3_info->ao_handle, mpg123_encsize(mp3_info->encoding) * 8, mp3_info->rate, mp3_info->channels);
        aodev_open(mp3_info->ao_handle);
        mp3_info->is_open = el_true;
        mp3_info->is_file = el_true;
        current_position_in_ms = 0;
        guard_position_in_ms = -1; 
        {
          off_t l = mpg123_length(mp3_info->handle);
          if (l == MPG123_ERR) {
            mp3_info->length = -1; 
          } else {
            mp3_info->length = (l * 1000) / mp3_info->rate;
          }
          sem_post(&mp3_info->length_set);
        }
      }
      break;
      case INTERNAL_CMD_LOAD_URL: {
        playing = el_false;
        log_debug("loading url");
        // Wait for feeding streams to end
        if (!mp3_info->is_file) {
          mp3_info->continue_streaming = el_false;
          sem_wait(&mp3_info->stream_ready);
          mc_free(mp3_info->file_or_url);
        }
        mp3_info->is_file = el_false;
        log_debug("current stream ended");
        
        if (mp3_info->is_open) {
          mpg123_close(mp3_info->handle);
          aodev_close(mp3_info->ao_handle);
          mp3_info->is_open = el_false;
        }
        log_debug("aodev closed");
        
        mpg123_open_feed(mp3_info->handle);
        log_debug("feed opened");
        
        pthread_create(&thread_id, NULL, stream_thread, mp3_info);
        log_debug("stream thread started");
        
        mp3_info->is_open = el_true;
        current_position_in_ms = 0;
        guard_position_in_ms = -1;
        mp3_info->length = 0;
        mp3_info->continue_streaming = el_true;
        
        sem_post(&mp3_info->length_set);
      }
      break;
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
      case INTERNAL_CMD_GUARD: {
        guard_position_in_ms = event_position;
      }
      break;
      case INTERNAL_CMD_SET_VOLUME: {
        double volume = ((double) event_position) / 1000.0;
        mpg123_volume(mp3_info->handle, volume);
      }
      case INTERNAL_CMD_NONE:
      break;
      default:
      break;
    }
    
    //log_debug3("guard = %d, playing = %d", guard_position_in_ms, playing);
    if (guard_position_in_ms >= 0 && current_position_in_ms >= guard_position_in_ms) {

      guard_position_in_ms = -1;
      post_event(mp3_info->client_notification, AUDIO_GUARD_REACHED, current_position_in_ms);
      
    } else if (playing) {

      if (mp3_info->is_file) {  
        size_t bytes;
        int res = mpg123_read(mp3_info->handle, mp3_info->buffer, mp3_info->buffer_size, &bytes);
        if (res == MPG123_OK) {
          aodev_play_buffer(mp3_info->ao_handle, mp3_info->buffer, bytes);
          off_t frame = mpg123_tellframe(mp3_info->handle);
          double time_per_frame = (mpg123_tpf(mp3_info->handle)*1000.0);
          //static int prt = 1;
          //if (prt) { printf("tpf=%.6lf\n",time_per_frame);prt=0; }
          current_position_in_ms = (long) (frame * time_per_frame);    // 1 frame is about 26 milliseconds
          if (previous_position_in_ms == -1) previous_position_in_ms = current_position_in_ms;
          if ((current_position_in_ms - previous_position_in_ms) >= STATE_REPORT_THRESHOLD) {
            post_event(mp3_info->client_notification, AUDIO_PLAYING, current_position_in_ms);
          }
          previous_position_in_ms = current_position_in_ms;
        } else if (res == MPG123_DONE) {
          post_event(mp3_info->client_notification, AUDIO_EOS, current_position_in_ms);
          playing = el_false;
        } else {
          post_event(mp3_info->client_notification, AUDIO_STATE_ERROR, current_position_in_ms);
          playing = el_false;
        }
      } else { // Stream playing
        
        if (mp3_stream_fifo_peek(mp3_info->stream_fifo) != NULL) {
          el_bool go_on = el_true; 
          while (go_on && mp3_stream_fifo_peek(mp3_info->stream_fifo) != NULL) {
            memblock_t* blk = mp3_stream_fifo_dequeue(mp3_info->stream_fifo);
            
            mpg123_feed(mp3_info->handle, (const unsigned char*) memblock_as_str(blk), memblock_size(blk));
            memblock_destroy(blk);
            
            size_t done;
            int err;
            unsigned char *audio;
            off_t frame_offset;
            
            do {
              err = mpg123_decode_frame(mp3_info->handle, &frame_offset, &audio, &done);
              switch(err) {
                case MPG123_NEW_FORMAT:
                  mpg123_getformat(mp3_info->handle, &mp3_info->rate, &mp3_info->channels, &mp3_info->encoding);
                  if (aodev_is_open(mp3_info->ao_handle)) {
                    aodev_close(mp3_info->ao_handle);
                  }
                  aodev_set_format(mp3_info->ao_handle, mpg123_encsize(mp3_info->encoding) * 8, mp3_info->rate, mp3_info->channels);
                  aodev_open(mp3_info->ao_handle);
                break;
                case MPG123_OK:
                  //log_debug2("playing buffer %d", done);
                  aodev_play_buffer(mp3_info->ao_handle, audio, done);
                  off_t frame = mpg123_tellframe(mp3_info->handle);
                  double time_per_frame = (mpg123_tpf(mp3_info->handle)*1000.0);
                  current_position_in_ms = (long) (frame * time_per_frame);    // 1 frame is about 26 milliseconds
                  if (previous_position_in_ms == -1) previous_position_in_ms = current_position_in_ms;
                  if ((current_position_in_ms - previous_position_in_ms) >= STATE_REPORT_THRESHOLD) {
                    post_event(mp3_info->client_notification, AUDIO_PLAYING, current_position_in_ms);
                  }
                  previous_position_in_ms = current_position_in_ms;
                  go_on = el_false;
                  break;
                case MPG123_NEED_MORE:
                  break;
                default:
                  break;
              }
            } while (done > 0);
          }
        } else {
          // no streaming data, prevent race conditions
          // sleep for a small time (50 ms);
          sleep_ms(50);
        }
      } 
    }
    
    if (playing) {
      if (audio_event_fifo_peek(mp3_info->player_control) != NULL) {
        event = audio_event_fifo_dequeue(mp3_info->player_control);
      } else {
        event = (audio_event_t*) mc_malloc(sizeof(audio_event_t));
        event->state = INTERNAL_CMD_NONE;
        event->position_in_ms = -1;
      }
    } else {
      //log_debug("waiting for next event");
      event = audio_event_fifo_dequeue(mp3_info->player_control);
    }
  }

  // destroy event received
  log_debug("destroy event received");
  
  // Kill playing streams
  if (mp3_info->streaming) {
    mp3_info->continue_streaming = el_false;
    sem_wait(&mp3_info->stream_ready);
    mc_free(mp3_info->file_or_url);
  }
  
  audio_event_destroy(event);

  // exit thread  
  return NULL;
}

/******************************************************************************************
 * mp3 worker commands
 ******************************************************************************************/

static audio_result_t load_file(void* _mp3_info, const char* file)
{
  mp3_t* mp3_info = (mp3_t*) _mp3_info;
  mp3_info->file_or_url = mc_strdup(file);
  post_event(mp3_info->player_control, INTERNAL_CMD_LOAD_FILE, -1);
  sem_wait(&mp3_info->length_set);
  return AUDIO_OK;
}

static audio_result_t load_url(void* _mp3_info, const char* url)
{
  mp3_t* mp3_info = (mp3_t*) _mp3_info;
  mp3_info->file_or_url = mc_strdup(url);
  post_event(mp3_info->player_control, INTERNAL_CMD_LOAD_URL, -1);
  sem_wait(&mp3_info->length_set);
  return AUDIO_OK;
}

static audio_result_t play(void* _mp3_info) 
{
  mp3_t* mp3_info = (mp3_t* ) _mp3_info;
  post_event(mp3_info->player_control, INTERNAL_CMD_PLAY, -1);
  return AUDIO_OK;
}

static audio_result_t mp3_pause(void* _mp3_info)
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

static audio_result_t guard(void* _mp3_info, long position_in_ms) 
{
  mp3_t* mp3_info = (mp3_t* ) _mp3_info;
  post_event(mp3_info->player_control, INTERNAL_CMD_GUARD, position_in_ms);
  return AUDIO_OK;
}

static audio_result_t set_volume(void* _mp3_info, double percentage)
{
  mp3_t* mp3_info = (mp3_t* ) _mp3_info;
  long perc_times_10 = (long) (percentage*10.0);
  post_event(mp3_info->player_control, INTERNAL_CMD_SET_VOLUME, perc_times_10);
  return AUDIO_OK;
}

static long length_in_ms(void* _mp3_info)
{
  mp3_t* mp3_info = (mp3_t*) _mp3_info;
  return mp3_info->length;
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
  
  mp3_stream_fifo_destroy(mp3_info->stream_fifo);
  
  mc_free(mp3_info->buffer);
  
  mc_free(mp3_info);
}

