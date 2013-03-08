#ifndef __TRACK__HOD
#define __TRACK__HOD

#include <elementals/types.h>
#include <time.h>
#include <stdio.h>


typedef struct {
  long id;
  char* id_str;
  char* title;
  char* artist;
  char* composer;
  char* piece;
  char* album_title;
  char* album_artist;
  char* album_composer;
  char* genre;
  char* lyric;
  int year;
  int nr;
  char* artid;
  el_bool is_file;
  char* source_id;          // id of the source (e.g. cuesheet location)
  time_t source_mtime;      // gard is source has changed
  char* file_or_url; 
  long begin_offset_in_ms;  // -1 is not used
  long end_offset_in_ms;    // -1 is unknown
  long length_in_ms;        // -1 is unknown, length must be the length of the segment 
                            // (endoffset-beginoffset) or the length of the media file if
                            // endoffset<0
  long long file_size;      // file_size is the size of the segment in bytes (by approximation)
                            // !never exact! it is calculated when track_set_file is used.
                            // it can be used to estimate the size of a copy of tracks to an audio 
                            // device.
} track_t;

track_t* track_new();
void track_destroy(track_t* t);
el_bool track_valid_id(track_t* t);
track_t* track_copy(track_t* t);

el_bool track_fread(track_t* t, FILE* f);
void track_fwrite(track_t* t, FILE* f);

void log_track(track_t* t);

void track_set_id(track_t* t, long id);
long track_get_id(track_t* t);
const char* track_get_id_as_str(track_t* t);

void track_set_file_size(track_t* t, long long size);
long long track_get_file_size(track_t* t);

void track_set_genre(track_t* t, const char* genre);
const char* track_get_genre(track_t* t);

void track_set_title(track_t* t, const char* val);
const char* track_get_title(track_t* t);

void track_set_artist(track_t* t, const char* val);
const char* track_get_artist(track_t* t);

void track_set_composer(track_t* t, const char* val);
const char* track_get_composer(track_t* t);

void track_set_piece(track_t* t, const char* val);
const char* track_get_piece(track_t* t);

void track_set_album_title(track_t* t, const char* val);
const char* track_get_album_title(track_t* t);

void track_set_album_artist(track_t* t, const char* val);
const char* track_get_album_artist(track_t* t);

void track_set_album_composer(track_t* t, const char* val);
const char* track_get_album_composer(track_t* t);

void track_set_year(track_t* t, int year);
int track_get_year(track_t* t);

void track_set_nr(track_t* t, int year);
int track_get_nr(track_t* t);

void track_set_artid(track_t* t, const char* val);
const char* track_get_artid(track_t* t);

void track_set_file(track_t* t, const char* file, long length_in_ms, long begin_offset_in_ms, long end_offset_in_ms);
void track_set_stream(track_t* t, const char* url);

const char* track_get_file(track_t* t);
const char* track_get_url(track_t* t);
el_bool track_get_is_file(track_t* t);
el_bool track_get_is_stream(track_t* t);
long track_get_length_in_ms(track_t* t);
long track_get_begin_offset_in_ms(track_t* t);
long track_get_end_offset_in_ms(track_t* t);

void track_set_source_mtime(track_t* t, time_t val);
time_t track_get_source_mtime(track_t* t);

void track_set_source_id(track_t* t,const char* id);
const char* track_get_source_id(track_t* t);

void track_set_lyric(track_t* t,const char* lyric);
const char* track_get_lyric(track_t* t);

int track_cmp(track_t *t1, track_t* t2);

#endif

