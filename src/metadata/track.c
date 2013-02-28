#include <metadata/track.h>
#include <elementals.h>

#define I(a) t->/**/a = mc_strdup("")

track_t* track_new() 
{
  track_t* t = (track_t*) mc_malloc(sizeof(track_t));
  I(title);
  I(artist);
  I(composer);
  I(piece);
  I(genre);
  I(album_title);
  I(album_artist);
  I(album_composer);
  I(artid);
  I(source_id);
  I(file_or_url);
  t->year = -1;
  t->nr = -1;
  t->id = -1;
  t->begin_offset_in_ms = -1;
  t->end_offset_in_ms = -1;
  t->length_in_ms = -1;
  return t;
}

el_bool track_valid_id(track_t* t)
{
  return t->id>=0;
}

#define C(a) t->/**/a = mc_strdup(s->/**/a)
#define CC(a) t->/**/a = s->/**/a

track_t* track_copy(track_t* s) 
{
  track_t* t = (track_t*) mc_malloc(sizeof(track_t));
  C(title);
  C(artist);
  C(composer);
  C(piece);
  C(genre);
  C(album_title);
  C(album_artist);
  C(album_composer);
  C(artid);
  C(source_id);
  C(file_or_url);
  CC(year);
  CC(nr);
  CC(id);
  CC(begin_offset_in_ms);
  CC(end_offset_in_ms);
  CC(length_in_ms);
  return t;
}

#define D(a) mc_free(t->/**/a)

void track_destroy(track_t* t)
{
  D(title);
  D(artist);
  D(composer);
  D(piece);
  D(genre);
  D(album_title);
  D(album_artist);
  D(album_composer);
  D(artid);
  D(source_id);
  D(file_or_url);
  mc_free(t);
}

#define SET_GET(a) \
  void track_set_##a(track_t* t, const char* val) { \
    mc_free(t->/**/a); \
    t->/**/a = mc_strdup(val); \
  } \
  const char* track_get_##a(track_t* t) { \
    return t->/**/a; \
  }
  
#define SET_GET_T(a,type) \
  void track_set_##a(track_t *t, type val) { \
    t->/**/a = val; \
  } \
  type track_get_##a(track_t *t) { \
    return t->/**/a; \
  }
  
SET_GET(title)
SET_GET(artist)
SET_GET(composer)
SET_GET(piece)
SET_GET(genre)
SET_GET(album_title)
SET_GET(album_artist)
SET_GET(album_composer)
SET_GET(artid)

SET_GET(source_id)
SET_GET_T(source_mtime, time_t);

SET_GET_T(year, int)
SET_GET_T(nr, int)

SET_GET_T(id, long);

void track_set_file(track_t* t, const char* file, long length_in_ms, long begin_offset_in_ms, long end_offset_in_ms)
{
  mc_free(t->file_or_url);
  t->file_or_url = mc_strdup(file);
  t->length_in_ms = length_in_ms;
  t->begin_offset_in_ms = begin_offset_in_ms;
  t->end_offset_in_ms = end_offset_in_ms;
  t->is_file = el_true;
}

void track_set_stream(track_t* t,const char* url)
{
  mc_free(t->file_or_url);
  t->file_or_url = mc_strdup(url);
  t->length_in_ms = -1;
  t->begin_offset_in_ms = -1;
  t->end_offset_in_ms = -1;
  t->is_file = el_false;
}

#define GET_T(a, type, name) \
  type track_get_##a(track_t* t) { \
    return t->/**/name; \
  }
  
GET_T(file, const char*, file_or_url)
GET_T(url, const char*, file_or_url)
GET_T(is_file, el_bool, is_file)
GET_T(length_in_ms, long, length_in_ms)
GET_T(begin_offset_in_ms, long, begin_offset_in_ms)
GET_T(end_offset_in_ms, long, end_offset_in_ms)

el_bool track_get_is_stream(track_t* t)
{
  return !t->is_file;
}



