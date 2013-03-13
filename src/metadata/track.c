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
  I(lyric);
  I(artid);
  I(source_id);
  I(file_or_url);
  t->source_mtime = 0;
  t->is_file = 0;
  t->year = -1;
  t->nr = -1;
  t->id = -1;
  t->id_str = mc_strdup("-1");
  t->begin_offset_in_ms = -1;
  t->end_offset_in_ms = -1;
  t->length_in_ms = -1;
  t->file_size = -1;
  return t;
}

el_bool track_valid_id(track_t* t)
{
  return t->id >= 0;
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
  C(lyric);
  C(artid);
  C(source_id);
  CC(source_mtime);
  C(file_or_url);
  CC(is_file);
  CC(year);
  CC(nr);
  CC(id);
  C(id_str);
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
  D(lyric);
  D(artid);
  D(source_id);
  D(file_or_url);
  D(id_str);
  mc_free(t);
}

#define SET_GET(a) \
  void track_set_##a(track_t* t, const char* val) { \
    mc_free(t->/**/a); \
    t->/**/a =  mc_strdup(val); \
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
SET_GET(lyric);
SET_GET(artid)

SET_GET(source_id)
SET_GET_T(source_mtime, time_t);

SET_GET_T(year, int)
SET_GET_T(nr, int)

void track_set_id(track_t* t, long val)
{
  t->id = val;
  char s[100];
  sprintf(s, "%ld", val);
  mc_free(t->id_str);
  t->id_str = mc_strdup(s);
}

long track_get_id(track_t* t) 
{
  return t->id;
}

const char* track_get_id_as_str(track_t* t)
{
  return t->id_str;
}

SET_GET_T(file_size, long long);

void track_set_file(track_t* t, const char* file, long length_in_ms, long begin_offset_in_ms, long end_offset_in_ms)
{
  mc_free(t->file_or_url);
  t->file_or_url = mc_strdup(file);
  t->length_in_ms = length_in_ms;
  t->begin_offset_in_ms = begin_offset_in_ms;
  t->end_offset_in_ms = end_offset_in_ms;
  if (end_offset_in_ms >= 0) {
    t->length_in_ms = end_offset_in_ms - begin_offset_in_ms;
  } else if (begin_offset_in_ms > 0) {
    t->length_in_ms = length_in_ms - begin_offset_in_ms;
  }
  file_info_t* info = file_info_new(file);
  size_t s = file_info_size(info);
  file_info_destroy(info);
  if (t->length_in_ms == 0) {
    t->file_size = s;
  } else {
    t->file_size = (long long) (((double) length_in_ms / (double) t->length_in_ms) * s);
  }
  t->is_file = el_true;
}

void track_set_stream(track_t* t,const char* url)
{
  mc_free(t->file_or_url);
  t->file_or_url = mc_strdup(url);
  t->length_in_ms = -1;
  t->begin_offset_in_ms = -1;
  t->end_offset_in_ms = -1;
  t->file_size = -1;
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


#define L(a) log_debug3("  %s = %s", #a, t->/**/a )
#define LD(a) log_debug3("  %s = %d", #a, (int) t->/**/a)
void log_track(track_t* t)
{
  log_debug("begin track");
  LD(id);
  LD(nr);
  L(title);
  L(artist);
  L(composer);
  L(piece);
  L(album_title);
  L(album_artist);
  L(album_composer);
  LD(year);
  L(genre);
  L(artid);
  L(source_id);
  LD(source_mtime);
  L(file_or_url);
  LD(is_file);
  LD(begin_offset_in_ms);
  LD(end_offset_in_ms);
  LD(length_in_ms);
  LD(file_size);
  log_debug("end track");
}

#define TRACK_MAGIC_NUMBER 93224244L
#define WSTR(s, f) { long len = strlen(s)+1;fwrite(&len,sizeof(long),1,f);fwrite(s,len,1,f); }
#define WNR(n, f) fwrite(&n, sizeof(n), 1, f)

void track_fwrite(track_t* t, FILE* f) 
{
  long track_magic_number = TRACK_MAGIC_NUMBER;
  fwrite(&track_magic_number, sizeof(long), 1, f);
  WNR(t->id, f);
  WSTR(t->title, f);
  WSTR(t->artist, f);
  WSTR(t->composer, f);
  WSTR(t->piece, f);
  WSTR(t->album_title, f);
  WSTR(t->album_artist, f);
  WSTR(t->album_composer, f);
  WSTR(t->genre, f);
  //WSTR(t->lyric, f);
  WNR(t->year, f);
  WNR(t->nr, f);
  WSTR(t->artid, f);
  WNR(t->is_file, f);
  WSTR(t->source_id, f);
  WNR(t->source_mtime, f);
  WSTR(t->file_or_url, f);
  WNR(t->begin_offset_in_ms, f);
  WNR(t->end_offset_in_ms, f);
  WNR(t->length_in_ms, f);
  WNR(t->file_size, f);
}

#define RSTR(result, s, f) if (result) { long len; \
                     int c = fread(&len, sizeof(len), 1, f); \
                     mc_free(s); \
                     s = (char*) mc_malloc(len); \
                     c += fread(s, len, 1, f); \
                     result = (c == 2); \
}

#define RNR(result, n, f) if (result) { result = (fread(&n, sizeof(n), 1, f) == 1); }

el_bool track_fread(track_t* t, FILE* f)
{
  long track_magic_number;
  if (fread(&track_magic_number, sizeof(long), 1, f) != 1) {
    return el_false;
  }
  
  if (track_magic_number == TRACK_MAGIC_NUMBER) {
    el_bool result = el_true;
    RNR(result, t->id, f);
    track_set_id(t, t->id);
    RSTR(result, t->title, f);
    RSTR(result, t->artist, f);
    RSTR(result, t->composer, f);
    RSTR(result, t->piece, f);
    RSTR(result, t->album_title, f);
    RSTR(result, t->album_artist, f);
    RSTR(result, t->album_composer, f);
    RSTR(result, t->genre, f);
    //RSTR(result, t->lyric, f);
    mc_free(t->lyric);
    t->lyric = mc_strdup(""); // lyrics are read from the music library when needed
    RNR(result, t->year, f);
    RNR(result, t->nr, f);
    RSTR(result, t->artid, f);
    RNR(result, t->is_file, f);
    RSTR(result, t->source_id, f);
    RNR(result, t->source_mtime, f);
    RSTR(result, t->file_or_url, f);
    RNR(result, t->begin_offset_in_ms, f);
    RNR(result, t->end_offset_in_ms, f);
    RNR(result, t->length_in_ms, f);
    RNR(result, t->file_size, f);
    return result;
  } else {
    return el_false;
  }
}

int track_cmp(track_t* t1, track_t* t2)
{
  int r1 = strcmp(t1->file_or_url, t2->file_or_url);
  if (r1 == 0) {
    long b1 = t1->begin_offset_in_ms;
    long b2 = t2->begin_offset_in_ms;
    long e1 = (t1->end_offset_in_ms >= 0) ? t1->end_offset_in_ms : t1->length_in_ms;
    long e2 = (t2->end_offset_in_ms >= 0) ? t2->end_offset_in_ms : t2->length_in_ms;
    if (b1 == b2) {
      if (e1 > e2) { return 1; }
      else if (e1 == e2) { return 0; }
      else { return -1; }
    } else if (b1 > b2) { 
      return 1; 
    } else {
      return -1;
    }
  } else {
    return r1;
  }
}
