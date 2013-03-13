#ifndef __LIBRARY__HOD
#define __LIBRARY__HOD

#include <playlist/playlist.h>
#include <elementals.h>
#include <gui/scanjob.h>

DECLARE_EL_ARRAY(genre_array, char);
DECLARE_EL_ARRAY(artist_array, char);
DECLARE_EL_ARRAY(album_array, char);
DECLARE_HASH(library_db, track_t);

typedef struct {
  long current_id;
  library_db* tracks_db;
  playlist_t* all_tracks;
  char* filter_genre;
  char* filter_album_artist;
  char* filter_album_title;
  el_bool dirty;
  genre_array genres;
  artist_array artists;
  album_array albums;
  el_bool filter;
  set_t* filtered_artists;
  set_t* filtered_albums;
  set_t* filtered_tracks;
  char* library_path;
  
} library_t;

typedef enum {
  LIBRARY_OK,
  LIBRARY_ERROR
} library_result_t;

library_t* library_new(void);
void library_destroy(library_t* library);
int library_count(library_t* library);

library_result_t library_load(library_t* library, const char* localpath);
library_result_t library_save(library_t* library, const char* localpath);

playlist_t* library_current_selection(library_t* library, const char* name);

library_result_t library_add(library_t* library, track_t* t);
library_result_t library_set(library_t* library, int index, track_t* t);
int library_find_index(library_t* library, track_t* t);

genre_array library_genres(library_t* library);
artist_array library_artists(library_t* library);
album_array library_albums(library_t* library);

set_t* library_filtered_artists(library_t* library);
set_t* library_filtered_albums(library_t* library);
set_t* library_filtered_tracks(library_t* library);

void library_filter_none(library_t* library);
void library_filter_genre(library_t* library, const char* genre);
void library_filter_album_artist(library_t* library, const char* album_artist);
void library_filter_album_title(library_t* library, const char* album_title);

void library_set_basedir(library_t* library, const char* path);
const char* library_get_basedir(library_t* library);

void scan_library(scan_job_t* job, ScanJobCBFunc func, void* library);
void library_sort(library_t* library);

#endif

