#include <metadata/tracks_source.h>

static track_t* copy(track_t* t) {
  return track_copy(t);
}

static void destroy(track_t* t) {
  track_destroy(t);
}

IMPLEMENT_EL_ARRAY(track_array, track_t, copy, destroy);

track_array tracks_from_cue(const char* cuefile) 
{
  return NULL;
}

track_array tracks_from_mp3(const char* localfile)
{
  return NULL;
}

track_array tracks_from_flac(const char* localfile)
{
  return NULL;
}


