#include <metadata/tracks_source.h>
#include <metadata/cue.h>
#include <tag_c.h>
#include <libtagcoverart.h>

static track_t* copy(track_t* t) {
  return track_copy(t);
}

static void destroy(track_t* t) {
  track_destroy(t);
}

IMPLEMENT_EL_ARRAY(track_array, track_t, copy, destroy);

#define TL_SET(name,t,val) \
  track_set_##name(t, (val == NULL) ? "" : val) 
  
#define TL_SETT(name,t,val) \
  track_set_##name(t, val)

track_array tracks_from_cue(const char* cuefile) 
{
  cue_t *cue = cue_new(cuefile);
  
  track_array array = mc_take_over(track_array_new());
  
  int i, N;
  for(i = 0, N = cue_entries(cue);i < N;++i) {
    cue_entry_t *entry = cue_entry(cue, i);
    track_t* t = track_new();
    
    TL_SET(album_title, t, cue_album_title(cue));
    TL_SET(album_artist, t, cue_album_performer(cue));
    TL_SET(album_composer, t, cue_album_composer(cue));
    TL_SET(genre, t, cue_genre(cue));
    {
      file_info_t* info = file_info_new(cue_audio_file(cue));
      char* imgfile = mc_strdup(cue_image_file(cue));
      if (strcmp(imgfile, "") == 0) {
        mc_free(imgfile);
        char s[10240];
        snprintf(s, 10239, "img_%s.art", cue_album_title(cue));
        imgfile = mc_strdup(s);
        file_info_blend(imgfile);
      }
        
      file_info_t* artfile = file_info_new(imgfile);
      if (!file_info_is_absolute(artfile)) {
        const char* basedir = file_info_dirname(info);
        file_info_t* dir_info = file_info_new(basedir);
        file_info_t* nartfile = file_info_combine(dir_info, file_info_filename(artfile));
        TL_SET(artid, t, file_info_absolute_path(nartfile));
        file_info_destroy(dir_info);
        file_info_destroy(nartfile);
      } else {
        TL_SET(artid, t, imgfile);
      }
      
      file_info_destroy(artfile);
      file_info_destroy(info);
      mc_free(imgfile);
    }
    
    long bo = cue_entry_begin_offset_in_ms(entry);
    long be = cue_entry_end_offset_in_ms(entry);
    long le = -1;
    {
      TagLib_File* fl = taglib_file_new(cue_entry_audio_file(entry));
      if (fl != NULL && taglib_file_is_valid(fl)) {
        const TagLib_AudioProperties* ap = taglib_file_audioproperties(fl);
        le = taglib_audioproperties_length(ap) * 1000;
      }
      taglib_file_free(fl);
      taglib_tag_free_strings();
    }
    if (be < 0) be = le;

    log_debug5("audio file: %s, le = %ld, bo = %ld, be = %ld", cue_entry_audio_file(entry), le, bo, be);
    track_set_file(t, cue_entry_audio_file(entry), le, bo, be);
    
    TL_SET(title,t, cue_entry_title(entry));
    TL_SET(artist,t, cue_entry_performer(entry));
    TL_SET(composer,t, cue_entry_composer(entry));
    TL_SET(piece,t, cue_entry_piece(entry));
    TL_SETT(year,t, atoi(cue_entry_year(entry)));
    TL_SETT(nr, t, cue_entry_tracknr(entry));
    
    file_info_t *info = file_info_new(cuefile);
    TL_SET(source_id, t, file_info_absolute_path(info));
    TL_SETT(source_mtime, t, file_info_mtime(info));
    file_info_destroy(info);
    
    track_array_append(array, t);
    track_destroy(t);
  }
  
  cue_destroy(cue);
  
  return array;
}


track_array tracks_from_media(const char* localfile)
{
  TagLib_File* fl = taglib_file_new(localfile);
  track_array array = mc_take_over(track_array_new());
  
  if (fl == NULL) 
    return array;
  
  track_t* t = track_new();
  if (taglib_file_is_valid(fl)) {
    file_info_t* info = file_info_new(localfile);
    
    TagLib_Tag* tg = taglib_file_tag(fl);
    TL_SET(title, t, taglib_tag_title(tg));
    TL_SET(artist, t, taglib_tag_artist(tg));
    TL_SET(album_artist, t, taglib_tag_artist(tg));
    TL_SET(album_title, t, taglib_tag_album(tg));
    TL_SET(genre, t, taglib_tag_genre(tg));
    TL_SETT(year, t, (int) taglib_tag_year(tg));
    TL_SETT(nr, t, (int) taglib_tag_track(tg));
    TL_SET(piece, t, taglib_tag_comment(tg));
    
    if (strcmp(track_get_title(t),"") == 0) {
      TL_SET(title, t, file_info_basename(info)); 
    }
    
    const TagLib_AudioProperties* ap = taglib_file_audioproperties(fl);
    track_set_file(t, localfile, taglib_audioproperties_length(ap) * 1000, -1, -1);
    
    TL_SET(source_id, t, file_info_absolute_path(info));
    TL_SETT(source_mtime, t, file_info_mtime(info));
    
    // cover art & other tags not there in tag_c.h
    {
      file_info_t* art_dir = file_info_new(file_info_dirname(info));
      char *s = (char*) mc_malloc(sizeof(char) * (strlen(track_get_album_title(t)) + 4 + 4 + 1));
      strcpy(s,"img_");
      strcat(s,track_get_album_title(t));
      strcat(s,".art");
      file_info_blend(s);
      file_info_t* art_file = file_info_combine(art_dir, s);
      //log_debug2("art file = %s", file_info_absolute_path(art_file));
      
      // TAG
      tag_cover_art_t* tag = tag_cover_art_new(file_info_absolute_path(info));
      
      // ART
      if (!file_info_exists(art_file)) {
        //log_debug2("extracting %s", file_info_absolute_path(art_file));
        if (tag_cover_art_extract(tag, file_info_absolute_path(art_file))) {
           TL_SET(artid, t, file_info_absolute_path(art_file));
        } else {
          TL_SET(artid, t, file_info_absolute_path(art_file));
        }
      } else {
        TL_SET(artid, t, file_info_absolute_path(art_file));
      }
      
      // Composer
      {
        char* _composer;
        if (tag_cover_art_get_composer(tag, &_composer)) {
          TL_SET(composer, t, _composer);
          mc_free(_composer);
        }
      }

      // Destroy tag
      tag_cover_art_destroy(tag);
      file_info_destroy(art_file);
      file_info_destroy(art_dir);
      mc_free(s);
    }
    
    file_info_destroy(info);

    track_array_append(array, t);
    track_destroy(t);

  } 
  taglib_file_free(fl);
  taglib_tag_free_strings();
  
  return array;
}

el_bool save_track(track_t *t)
{
  return el_false;
}


