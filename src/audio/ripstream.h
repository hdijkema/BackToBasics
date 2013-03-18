#ifndef __RIP_STREAM_H
#define __RIP_STREAM_H

#include <elementals.h> 

typedef enum {
  RIP_STREAM_BUFFERING = 10,
  RIP_STREAM_DONE = 99,
  RIP_STREAM_ERROR = -10,
  RIP_STREAM_INITIALIZING = 1,
  RIP_STREAM_NEW_TRACK = 52,
  RIP_STREAM_NIL = 0,
  RIP_STREAM_RECONNECTING = 51,
  RIP_STREAM_RECORDING = 50 ,
  RIP_STREAM_SKIPPING = 80,
  RIP_STREAM_STARTED = 5,
  RIP_STREAM_STOPPED = 98
} rip_stream_enum;

typedef struct __rip_stream__ rip_stream_t;

rip_stream_t* rip_stream_new(void);
void rip_stream_destroy(rip_stream_t* rip);
rip_stream_t* rip_stream_copy(rip_stream_t* rip);

el_bool rip_stream_start_recording(rip_stream_t* rip, 
                                   const char* recordings_loc, 
                                   const char* name, 
                                   const char* url);
void rip_stream_stop_recording(rip_stream_t* rip);
el_bool rip_stream_is_recording(rip_stream_t* rip);

char* rip_stream_last_error(rip_stream_t* rip);
int rip_stream_last_error_code(rip_stream_t* rip);
rip_stream_enum rip_stream_state(rip_stream_t* rip);

el_bool rip_stream_is_recording_state(rip_stream_t* rip);
el_bool rip_stream_is_buffering(rip_stream_t* rip);
el_bool rip_stream_is_stopped(rip_stream_t* rip);
el_bool rip_stream_is_connecting(rip_stream_t* rip);
el_bool rip_stream_is_error(rip_stream_t* rip);

void rip_stream_set_error(rip_stream_t* rip, const char* str, int code);
void rip_stream_set_state(rip_stream_t* rip, rip_stream_enum state);
void rip_stream_set_bitrate(rip_stream_t* rip, int rate);

#endif
