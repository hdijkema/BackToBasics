#include <elementals.h>
#include <stdio.h>
#ifdef HAVE_READLINE
#include <readline.h>
#else
#error Needs readline
#endif
#include <audio/audio.h>
#include <metadata/tracks_source.h>
#include <i18n/i18n.h>

void sleep_ms(int ms)
{
  struct timespec t = { 0, ms * 1000000 };
  nanosleep(&t,NULL);
}

void work_worker(audio_worker_t* worker, el_bool print)
{
  if (print) { sleep_ms(100); }
  
  if (worker != NULL) {
    audio_state_t st = AUDIO_NONE;
    double secs = -1.0;
    int different = 0;
    if (print) printf("\r");
    while (media_peek_event(worker)) {
      audio_event_t* event = media_get_event(worker);
      if (audio_event_state(event) != st) {
        if (st != AUDIO_NONE && print) { 
          printf("\n");
        }
        if (st != AUDIO_NONE) {
          different = 1;
        }
        st = audio_event_state(event);
      }
      secs = audio_event_ms(event) / 1000.0;
      if (different) {
        printf("%02d %06.03lf\n", audio_event_state(event), secs);
        different = 0;
      }
      audio_event_destroy(event);
    }
    if (print) printf("%02d %06.03lf\n", st, secs);
  }
} 

char *readln(const char *prompt, audio_worker_t* worker) 
{
  hre_t re_empty_line = hre_compile("^\\s*$","");
  char* line;
  el_bool stop = el_false;
  while (!stop && (line = readline(prompt)) != NULL) {
    if (hre_has_match(re_empty_line, line)) {
      // do nothing
      work_worker(worker, el_true);
    } else {
      stop = el_true;
    }
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

audio_worker_t *worker = NULL;

int hook_func() {
  work_worker(worker, el_false);
}

int main(char *argv[],int argc) {
  mc_init();
  el_bool stop = el_false;
  hre_t re_load = hre_compile("^load(&play)?\\s+(.*)$","i");
  hre_t re_play = hre_compile("^play$","i");
  hre_t re_pause = hre_compile("^pause$","i");
  hre_t re_quit = hre_compile("^quit$","i");
  hre_t re_guard = hre_compile("^guard\\s([0-9]+([.][0-9]+)?)$","i");
  hre_t re_seek = hre_compile("^seek\\s([0-9]+([.][0-9]+)?)$","i");
  
  file_info_t *history_file = mc_take_over(file_info_new_home(".btb_play"));
  printf("history file: %s\n", file_info_absolute_path(history_file));
  printf("history file: %s\n", file_info_path(history_file));
  if (file_info_exists(history_file)) {
    read_history(file_info_absolute_path(history_file));
  }
  rl_event_hook = hook_func;
  
  audio_result_t result;
  
  i18n_set_language("nl");
  printf(_("Starting btb_play\n"));
  
  while (!stop) {
    char* line = readln(">", worker);
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
              mediafile = track_get_file(t);
            } else {
              array = tracks_from_media(file_info_absolute_path(info));
              mediafile = file_info_absolute_path(info);
            }
            if (worker == NULL) {
              worker = media_new_from_file(mediafile,&result);
            } else {
              media_load_file(worker, mediafile);
            }
            if (worker != NULL) 
              printf("loaded - length = %.3lf\n", media_length_in_ms(worker) / 1000.0 );
            
            if (track_array_count(array) > 0) {
              int i,N;
              for(i=0, N=track_array_count(array);i < N;++i) {
                track_t* t = track_array_get(array, i);
                log_track(t);
              }
            } else {
              log_error2("Cannot read metadata from %s", file_info_filename(info));
            }
            track_array_destroy(array);
          }
        }
      }
      file_info_destroy(info);      
      if (strcasecmp(play,"&play")==0) { media_play(worker); }
      mc_free(file);
      mc_free(play);
    } else if (hre_has_match(re_play, line)) {
        if (worker) media_play(worker);
    } else if (hre_has_match(re_pause, line)) {
        if (worker) media_pause(worker);
    } else if (hre_has_match(re_seek, line)) {
        if (worker) {
          hre_matches m = hre_match(re_seek, line);
          hre_match_t* match = hre_matches_get(m, 1);
          char* position = mc_strdup(hre_match_str(match));
          hre_matches_destroy(m);
          double s = atof(position);
          int ms = s * 1000;
          mc_free(position);
          media_seek(worker, ms);
        }
    } else if (hre_has_match(re_guard, line)) {
        if (worker) {
          hre_matches m = hre_match(re_guard, line);
          hre_match_t* match = hre_matches_get(m, 1);
          char* position = mc_strdup(hre_match_str(match));
          hre_matches_destroy(m);
          double s = atof(position);
          int ms = s * 1000;
          printf("guard %d, %.3lf\n", ms, s);
          mc_free(position);
          media_guard(worker, ms);
        }
    }
    
    mc_free(line);
    
    work_worker(worker, el_true);    
  }
  
  if (worker != NULL) {
    media_destroy(worker);
  }
  
  printf("%d\n",write_history(file_info_absolute_path(history_file)));
  
  file_info_destroy(history_file);
  hre_destroy(re_load);
  hre_destroy(re_play);
  hre_destroy(re_pause);
  hre_destroy(re_quit);
  hre_destroy(re_guard);
  hre_destroy(re_seek);
  
  return 0;
}

