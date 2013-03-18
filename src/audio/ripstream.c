#include <libstreamripper.h>
#include <audio/ripstream.h>
#include <elementals.h>


struct __rip_stream__ {
  STREAM_PREFS prefs;
  RIP_MANAGER_INFO* rmi;
  
  char* recordings_location;
  char* stream_name;
  char* stream_url;
  char* last_error;
  int   last_error_code;
  int   bitrate;
  int   refcount;
  rip_stream_enum state;
  pthread_mutex_t mutex;
};

/*********************************************************************************
 *  StreamRipper callback
 *********************************************************************************/

static void status_callback (RIP_MANAGER_INFO* rmi, int message, void *data) 
{
  rip_stream_t *rip=(rip_stream_t *) rip_manager_callback_data(rmi);
  STREAM_PREFS *prefs = rmi->prefs;
  switch(message) {
    case RM_UPDATE: {
        switch (rmi->status) {
          case RM_STATUS_BUFFERING:  
            rip_stream_set_state(rip, RIP_STREAM_BUFFERING);
            break;
          case RM_STATUS_RIPPING:
            if (rmi->track_count < prefs->dropcount) {
              rip_stream_set_state(rip, RIP_STREAM_SKIPPING);
            } else {
              rip_stream_set_state(rip, RIP_STREAM_RECORDING);
            }
            break;
          case RM_STATUS_RECONNECTING:
            rip_stream_set_state(rip, RIP_STREAM_RECONNECTING);
            break;
           
        }
        if (rmi->http_bitrate > 0) {
          rip_stream_set_bitrate(rip, rmi->http_bitrate);
        }
    }
    break;
    case RM_ERROR: {
      ERROR_INFO* err=(ERROR_INFO* ) data;
      rip_stream_set_error(rip, err->error_str, err->error_code);
    }
    break;
    case RM_DONE: rip_stream_set_state(rip, RIP_STREAM_DONE);
    break;
    case RM_STARTED: rip_stream_set_state(rip, RIP_STREAM_STARTED);
    break;
    case RM_NEW_TRACK: rip_stream_set_state(rip, RIP_STREAM_NEW_TRACK);
    break;
  }
}

/*********************************************************************************
 * Implementation of the RipStream
 *********************************************************************************/

static int ripper_count = 0;

rip_stream_t* rip_stream_new(void)
{
  rip_stream_t* rip = (rip_stream_t*) mc_malloc(sizeof(rip_stream_t));
  rip->rmi = NULL;
  rip->recordings_location = mc_strdup("");
  rip->stream_name = mc_strdup("");
  rip->stream_url = mc_strdup("");
  rip->last_error = mc_strdup("");
  rip->last_error_code = 0;
  rip->bitrate = 0;
  rip->state = RIP_STREAM_NIL;
  pthread_mutex_init(&rip->mutex, NULL);
  ripper_count += 1;
  rip->refcount = 1;
  log_debug("NEW");
  return rip;
}

rip_stream_t* rip_stream_copy(rip_stream_t* rip0)
{
  pthread_mutex_lock(&rip0->mutex);
  rip0->refcount += 1;
  pthread_mutex_unlock(&rip0->mutex);
  log_debug("COPY");
  return rip0;
}

void rip_stream_destroy(rip_stream_t* rip)
{
  pthread_mutex_lock(&rip->mutex);
  rip->refcount -= 1;
  if (rip->refcount > 0) {
    pthread_mutex_unlock(&rip->mutex);
    return;
  } else {
    if (rip->rmi != NULL) {
      rip_manager_stop(rip->rmi);
    }
    pthread_mutex_unlock(&rip->mutex);
    pthread_mutex_destroy(&rip->mutex);
    
    ripper_count -= 1;
    if (ripper_count == 0) {
      rip_manager_cleanup();
    }
    log_debug("destroy");
    
    mc_free(rip->recordings_location);
    mc_free(rip->stream_name);
    mc_free(rip->stream_url);
    mc_free(rip->last_error);
    
    mc_free(rip);
  }
}

el_bool rip_stream_start_recording(rip_stream_t* rip,
                                    const char* recordings_loc, 
                                    const char* name, 
                                    const char* url)
{
  if (rip_stream_is_recording(rip)) {
    return el_false;
  }
  
  mc_free(rip->recordings_location);
  rip->recordings_location = mc_strdup(recordings_loc);
  mc_free(rip->stream_name);
  rip->stream_name = mc_strdup(name);
  mc_free(rip->stream_url);
  rip->stream_url = mc_strdup(url);
  
  STREAM_PREFS* prefs = &rip->prefs;
 
  strncpy(prefs->url, rip->stream_url, MAX_URL_LEN);
  prefs_load();
  prefs_get_stream_prefs(prefs, prefs->url);
  prefs_save();
 
  char* pattern = hre_concat(rip->stream_name, "-%d");
  OPT_FLAG_SET(prefs->flags, OPT_SINGLE_FILE_OUTPUT, 1);
 
  strncpy(prefs->output_pattern, pattern, SR_MAX_PATH); 
  strncpy(prefs->output_directory, rip->recordings_location, SR_MAX_PATH);
  
  mc_free(pattern);
 
  prefs->timeout = 10;
 
  int rc = rip_manager_start_with_data(&rip->rmi, &rip->prefs, status_callback, (void *) rip);
 
  return rc == SR_SUCCESS;
}

void rip_stream_stop_recording(rip_stream_t* rip)
{
  rip_manager_stop(rip->rmi);
  pthread_mutex_lock(&rip->mutex);
  rip->rmi = NULL;
  pthread_mutex_unlock(&rip->mutex);
  rip_stream_set_state(rip, RIP_STREAM_STOPPED);
}

el_bool rip_stream_is_recording_state(rip_stream_t* rip)
{
  rip_stream_enum st = rip->state;
  return st == RIP_STREAM_RECORDING ||
         st == RIP_STREAM_SKIPPING ||
         st == RIP_STREAM_NEW_TRACK ||
         st == RIP_STREAM_STARTED;
}

el_bool rip_stream_is_buffering(rip_stream_t* rip)
{
  return rip->state == RIP_STREAM_BUFFERING;
}

el_bool rip_stream_is_stopped(rip_stream_t* rip)
{
  return rip->state == RIP_STREAM_NIL ||
         rip->state == RIP_STREAM_DONE ||
         rip->state == RIP_STREAM_STOPPED;
}

el_bool rip_stream_is_connecting(rip_stream_t* rip)
{
  return rip->state == RIP_STREAM_INITIALIZING ||
         rip->state == RIP_STREAM_RECONNECTING;
}

el_bool rip_stream_is_error(rip_stream_t* rip)
{
  return rip->state == RIP_STREAM_ERROR;
}

el_bool rip_stream_is_recording(rip_stream_t* rip)
{
  return rip->rmi != NULL;
}

void rip_stream_set_state(rip_stream_t* rip, rip_stream_enum state)
{
  pthread_mutex_lock(&rip->mutex);
  rip->state = state;
  pthread_mutex_unlock(&rip->mutex);
  // TODO send a fifo event
}

void rip_stream_set_error(rip_stream_t* rip, const char* error, int error_code)
{
  pthread_mutex_lock(&rip->mutex);
  mc_free(rip->last_error);
  rip->last_error = mc_strdup(error);
  rip->last_error_code = error_code;
  pthread_mutex_unlock(&rip->mutex);
}

void rip_stream_set_bitrate(rip_stream_t* rip, int rate) 
{
  pthread_mutex_lock(&rip->mutex);
  rip->bitrate = rate;
  pthread_mutex_unlock(&rip->mutex);
}

char* rip_stream_last_error(rip_stream_t* rip)
{
  pthread_mutex_lock(&rip->mutex);
  char *result = mc_strdup(rip->last_error);
  pthread_mutex_unlock(&rip->mutex);
  return result;
}

int rip_stream_last_error_code(rip_stream_t* rip)
{
  pthread_mutex_lock(&rip->mutex);
  int result = rip->last_error_code;
  pthread_mutex_unlock(&rip->mutex);
  return result;
}

rip_stream_enum rip_stream_state(rip_stream_t* rip)
{
  pthread_mutex_lock(&rip->mutex);
  rip_stream_enum result = rip->state;
  pthread_mutex_unlock(&rip->mutex);
  return result;
}

