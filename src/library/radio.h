#ifndef __RADIO__HOD
#define __RADIO__HOD

#include <elementals.h>
#include <time.h>
#include <audio/ripstream.h>
#include <library/library.h>

/*************************************************************************
 * Radio  
 *************************************************************************/

typedef struct {
  char* url;
  char* name;
  char* webpage_url;
  el_bool is_recording;
  rip_stream_t* rip;
  time_t from_time;
} radio_t;

radio_t* radio_new(const char* url, const char* name, const char* webpage_url);
void radio_destroy(radio_t* radio);
radio_t* radio_copy(radio_t* r);

const char* radio_stream_url(radio_t* radio);
const char* radio_name(radio_t* radio);
const char* radio_webpage_url(radio_t* radio);
el_bool radio_has_webpage(radio_t* radio);

void radio_set_stream_url(radio_t* radio, const char* url);
void radio_set_name(radio_t* radio, const char* name);
void radio_set_webpage_url(radio_t* radio, const char* url);

void radio_start_recording(radio_t* radio, const char* location);
void radio_stop_recording(radio_t* radio);
el_bool radio_is_recording(radio_t* radio);


/*************************************************************************
 * Radio library  
 *************************************************************************/

DECLARE_EL_ARRAY(radio_array, radio_t);

typedef struct {
  radio_array stations;
  char* recordings_location;
  library_t* radio_lib;
} radio_library_t;


radio_library_t* radio_library_new(const char* recordings_loc);
void radio_library_destroy(radio_library_t* lib);

const char* radio_library_rec_location(radio_library_t* lib);
void radio_library_set_rec_location(radio_library_t* lib, const char* newloc);
library_t* radio_library_lib(radio_library_t* lib);

void radio_library_load(radio_library_t* lib, const char* filename);
void radio_library_save(radio_library_t* lib, const char* filename);

radio_t* radio_library_station(radio_library_t* lib, int index);
radio_t* radio_library_get(radio_library_t* lib, int index);
int radio_library_count(radio_library_t* lib);

void radio_library_clear(radio_library_t* lib);
void radio_library_insert(radio_library_t* lib, int index, radio_t* radio);
void radio_library_append(radio_library_t* lib, radio_t* radio);
void radio_library_delete(radio_library_t* lib, int index);

#endif
