#ifndef __TRACKS_SOURCE__HOD
#define __TRACKS_SOURCE__HOD

#include <elementals.h>
#include <metadata/track.h>

DECLARE_EL_ARRAY(track_array, track_t);

track_array tracks_from_cue(const char* cuefile);
track_array tracks_from_media(const char* localfile);
el_bool save_track(track_t *t);

#endif
