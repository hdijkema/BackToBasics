#ifndef __TRACKS_SOURCE__HOD
#define __TRACKS_SOURCE__HOD

#include <elementals.h>
#include <metadata/track.h>

DECLARE_EL_ARRAY(track_array, track_t);

track_array tracks_from_cue(const char* cuefile);
track_array tracks_from_mp3(const char* localfile);
track_array tracks_from_flac(const char* localfile);

#endif
