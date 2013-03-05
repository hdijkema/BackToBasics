#ifndef __LIBRARY__HOD
#define __LIBRARY__HOD

#include <playlist/playlist.h>
#include <elementals.h>

typedef struct {
  long current_id;
  playlist_t* all_tracks;
  char* filter_genre;
  char* filter_album_artist;
  char* filter_album_title;
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

void library_filter_none(library_t* library);
void library_filter_genre(library_t* library, const char* genre);
void library_filter_album_artist(library_t* library, const char* album_artist);
void library_filter_album_title(library_t* library, const char* album_title);

void scan_library(library_t* library, const char* path, void (*cb)(int c, int tot));

#endif

