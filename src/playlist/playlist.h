#ifndef __PLAYLIST__HOD
#define __PLAYLIST__HOD

#include <elementals.h>
#include <metadata/track.h>

DECLARE_EL_ARRAY(playlist_array, track_t);
typedef struct {
  playlist_array list;
  char* name;
} playlist_t;

playlist_t* playlist_new(const char* name);
void playlist_destroy(playlist_t* pl);

void playlist_append(playlist_t* pl,track_t* t);
void playlist_insert(playlist_t* pl,int index, track_t* t);
void playlist_set(playlist_t* pl,int index, track_t* t);
track_t* playlist_get(playlist_t* pl,int index);

int playlist_count(playlist_t* pl);

long long playlist_tracks_hash(playlist_t* pl);

#endif
