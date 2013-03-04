#ifndef __LIBRARY__HOD
#define __LIBRARY__HOD

#include <playlist/playlist.h>

typedef struct {
  playlist_t* all_tracks;
} library_t;

typedef enum {
  LIBRARY_OK,
  LIBRARY_ERROR
} library_result_t;

library_t* library_new();
void library_destroy(library_t* library);

library_result_t library_load(library_t* library, const char* localpath);
library_result_t library_save(library_t* library, const char* localpath);

playlist_t* library_current_selection(library_t* library);

library_result_t library_filter_none(library_t* library);
library_result_t library_filter_genre(library_t* library, const char);
void playlist_filter_genre(library_t* library, const char* genre);
void playlist_filter_album_artist(library_t* library, const char* album_artist);
void playlist_filter_album_title(library_t* library, const char* album_title);

#endif
