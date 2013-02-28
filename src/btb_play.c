#include <elementals.h>
#include <stdio.h>
#ifdef HAVE_READLINE
#include <readline.h>
#else
#error Needs readline
#endif
#include <audio/audio.h>

char *readln(const char *prompt) 
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
  }
  if (line != NULL) {
    add_history(line);
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

int main(char *argv[],int argc) {
  el_bool stop = el_false;
  hre_t re_load = hre_compile("^load\\s+(.*)$","i");
  hre_t re_play = hre_compile("^play$","i");
  hre_t re_pause = hre_compile("^pause$","i");
  hre_t re_quit = hre_compile("^quit$","i");
  hre_t re_gard = hre_compile("^gard\\s([0-9]+([.][0-9]+)?)$","i");
  hre_t re_seek = hre_compile("^seek\\s([0-9]+([.][0-9]+)?)$","i");
  
  file_info_t *history_file = file_info_new_home(".btb_play");
  printf("history file: %s\n", file_info_absolute_path(history_file));
  printf("history file: %s\n", file_info_path(history_file));
  if (file_info_exists(history_file)) {
    read_history(file_info_absolute_path(history_file));
  }
  
  audio_worker_t *worker = NULL;
  audio_result_t result;
  
  while (!stop) {
    char* line = readln(">");
    hre_trim(line);
    if (hre_has_match(re_quit, line)) {
      stop = el_true;
    } else if (hre_has_match(re_load, line)) {
      hre_matches m = hre_match(re_load, line);
      hre_match_t *match = hre_matches_get(m, 1);
      char* file = mc_strdup(hre_match_str(match));
      hre_matches_destroy(m);
      printf("loading '%s'\n", file);
      file_info_t *info = file_info_new(file);
      if (file_info_exists(info)) {
        if (file_info_is_file(info)) {
          if (file_info_can_read(info)) {
            if (worker == NULL) {
              worker = media_new_from_file(file_info_absolute_path(info),&result);
            } else {
              media_load_file(worker, file_info_absolute_path(info));
            }
            printf("loaded - length = %.3lf\n", media_length_in_ms(worker) / 1000.0 );
          }
        }
      }
      file_info_destroy(info);      
      mc_free(file);
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
    } else if (hre_has_match(re_gard, line)) {
        if (worker) {
          hre_matches m = hre_match(re_gard, line);
          hre_match_t* match = hre_matches_get(m, 1);
          char* position = mc_strdup(hre_match_str(match));
          hre_matches_destroy(m);
          double s = atof(position);
          int ms = s * 1000;
          printf("gard %d, %.3lf\n", ms, s);
          mc_free(position);
          media_gard(worker, ms);
        }
    }
    
    free(line);
    
    if (worker != NULL) {
      audio_state_t st = AUDIO_NONE;
      while (media_peek_event(worker)) {
        audio_event_t* event = media_get_event(worker);
        if (audio_event_state(event) != st) {
          if (st != AUDIO_NONE) printf("\n");
          st = audio_event_state(event);
        }
        printf("%02d %06.03lf\r", audio_event_state(event), audio_event_ms(event) / 1000.0);
        audio_event_destroy(event);
      }
      printf("\n");
    }
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
  hre_destroy(re_gard);
  hre_destroy(re_seek);
  
  return 0;
}
