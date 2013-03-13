#include <library/radio.h>
#include <elementals.h>
#include <time.h>
#include <util/config.h>

/*************************************************************************
 * Radio implementation
 *************************************************************************/
 
radio_t* radio_new(const char* url, const char* name, const char* webpage_url)
{
  radio_t* r = (radio_t*) mc_malloc(sizeof(radio_t));
  r->url = mc_strdup(url);
  r->name = mc_strdup(name);
  r->webpage_url = (webpage_url == NULL) ? NULL : mc_strdup(webpage_url);
  r->is_recording = el_false;
  r->from_time = time(NULL);
  return r;
}

void radio_destroy(radio_t* radio)
{
  if (radio_is_recording(radio)) {
    radio_stop_recording(radio);
  }
  mc_free(radio->webpage_url);
  mc_free(radio->name);
  mc_free(radio->url);
  mc_free(radio);
}

radio_t* radio_copy(radio_t* r)
{
  radio_t* rr =radio_new(r->url, r->name, r->webpage_url);
  rr->is_recording = r->is_recording;
  rr->from_time = r->from_time;
  return rr;
}

const char* radio_stream_url(radio_t* radio)
{
  return radio->url;
}

void radio_set_stream_url(radio_t* radio, const char* url)
{
  mc_free(radio->url);
  radio->url = mc_strdup(url);
}

const char* radio_name(radio_t* radio)
{
  return radio->name;
}

const char* radio_webpage_url(radio_t* radio)
{
  return radio->webpage_url;
}

el_bool radio_has_webpage(radio_t* radio)
{
  return radio->webpage_url != NULL;
}

void radio_start_recording(radio_t* radio, const char* location)
{
  radio->is_recording = el_true;
  radio->from_time = time(NULL);
  // use libstreamripper
}

void radio_stop_recording(radio_t* radio)
{
  radio->is_recording = el_false;
  // stop streamripper
}

el_bool radio_is_recording(radio_t* radio)
{
  return radio->is_recording;
}


/*************************************************************************
 * Radio library implementation 
 *************************************************************************/

static radio_t* copy(radio_t* r) 
{
  return radio_copy(r);
}

static void destroy(radio_t* r)
{
  radio_destroy(r);
}
 
IMPLEMENT_EL_ARRAY(radio_array, radio_t, copy, destroy);
 
 
radio_library_t* radio_library_new(const char* recordings_location)
{
  radio_library_t* lib = (radio_library_t*) mc_malloc(sizeof(radio_library_t));
  lib->stations = radio_array_new();
  lib->recordings_location = mc_strdup(recordings_location);
  return lib;
}

void radio_library_destroy(radio_library_t* lib)
{
  radio_array_destroy(lib->stations);
  mc_free(lib->recordings_location);
  mc_free(lib);
}

void radio_library_load(radio_library_t* lib, const char* filename)
{ 
  el_config_t* cfg = el_config_new();
  radio_library_clear(lib);
  el_config_load(cfg, filename);
  
  int n = el_config_get_int(cfg, "number_of_stations", 0);
  int i;
  for(i = 0; i < n; ++i) {
    char path[100];
    sprintf(path, "station_%05d", i);
    char station_cfg[1024];
    sprintf(station_cfg, "%s.name", path);
    char* name = el_config_get_string(cfg, station_cfg, "");
    sprintf(station_cfg, "%s.url", path);
    char* url = el_config_get_string(cfg, station_cfg, "");
    sprintf(station_cfg, "%s.web", path);
    char* web = el_config_get_string(cfg, station_cfg, "");
    radio_t* station = radio_new(url, name, web);
    radio_library_append(lib, station);
    radio_destroy(station);
  }
  el_config_destroy(cfg);
}

void radio_library_save(radio_library_t* lib, const char* filename)
{
  el_config_t* cfg = el_config_new();
  int i, n;
  for(i = 0, n = radio_library_count(lib); i < n; ++i) {
    char path[100];
    sprintf(path, "station_%05d", i);
    char station_cfg[1024];
    radio_t* station = radio_library_get(lib, i);
    sprintf(station_cfg, "%s.name", path);
    el_config_set_string(cfg, station_cfg, radio_name(station)); 
    sprintf(station_cfg, "%s.url", path);
    el_config_set_string(cfg, station_cfg, radio_stream_url(station)); 
    sprintf(station_cfg, "%s.web", path);
    el_config_set_string(cfg, station_cfg, radio_webpage_url(station)); 
  }
  el_config_set_int(cfg, "number_of_stations", n);
  
  el_config_save(cfg, filename);
  
  el_config_destroy(cfg);
}

radio_t* radio_library_station(radio_library_t* lib, int index)
{
  return radio_array_get(lib->stations, index);
}

radio_t* radio_library_get(radio_library_t* lib, int index)
{
  return radio_array_get(lib->stations, index);
}


int radio_library_count(radio_library_t* lib)
{
  return radio_array_count(lib->stations);
}

void radio_library_clear(radio_library_t* lib)
{
  radio_array_clear(lib->stations);
}

void radio_library_insert(radio_library_t* lib, int index, radio_t* radio)
{
  radio_array_insert(lib->stations, index, radio);
}

void radio_library_append(radio_library_t* lib, radio_t* radio)
{
  radio_array_append(lib->stations, radio);
}

void radio_library_delete(radio_library_t* lib, int index)
{
  radio_array_delete(lib->stations, index);
}

const char* radio_library_rec_location(radio_library_t* lib) 
{
  return lib->recordings_location;
}

void radio_library_set_rec_location(radio_library_t* lib, const char* newloc)
{
  mc_free(lib->recordings_location);
  lib->recordings_location = mc_strdup(newloc);
}





