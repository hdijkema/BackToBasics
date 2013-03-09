#ifndef __LYRICS__HOD
#define __LYRICS__HOD

#include <metadata/track.h>
#include <elementals/types.h>

void fetch_lyric(track_t* t, void (*f)(char* lyric, void* data), void* data);
void write_lyric(track_t* t, const char* lyric, el_bool overwrite);
char* lyric_html_to_text(const char* lyric);
char* lyric_text_to_html(const char* lyric);

#endif

