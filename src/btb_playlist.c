#include <elementals.h>
#include <stdio.h>
#ifdef HAVE_READLINE
#include <readline.h>
#else
#error Needs readline
#endif
#include <audio/audio.h>
#include <metadata/tracks_source.h>
#include <playlist/playlist_player.h>
#include <playlist/playlist.h>
#include <library/library.h>
#include <metadata/track.h>
#include <i18n/i18n.h>

void sleep_ms(int ms)
{
  struct timespec t = { 0, ms * 1000000 };
  nanosleep(&t,NULL);
}

void p_player(playlist_player_t *player)
{
  int track = playlist_player_get_track(player);
  long pos = playlist_player_get_track_position_in_ms(player);
  el_bool changed = playlist_player_get_track_changed(player);
  playlist_t* pl = playlist_player_get_playlist(player);
  el_bool playing = playlist_player_is_playing(player);
  el_bool paused = playlist_player_is_paused(player);
  el_bool nothing = playlist_player_does_nothing(player);
  playlist_player_repeat_t rep = playlist_player_get_repeat(player);
  track_t* t = playlist_get(pl, track);
  long len = 0;
  if (t != NULL) {
    len = track_get_length_in_ms(t);
  }
  
  printf("current track = %d\n", track);
  printf("state = %s\n", (playing) ? "playing" : ((paused) ? "paused" : "stopped"));
  printf("track changed: %s\n", (changed) ? "yes" : "no"); 
  printf("repeat = %s\n", (rep == PLP_NO_REPEAT) ? "no" 
                                                 : ((rep == PLP_TRACK_REPEAT) ? "track" 
                                                                              : "list"));
  int min = pos/1000/60;
  int sec = (pos/1000)%60;
  int ms = pos%1000;
  
  int lmin = len/1000/60;
  int lsec = (len/1000)%60;
  int lms = len%1000;
  printf("time = %02d:%02d:%03d (%02d:%02d:%03d)\n", min, sec, ms, lmin, lsec, lms);
} 

char *readln(const char *prompt, playlist_player_t* player) 
{
  hre_t re_empty_line = hre_compile("^\\s*$","");
  char* line;
  el_bool stop = el_false;
  while (!stop && (line = readline(prompt)) != NULL) {
    if (hre_has_match(re_empty_line, line)) {
      // do nothing
    } else {
      stop = el_true;
    }
    p_player(player);
  }

  if (line != NULL) {
    add_history(line);
    mc_take_control(line, strlen(line)+1);
  } else {
    line = mc_strdup("");
  }
  
  return line;
}

int log_this_severity(int severity) 
{
  return 1;
}

FILE* log_handle()
{
  return stderr;
}

void library_cb(int count, int tot) {
  printf("scanning library %d of %d\r", count, tot);
}


int main(char *argv[],int argc) {
  mc_init();
  el_bool stop = el_false;
  
  hre_t re_load = hre_compile("^load(&play)?\\s+(.*)$","i");
  hre_t re_play = hre_compile("^play$","i");
  hre_t re_pause = hre_compile("^pause$","i");
  hre_t re_next = hre_compile("^next$","i");
  hre_t re_previous = hre_compile("^prev(ious)?$","i");
  hre_t re_track = hre_compile("^track\\s+([0-9]+)$","i");
  hre_t re_quhit = hre_compile("^quit$","i");
  hre_t re_seek = hre_compile("^seek\\s([0-9]+([.][0-9]+)?)$","i");
  hre_t re_quit = hre_compile("^quit$","i");
  hre_t re_repeat = hre_compile("^repeat\\s+(off|list|track)$","i");
  hre_t re_again = hre_compile("^again$","i");
  hre_t re_scan = hre_compile("^scan\\s+(.*)$","i");
  
  file_info_t *history_file = mc_take_over(file_info_new_home(".btb_playlist"));
  printf("history file: %s\n", file_info_absolute_path(history_file));
  printf("history file: %s\n", file_info_path(history_file));
  if (file_info_exists(history_file)) {
    read_history(file_info_absolute_path(history_file));
  }
  
  i18n_set_language("nl");
  printf(_("Starting btb_playlist\n"));

  playlist_player_t *player = playlist_player_new();

  while (!stop) {
    char* line = readln(">", player);
    hre_trim(line);
    if (hre_has_match(re_quit, line)) {
      stop = el_true;
    } else if (hre_has_match(re_load, line)) {
      hre_matches m = hre_match(re_load, line);
      hre_match_t *mplay = hre_matches_get(m, 1);
      hre_match_t *match = hre_matches_get(m, 2);
      char* file = mc_strdup(hre_match_str(match));
      char* play = mc_strdup(hre_match_str(mplay));
      hre_matches_destroy(m);
      printf("loading '%s', '%s'\n", file, play);
      file_info_t *info = file_info_new(file);
      if (file_info_exists(info)) {
        if (file_info_is_file(info)) {
          if (file_info_can_read(info)) {
            const char *mediafile = NULL;
            track_array array;
            if (strcasecmp(file_info_ext(info),"cue") == 0) {
              array = tracks_from_cue(file_info_absolute_path(info));
              track_t* t = track_array_get(array, 0);
            } else {
              array = tracks_from_media(file_info_absolute_path(info));
            }
            playlist_t* pl = playlist_new("playlist");
            int i;
            for(i=0; i < track_array_count(array); ++i) {
              playlist_append(pl, track_array_get(array, i));
            }
            playlist_player_set_playlist(player, pl);
            track_array_destroy(array);
          }
        }
      }
      file_info_destroy(info);      
      if (strcasecmp(play,"&play")==0) { playlist_player_play(player); }
      mc_free(file);
      mc_free(play);
    } else if (hre_has_match(re_play, line)) {
      playlist_player_play(player);
    } else if (hre_has_match(re_pause, line)) {
      playlist_player_pause(player);
    } else if (hre_has_match(re_seek, line)) {
      hre_matches m = hre_match(re_seek, line);
      hre_match_t* match = hre_matches_get(m, 1);
      char* position = mc_strdup(hre_match_str(match));
      hre_matches_destroy(m);
      double s = atof(position);
      int ms = s * 1000;
      mc_free(position);
      playlist_player_seek(player, ms); 
    } else if (hre_has_match(re_next, line)) {
      playlist_player_next(player);
    } else if (hre_has_match(re_previous, line)) {
      playlist_player_previous(player);
    } else if (hre_has_match(re_track, line)) {
      hre_matches m = hre_match(re_track, line);
      hre_match_t* match = hre_matches_get(m, 1);
      char* nr = mc_strdup(hre_match_str(match));
      hre_matches_destroy(m);
      int index = atoi(nr);
      mc_free(nr);
      printf("setting track %d\n",index);
      playlist_player_set_track(player, index);
    } else if (hre_has_match(re_repeat, line)) {
      hre_matches m = hre_match(re_repeat, line);
      hre_match_t* match = hre_matches_get(m, 1);
      if (strcasecmp(hre_match_str(match),"track") == 0) {
        playlist_player_set_repeat(player, PLP_TRACK_REPEAT);
      } else if (strcasecmp(hre_match_str(match),"list") == 0) {
        playlist_player_set_repeat(player, PLP_LIST_REPEAT);
      } else {
        playlist_player_set_repeat(player, PLP_NO_REPEAT);
      }
      hre_matches_destroy(m);
    } else if (hre_has_match(re_again, line)) {
      playlist_player_again(player);
    } else if (hre_has_match(re_scan, line)) {
      hre_matches m = hre_match(re_scan, line);
      hre_match_t* match = hre_matches_get(m, 1);
      printf("scanning %s\n", hre_match_str(match));
      library_t* library = library_new();
      printf("calling scan_library\n");
      scan_library(library, hre_match_str(match), library_cb);
      printf("\n");
      printf("library: %d tracks\n", library_count(library));
      printf("done scanning, destroying library\n");
      library_destroy(library);
      printf("library destroyed\n");
      hre_matches_destroy(m);
      printf("matches destroyed\n");
    }
    
    mc_free(line);
  }

  playlist_player_destroy(player);
  
  printf("%d\n",write_history(file_info_absolute_path(history_file)));
  
  file_info_destroy(history_file);
  hre_destroy(re_load);
  hre_destroy(re_play);
  hre_destroy(re_pause);
  hre_destroy(re_quit);
  hre_destroy(re_seek);
  hre_destroy(re_track);
  hre_destroy(re_next);
  hre_destroy(re_previous);
  hre_destroy(re_again);
  hre_destroy(re_repeat);
  hre_destroy(re_scan);
  
  return 0;
}

