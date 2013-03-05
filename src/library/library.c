#include <elementals.h>

#include <library/library.h>
#include <i18n/i18n.h>
#include <metadata/tracks_source.h>

#define LIBRARY_MAGIC_NUMBER  826292822L
#define LIBRARY_VERSION       1

library_t* library_new(void)
{
  library_t* l = (library_t*) mc_malloc(sizeof(library_t));
  l->current_id = 0;
  l->all_tracks = playlist_new(_("Library"));
  l->filter_genre = NULL;
  l->filter_album_artist = NULL;
  l->filter_album_title = NULL;
  return l;
}

void library_destroy(library_t* l) 
{
  playlist_destroy(l->all_tracks);
  mc_free(l->filter_genre);
  mc_free(l->filter_album_artist);
  mc_free(l->filter_album_title);
  mc_free(l);
}

int library_count(library_t* l)
{
  return playlist_count(l->all_tracks);
}

library_result_t library_load(library_t* l, const char* localpath)
{
  FILE* f = fopen(localpath, "rt");
  if (f!=NULL) {
    long magic_number = -1;
    short version = -1;
    if (fread(&magic_number, sizeof(long), 1, f) != 1) {
      fclose(f);
      return LIBRARY_ERROR;
    }
    
    if (magic_number == LIBRARY_MAGIC_NUMBER) {
      if (fread(&version, sizeof(short), 1, f) != 1) {
        fclose(f);
        return LIBRARY_ERROR;
      }
      
      if (version == LIBRARY_VERSION) {
        if (fread(&l->current_id, sizeof(long), 1, f) != 1) {
          fclose(f);
          return LIBRARY_ERROR;
        }
        
        playlist_destroy(l->all_tracks);
        l->all_tracks = playlist_new(_("Library"));
        
        long ntracks;
        if (fread(&ntracks, sizeof(long), 1, f) != 1) {
          fclose(f);
          return LIBRARY_ERROR;
        }
        
        int i;
        for(i = 0;i < ntracks; ++i) {
          track_t* trk = track_new();
          track_fread(trk, f);
          playlist_append(l->all_tracks, trk);
        }
      } else {
        fclose(f);
        log_error3("I don't understand this version %d, my version is %d", version, LIBRARY_VERSION);
        return LIBRARY_ERROR;
      }
    } else {
      fclose(f);
      log_error("This is not a BackToBasics library, bad magic number");
      return LIBRARY_ERROR;
    }
    fclose(f);
    return LIBRARY_OK;
  } else {
    return LIBRARY_ERROR;
  }
}

library_result_t library_save(library_t* l, const char* path)
{
  FILE *f = fopen(path,"wt");
  if (f == NULL) {
    log_error2("Cannot open library file '%s' for writing", path);
    return LIBRARY_ERROR;
  } else {
    long magic_number = LIBRARY_MAGIC_NUMBER;
    short version = LIBRARY_VERSION;
    fwrite(&magic_number, sizeof(long), 1, f);
    fwrite(&version, sizeof(short), 1, f);
    fwrite(&l->current_id, sizeof(long), 1, f);
    
    long ntracks = playlist_count(l->all_tracks);
    fwrite(&ntracks, sizeof(long), 1, f);
    int i;
    for(i = 0;i < ntracks; ++i) {
      track_fwrite(playlist_get(l->all_tracks, i), f);
    }
    fclose(f);
    return LIBRARY_OK;
  }
}

playlist_t* library_current_selection(library_t* l, const char* name)
{
  playlist_t* pl = playlist_new(name);
  int i, N = playlist_count(l->all_tracks);
  for(i = 0;i < N; ++i) {
    track_t* trk = playlist_get(l->all_tracks, i);
    el_bool take_track = el_true;
    if (l->filter_genre != NULL) {
      if (strcasecmp(track_get_genre(trk), l->filter_genre) != 0) 
        take_track = el_false;
    }
    if (l->filter_album_artist != NULL) {
      if (strcasecmp(track_get_album_artist(trk), l->filter_album_artist) != 0)
        take_track = el_false;
    }
    if (l->filter_album_title != NULL) {
      if (strcasecmp(track_get_album_artist(trk), l->filter_album_artist) != 0)
        take_track = el_false;
    }
    if (take_track) 
      playlist_append(pl, trk);
  }
  
  return pl;
}

library_result_t library_add(library_t* l, track_t* t)
{
  l->current_id += 1;
  track_set_id(t, l->current_id);
  playlist_append(l->all_tracks, t);
  return LIBRARY_OK;
}

int library_find_index(library_t* l, track_t* t)
{
  int i, N = playlist_count(l->all_tracks);
  for(i = 0;i < N && track_cmp(playlist_get(l->all_tracks, i),t) != 0; ++i);
  if (i != N) {
    return i;
  } else {
    return -1;
  }
}

library_result_t library_set(library_t* l, int index, track_t* t)
{
  track_t* trk = playlist_get(l->all_tracks, index);
  track_set_id(t, track_get_id(trk));
  playlist_set(l->all_tracks, index, t);
  return LIBRARY_OK;
}

void library_filter_none(library_t* l)
{
  mc_free(l->filter_genre);
  mc_free(l->filter_album_artist);
  mc_free(l->filter_album_title);
  l->filter_genre = NULL;
  l->filter_album_artist = NULL;
  l->filter_album_title = NULL;
}

void library_filter_genre(library_t* l, const char* genre)
{
  mc_free(l->filter_genre);
  l->filter_genre = mc_strdup(genre);
}

void library_filter_album_artist(library_t* l, const char* album_artist)
{
  mc_free(l->filter_album_artist);
  l->filter_album_artist = mc_strdup(album_artist);
}

void library_filter_album_title(library_t* l, const char* album_title)
{
  mc_free(l->filter_album_title);
  l->filter_album_title = mc_strdup(album_title);
}

static char* copy(char* c)
{
  return mc_strdup(c);
}

static void destroy(char* c)
{
  mc_free(c);
}

DECLARE_STATIC_HASH(scanned_hash, char);
IMPLEMENT_STATIC_HASH(scanned_hash, char, copy, destroy);

static void scanlib(library_t* l, file_info_t* path, scanned_hash *hash,
                      void (*cb)(int c, int t), int *count, int total)
{
  hre_t re_cue = hre_compile("[.]cue$","i");
  hre_t re_music = hre_compile("[.](mp3|flac|m4a|ape|aac|ogg)$","i");
  
  file_info_array cues = file_info_scandir(path, re_cue);
  
  int i, N;
  for(i = 0, N = file_info_array_count(cues);i < N; ++i) {
    
    *count += 1;
    cb(*count, total);
    
    file_info_t* info = file_info_array_get(cues, i);
    track_array tracks = tracks_from_cue(file_info_absolute_path(info));

    int k, M;
    for(k = 0, M = track_array_count(tracks); k < M; ++k) {
      track_t* t = track_array_get(tracks, k);
      if (track_get_is_file(t)) {
        file_info_t* tf = file_info_new(track_get_file(t));
        const char* p = file_info_absolute_path(tf);
        if (!scanned_hash_exists(hash, p)) {
          scanned_hash_put(hash, p, (char*) p);
        }
        file_info_destroy(tf);
      } else {
        if (!scanned_hash_exists(hash, track_get_url(t))) {
          scanned_hash_put(hash, track_get_url(t), (char*) track_get_url(t));
        }
      }
      library_add(l, t);
    }
    
    track_array_destroy(tracks);
  }

  file_info_array_destroy(cues);
  file_info_array musics = file_info_scandir(path, re_music);

  for(i = 0, N = file_info_array_count(musics);i < N; ++i) {
    
    *count += 1;
    cb(*count, total);
    
    file_info_t* info = file_info_array_get(musics, i);
    const char* p = file_info_absolute_path(info);
    
    if (!scanned_hash_exists(hash, p)) {
      track_array tracks = tracks_from_media(p);
      
      int k, M;
      for(k = 0, M = track_array_count(tracks); k < M; ++k) {
        track_t* t = track_array_get(tracks, k);
        library_add(l, t);  
      }
      
      track_array_destroy(tracks);
    }
  }

  file_info_array_destroy(musics);
  file_info_array subdirs = file_info_subdirs(path);
  
  for(i = 0, N = file_info_array_count(subdirs);i < N; ++i) {
    scanlib(l, file_info_array_get(subdirs, i), hash, cb, count, total);
  }
  
  file_info_array_destroy(subdirs);

  hre_destroy(re_cue);
  hre_destroy(re_music);
}

int count_files(const file_info_t* path)
{
  hre_t re_music = hre_compile("[.](cue|mp3|flac|m4a|ape|aac|ogg)$","i");
  file_info_array A = file_info_scandir(path, re_music);
  int count = file_info_array_count(A);
  file_info_array_destroy(A);
  hre_destroy(re_music);
  
  file_info_array subdirs = file_info_subdirs(path);
  int i, N;
  for(i = 0, N = file_info_array_count(subdirs);i < N; ++i) {
    count += count_files(file_info_array_get(subdirs, i));
  }
  file_info_array_destroy(subdirs);
  
  return count;
}

void scan_library(library_t* library, const char* path, void (*cb)(int c, int tot))
{
  scanned_hash *h = scanned_hash_new(100, HASH_CASE_SENSITIVE);
  file_info_t* info = file_info_new(path);
  
  log_info("counting total files");
  int total_files = count_files(info);
  if (total_files == 0) { total_files = 1; }
  log_info2("total files = %d", total_files);
  
  int count = 0;
  scanlib(library, info, h, cb, &count, total_files);
  log_debug("scanlib done");
  
  file_info_destroy(info);
  log_debug("info destroyed");
  
  scanned_hash_destroy(h);
  
  log_debug("hash destroyed");
}

