#include <playlist/playlist.h>
#include <elementals.h>

/*****************************************************************
 * playlist array
 *****************************************************************/

static track_t* copy(track_t* t)
{
  //return mc_take_over(track_copy(t));
  return t;
}

static void destroy(track_t* t)
{
  //track_destroy(t);
  // Do nothing here. 
  // Playlists are just proxies 
}

IMPLEMENT_EL_ARRAY(playlist_array, track_t, copy, destroy);

/*****************************************************************
 * playlist implementation
 *****************************************************************/
 
playlist_t* playlist_new(const char* name) 
{
  playlist_t* pl = (playlist_t*) mc_malloc(sizeof(playlist_t));
  pl->list = mc_take_over(playlist_array_new());
  pl->name = mc_strdup(name);
  return pl;
}
 
void playlist_destroy(playlist_t* pl)
{
   mc_free(pl->name);
   playlist_array_destroy(pl->list);
   mc_free(pl);
}

playlist_t* playlist_copy(playlist_t* pl)
{
  playlist_t* npl = playlist_new(playlist_name(pl));
  int i, N; 
  for(i = 0, N = playlist_array_count(pl->list); i < N; ++i) {
    playlist_array_append(npl->list, playlist_array_get(pl->list, i));
  }
  return npl;
}

void playlist_append(playlist_t* pl, track_t* t)
{
  playlist_array_append(pl->list, t);
}

void playlist_insert(playlist_t* pl, int index, track_t* t)
{
  playlist_array_insert(pl->list, index, t); 
}

void playlist_set(playlist_t* pl,int index, track_t* t)
{
  playlist_array_set(pl->list, index, t);
}
 
const char* playlist_name(playlist_t* pl)
{
  return pl->name;
}
 
track_t* playlist_get(playlist_t* pl,int index)
{
  if (index >= 0 && index < playlist_array_count(pl->list)) { 
    return playlist_array_get(pl->list, index);
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
static int cmp_standard(track_t* t1, track_t* t2) 
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

void playlist_sort_standard(playlist_t* pl)
{
  playlist_array_sort(pl->list, cmp_standard); 
}

