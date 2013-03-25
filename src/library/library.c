#include <elementals.h>

#include <library/library.h>
#include <i18n/i18n.h>
#include <metadata/tracks_source.h>

#define LIBRARY_MAGIC_NUMBER  826292822L
#define LIBRARY_VERSION       1

static char* copy_str(char* s)
{
  return mc_strdup(s);
}

static void destroy_str(char* s)
{
  mc_free(s);
}

static track_t* copy_trk(track_t* trk)
{
  return mc_take_over(track_copy(trk));
}

static void destroy_trk(track_t* trk)
{
  track_destroy(trk);
}

static playlist_t* copy_pl(playlist_t* pl)
{
  return pl;
}

static void destroy_pl(playlist_t* pl)
{
  // does nothing
}

IMPLEMENT_EL_ARRAY(genre_array, char, copy_str, destroy_str);
IMPLEMENT_EL_ARRAY(artist_array, char, copy_str, destroy_str);
IMPLEMENT_EL_ARRAY(album_array, char, copy_str, destroy_str);
IMPLEMENT_EL_ARRAY(playlists_array, playlist_t, copy_pl, destroy_pl);
IMPLEMENT_HASH(library_db, track_t, copy_trk, destroy_trk); 

library_t* library_new(void)
{
  library_t* l = (library_t*)  mc_malloc(sizeof(library_t));
  l->current_id = 0;
  l->all_tracks = playlist_new(l, _("Library"));
  l->tracks_db = library_db_new(100, HASH_CASE_SENSITIVE);
  l->filter_genre = NULL;
  l->filter_album_artist = NULL;
  l->filter_album_title = NULL;
  l->dirty = el_true;
  l->filter = el_true;
  l->genres = mc_take_over(genre_array_new());
  l->artists = mc_take_over(artist_array_new());
  l->albums = mc_take_over(album_array_new());
  l->library_path = mc_strdup("");
  
  l->filtered_artists = set_new(SET_CASE_INSENSITIVE);
  l->filtered_albums = set_new(SET_CASE_INSENSITIVE);
  l->filtered_tracks = set_new(SET_CASE_SENSITIVE);
  
  l->playlists = mc_take_over(playlists_array_new());
  playlists_array_append(l->playlists, l->all_tracks);
  
  return l;
}

void library_destroy(library_t* l) 
{
  playlists_array_destroy(l->playlists);
  
  playlist_destroy(l->all_tracks);
  mc_free(l->filter_genre);
  mc_free(l->filter_album_artist);
  mc_free(l->filter_album_title);
  genre_array_destroy(l->genres);
  artist_array_destroy(l->artists);
  album_array_destroy(l->albums);
  set_destroy(l->filtered_artists);
  set_destroy(l->filtered_albums);
  set_destroy(l->filtered_tracks);
  library_db_destroy(l->tracks_db);
  mc_free(l->library_path);
  mc_free(l);
}

void library_clear(library_t* l)
{
  playlist_clear(l->all_tracks);
  genre_array_clear(l->genres);
  artist_array_clear(l->artists);
  album_array_clear(l->albums);
  library_db_clear(l->tracks_db);
  l->filter = el_true;
  l->dirty = el_true;
}

int library_count(library_t* l)
{
  return playlist_count(l->all_tracks);
}

library_result_t library_load(library_t* l, const char* localpath)
{
  FILE* f = fopen(localpath, "rt");
  l->dirty = el_true;
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
        
        //playlist_destroy(l->all_tracks);
        //l->all_tracks = playlist_new(l, _("Library"));
        playlist_clear(l->all_tracks);
        
        long ntracks;
        if (fread(&ntracks, sizeof(long), 1, f) != 1) {
          fclose(f);
          return LIBRARY_ERROR;
        }
        
        int i;
        for(i = 0;i < ntracks; ++i) {
          track_t* trk = track_new();
          track_fread(trk, f);
          library_add(l, trk);
          track_destroy(trk);
          //playlist_append(l->all_tracks, trk);
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
  playlist_t* pl = playlist_new(l, name);
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
      if (strcasecmp(track_get_album_title(trk), l->filter_album_title) != 0)
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
  library_db_put(l->tracks_db, track_get_id_as_str(t), t);
  track_t* nt = library_db_get(l->tracks_db, track_get_id_as_str(t));
  playlist_append(l->all_tracks, nt);
  l->dirty = el_true;
  return LIBRARY_OK;
}

track_t* library_get(library_t* l, const char* id)
{
  return library_db_get(l->tracks_db, id);
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
  //track_t* trk = playlist_get(l->all_tracks, index);
  playlist_set(l->all_tracks, index, t);
  l->dirty = el_true;
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
  l->filter = el_true;
}

void library_filter_genre(library_t* l, const char* genre)
{
  mc_free(l->filter_genre);
  l->filter_genre = (genre==NULL) ? NULL : mc_strdup(genre);
  l->filter = el_true;
}

void library_filter_album_artist(library_t* l, const char* album_artist)
{
  mc_free(l->filter_album_artist);
  l->filter_album_artist = (album_artist == NULL) ? NULL : mc_strdup(album_artist);
  l->filter = el_true;
}

void library_filter_album_title(library_t* l, const char* album_title)
{
  mc_free(l->filter_album_title);
  l->filter_album_title = (album_title == NULL) ? NULL : mc_strdup(album_title);
  l->filter = el_true;
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

static void fill_arrays(library_t* l)
{
  if (l->dirty) {
    scanned_hash *hgenres = scanned_hash_new(100, HASH_CASE_INSENSITIVE);
    scanned_hash *hartists = scanned_hash_new(100, HASH_CASE_INSENSITIVE);
    scanned_hash *halbums = scanned_hash_new(100, HASH_CASE_INSENSITIVE);
    
    genre_array_clear(l->genres);
    artist_array_clear(l->artists);
    album_array_clear(l->albums);
    
    int i, N;
    playlist_t* pl = l->all_tracks;
    for(i = 0, N = playlist_count(pl); i < N; ++i) {
      track_t* trk = playlist_get(pl, i);
      
      if (!scanned_hash_exists(hgenres, track_get_genre(trk))) {
        scanned_hash_put(hgenres, track_get_genre(trk), "");
        genre_array_append(l->genres, (char*) track_get_genre(trk));
      }
      
      if (!scanned_hash_exists(hartists, track_get_album_artist(trk))) {
        scanned_hash_put(hartists, track_get_album_artist(trk), "");
        artist_array_append(l->artists, (char*) track_get_album_artist(trk));
      }
      
      if (!scanned_hash_exists(halbums, track_get_album_title(trk))) {
        scanned_hash_put(halbums, track_get_album_title(trk), "");
        album_array_append(l->albums, (char*) track_get_album_title(trk));
      }
    }
    
    genre_array_sort(l->genres, (int (*)(char*,char*)) strcasecmp);
    artist_array_sort(l->artists, (int (*)(char*,char*)) strcasecmp);
    album_array_sort(l->albums, (int (*)(char*,char*)) strcasecmp);
    
    scanned_hash_destroy(hgenres);
    scanned_hash_destroy(hartists);
    scanned_hash_destroy(halbums);
    
    l->dirty = el_false;
  }
}

genre_array library_genres(library_t* l)
{
  fill_arrays(l);
  int i,N;
  for(i=0, N= genre_array_count(l->genres); i < N; i++) {
    log_debug2("%s", genre_array_get(l->genres, i));
  }
  return l->genres;
  //return genre_array_copy(l->genres);
}

album_array library_albums(library_t* l)
{
  fill_arrays(l);
  return l->albums;
  //return album_array_copy(l->albums);
}

artist_array library_artists(library_t* l)
{
  fill_arrays(l);
  return l->artists;
  //return artist_array_copy(l->artists);
}

static void fill_filters(library_t* l)
{
  if (l->filter) {
    l->filter = el_false;
    
    set_clear(l->filtered_artists);
    set_clear(l->filtered_albums);
    set_clear(l->filtered_tracks);
    
    int i, N;
    playlist_t* pl = l->all_tracks;
    for(i = 0, N = playlist_count(pl); i < N; ++i) {
      track_t* trk = playlist_get(pl, i);
      
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
        if (strcasecmp(track_get_album_title(trk), l->filter_album_title) != 0)
          take_track = el_false;
      }
      if (take_track) {
        set_put(l->filtered_artists, track_get_album_artist(trk));
        set_put(l->filtered_albums, track_get_album_title(trk));
        set_put(l->filtered_tracks, track_get_id_as_str(trk));
      }
    }
    
  }
}

set_t* library_filtered_artists(library_t* l)
{
  fill_filters(l);
  return l->filtered_artists;
}

set_t* library_filtered_albums(library_t* l)
{
  fill_filters(l);
  return l->filtered_albums;
}

set_t* library_filtered_tracks(library_t* l)
{
  fill_filters(l);
  return l->filtered_tracks;
}

static void scanlib(library_t* l, file_info_t* path, scanned_hash *hash,
                      scan_job_t* job, ScanJobCBFunc cb, int *count, int total)
{
  hre_t re_cue = hre_compile("[.]cue$","i");
  hre_t re_music = hre_compile("[.](mp3|flac|m4a|ape|aac|ogg)$","i");
  
  file_info_array cues = file_info_scandir(path, re_cue);
  
  const char* msg=_("Scanning media files");
  
  scan_result_t scan_res = SCAN_CONTINUE;
  
  int i, N;
  for(i = 0, N = file_info_array_count(cues);scan_res == SCAN_CONTINUE && i < N; ++i) {
    
    file_info_t* info = file_info_array_get(cues, i);

    *count += 1;
    if (cb) {
      char submsg[2048];
      sprintf(submsg, "scanning %s", file_info_filename(info));
      scan_res = cb(job, el_false, msg, submsg, *count, total);
    }
    
    if (scan_res == SCAN_CONTINUE && !file_info_is_hidden(info)) {
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
  }

  file_info_array_destroy(cues);
  file_info_array musics = file_info_scandir(path, re_music);

  for(i = 0, N = file_info_array_count(musics);scan_res == SCAN_CONTINUE && i < N; ++i) {
    
    file_info_t* info = file_info_array_get(musics, i);

    *count += 1;
    if (cb) {
      char submsg[2048];
      sprintf(submsg, "scanning %s", file_info_filename(info));
      scan_res = cb(job, el_false, msg, submsg, *count, total);
    }
    
    if (scan_res == SCAN_CONTINUE && !file_info_is_hidden(info)) {
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
  }

  file_info_array_destroy(musics);
  file_info_array subdirs = file_info_subdirs(path);
  
  for(i = 0, N = file_info_array_count(subdirs);scan_res == SCAN_CONTINUE && i < N; ++i) {
    scanlib(l, file_info_array_get(subdirs, i), hash, job, cb, count, total);
  }
  
  file_info_array_destroy(subdirs);

  hre_destroy(re_cue);
  hre_destroy(re_music);
}

int count_files(const file_info_t* path,scan_job_t* job, ScanJobCBFunc cb)
{ 
  hre_t re_music = hre_compile("[.](cue|mp3|flac|m4a|ape|aac|ogg)$","i");

  scan_result_t scan_res = SCAN_CONTINUE; 
  
  const char* msg =_("Counting media files");
  char submsg[2048];
  sprintf(submsg,_("Scanning directory %s"), file_info_filename(path));
  if (cb) {
    scan_res = cb(job, el_false, msg, submsg, 0, 100);
  }

  
  if (scan_res == SCAN_CONTINUE) {
    file_info_array A = file_info_scandir(path, re_music);
    int count = file_info_array_count(A);
    file_info_array_destroy(A);
    hre_destroy(re_music);
    
    file_info_array subdirs = file_info_subdirs(path);
    int i, N;
    for(i = 0, N = file_info_array_count(subdirs);i < N; ++i) {
      count += count_files(file_info_array_get(subdirs, i), job, cb);
    }
    file_info_array_destroy(subdirs);
    
    return count;
  } else {
    return 0;
  }
  
}

void library_set_basedir(library_t* library, const char* path)
{
  mc_free(library->library_path);
  library->library_path = mc_strdup(path);
}

const char* library_get_basedir(library_t* l)
{
  return l->library_path;
}

void scan_library(scan_job_t* job, ScanJobCBFunc cb, void* lib) 
{
  library_t* library = (library_t*) lib;
  char* path = library->library_path;
  scanned_hash *h = scanned_hash_new(100, HASH_CASE_SENSITIVE);
  file_info_t* info = file_info_new(path);
  
  library_clear(library);
  
  log_info2("Scanning library, %s", path); 
  
  log_info("counting total files");
  int total_files = count_files(info, job, cb);
  if (total_files == 0) { total_files = 1; }
  log_info2("total files = %d", total_files);
  
  int count = 0;
  scanlib(library, info, h, job, cb, &count, total_files);
  log_debug("scanlib done");

  log_debug2("library count: %d", library_count(library));
  
  file_info_destroy(info);
  log_debug("info destroyed");
  
  scanned_hash_destroy(h);
  
  library->dirty = el_true;
  log_debug("hash destroyed");
  
  if (cb) {
    cb(job, el_false, _("Scanning media library"), _("Sorting library"), count, total_files);
  }
  
  library_sort(library);
  
  log_debug2("library count: %d", library_count(library));

  if (cb) {   
    cb(job, el_true, _("Scanning media library"), _("Finished"), count, total_files);
  }
  
}

void library_sort(library_t* library)
{
  playlist_sort_standard(library->all_tracks);
}

/*******************************************************************
 * playlists
 *******************************************************************/
 
playlist_t* library_playlists_add(library_t* l, const char* name)
{
  playlist_t* pl = playlist_new(l, name);
  playlists_array_append(l->playlists, pl);
  return pl;
}

int library_playlists_count(library_t* l)
{
  return playlists_array_count(l->playlists);
}

playlist_t* library_playlists_get(library_t* l, int index)
{
  return playlists_array_get(l->playlists, index);
}

