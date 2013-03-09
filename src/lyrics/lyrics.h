#ifndef __LYRICS__HOD
#define __LYRICS__HOD

#include <metadata/track.h>

void fetch_lyric(track_t* t, void (*f)(const char* lyric, void* data), void* data);

#endif

