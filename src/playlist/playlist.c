#include <playlist/playlist.h>
#include <library/library.h>
#include <elementals.h>

/*****************************************************************
 * playlist array
 *****************************************************************/

static char* copy(char* track_id)
{
  //return mc_take_over(track_copy(t));
  return mc_strdup(track_id);
}

static void destroy(char* track_id)
{
  mc_free(track_id);
  //track_destroy(t);
  // Do nothing here. 
  // Playlists are just proxies 
}

IMPLEMENT_EL_ARRAY(playlist_array, char, copy, destroy);

/*****************************************************************
 * playlist implementation
 *****************************************************************/
 
playlist_t* playlist_new(library_t* library, const char* name) 
{
  playlist_t* pl = (playlist_t*) mc_malloc(sizeof(playlist_t));
  pl->list = mc_take_over(playlist_array_new());
  pl->name = mc_strdup(name);
  pl->library = library;
  return pl;
}
 
void playlist_destroy(playlist_t* pl)
{
   mc_free(pl->name);
   playlist_array_destroy(pl->list);
   mc_free(pl);
}

void playlist_clear(playlist_t* pl)
{
  playlist_array_clear(pl->list);
}

playlist_t* playlist_copy(playlist_t* pl)
{
  playlist_t* npl = playlist_new(pl->library, playlist_name(pl));
  int i, N; 
  for(i = 0, N = playlist_array_count(pl->list); i < N; ++i) {
    playlist_array_append(npl->list, playlist_array_get(pl->list, i));
  }
  return npl;
}

void playlist_append(playlist_t* pl, track_t* t)
{
  playlist_array_append(pl->list, (char*) track_get_id(t));
}

void playlist_insert(playlist_t* pl, int index, track_t* t)
{
  playlist_array_insert(pl->list, index, (char*) track_get_id(t)); 
}

void playlist_set(playlist_t* pl,int index, track_t* t)
{
  playlist_array_set(pl->list, index, (char*) track_get_id(t));
}
 
const char* playlist_name(playlist_t* pl)
{
  return pl->name;
}

struct __library__* playlist_get_library(playlist_t* pl)
{
  return pl->library;
}
 
track_t* playlist_get(playlist_t* pl,int index)
{
  if (index >= 0 && index < playlist_array_count(pl->list)) { 
    char* id = playlist_array_get(pl->list, index);
    return library_get(pl->library, id);
  } else {
    return NULL;
  }
}

int playlist_count(playlist_t* pl)
{
  return playlist_array_count(pl->list);
}

long long playlist_tracks_hash(playlist_t* pl)
{
  int i, N;
  long long a = 0;
  for(i = 0, N = playlist_array_count(pl->list); i < N; ++i) {
    a += str_crc32(track_get_id(playlist_get(pl, i)));
    a <<= 1;
  }
  a += N;
  return a;
}


// sort on genre, album_title, nr
static int cmp_standard0(track_t* t1, track_t* t2) 
{
  int r;
  if ((r = strcasecmp(track_get_genre(t1), track_get_genre(t2))) < 0) {
    return -1;
  } else if (r == 0) {
    if ((r = strcasecmp(track_get_album_title(t1), track_get_album_title(t2))) < 0) {
      return -1;
    } else if (r == 0) {
      return track_get_nr(t1) - track_get_nr(t2);
    } else {
      return 1;
    }
  } else {
    return 1;
  }
}

static int cmp_standaard(void* lib, char* id1, char* id2)
{
  return cmp_standard0(library_get(((library_t*) lib), id1),
                       library_get(((library_t*) lib), id2));
}

void playlist_sort_standard(playlist_t* pl)
{
  playlist_array_sort1(pl->list, pl->library, cmp_standaard); 
}

