#include <playlist/playlist.h>

/*****************************************************************
 * playlist array
 *****************************************************************/

static track_t* copy(track_t* t)
{
  return track_copy(t);
}

static void destroy(track_t* t)
{
  track_destroy(t);
}

IMPLEMENT_EL_ARRAY(playlist_array, track_t, copy, destroy);

/*****************************************************************
 * playlist implementation
 *****************************************************************/
 
 playlist_t* playlist_new(const char* name) 
 {
   playlist_t* pl = (playlist_t*) mc_malloc(sizeof(playlist_t));
   pl->list = playlist_array_new();
   pl->name = mc_strdup(name);
   return pl;
 }
 
 void playlist_destroy(playlist_t* pl)
 {
   mc_free(pl->name);
   playlist_array_destroy(pl->list);
   mc_free(pl);
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


