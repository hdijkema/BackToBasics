#ifndef __PLAYLIST_PLAYER__HOD
#define __PLAYLIST_PLAYER__HOD

#include <playlist/playlist.h>
#include <audio/audio.h>

typedef enum {
  PLAYLIST_PLAYER_STOPPED,
  PLAYLIST_PLAYER_PLAYING,
  PLAYLIST_PLAYER_PAUSED,
} playlist_player_state_t;

typedef enum {
  PLP_CMD_DESTROY = 0,
  
  PLP_CMD_PLAY = 1,
  PLP_CMD_PAUSE = 2,
  
  PLP_CMD_SET_TRACK = 3,
  PLP_CMD_SEEK = 4,
  PLP_CMD_NEXT = 5,
  PLP_CMD_PREVIOUS = 6,
  PLP_CMD_RESTART_TRACK = 7,

  PLP_CMD_NO_REPEAT = 8,
  PLP_CMD_TRACK_REPEAT = 9,
  PLP_CMD_LIST_REPEAT = 10,
  
  PLP_CMD_SET_PLAYLIST = 11,
  
  PLP_CMD_SET_VOLUME = 14,
  
  PLP_CMD_NONE = 20
  
} playlist_player_cmd_enum;

typedef enum {
  PLP_NO_REPEAT = 0,
  PLP_REPEAT_OFF = 0,
  PLP_TRACK_REPEAT = 1,
  PLP_LIST_REPEAT = 2
} playlist_player_repeat_t;

typedef struct {
  playlist_player_cmd_enum cmd;
  void* data;
} playlist_player_cmd_t;

DECLARE_FIFO(playlist_player_command_fifo, playlist_player_cmd_t);

typedef struct {
  audio_worker_t* worker;
  playlist_t* playlist;
 
  playlist_player_command_fifo* player_control;
  
  int current_track;
  long current_position_in_ms;
  long track_position_in_ms;
  
  el_bool track_changed;
  playlist_player_state_t player_state;
  playlist_player_repeat_t repeat;
  
  double volume_percentage;
  
  long long playlist_hash;
  
  pthread_mutex_t *mutex;
  pthread_t playlist_player_thread;
  
} playlist_player_t;

playlist_player_t *playlist_player_new(void);
void playlist_player_destroy(playlist_player_t* plp);

void playlist_player_set_playlist(playlist_player_t* player, playlist_t* playlist);

void playlist_player_play(playlist_player_t* player);
void playlist_player_pause(playlist_player_t* player);
void playlist_player_set_track(playlist_player_t* player, int track);
void playlist_player_next(playlist_player_t* player);
void playlist_player_previous(playlist_player_t* player);
void playlist_player_again(playlist_player_t* player);
void playlist_player_seek(playlist_player_t* player, long position_in_ms);
void playlist_player_set_repeat(playlist_player_t* player, playlist_player_repeat_t repeat);

int  playlist_player_get_track_index(playlist_player_t* player);
track_t* playlist_player_get_track(playlist_player_t* player);

long playlist_player_get_current_position_in_ms(playlist_player_t* player);
long playlist_player_get_track_position_in_ms(playlist_player_t* player);

long long playlist_player_get_hash(playlist_player_t* player);

el_bool playlist_player_get_track_changed(playlist_player_t* player);

playlist_t* playlist_player_get_playlist(playlist_player_t* player);

el_bool playlist_player_is_playing(playlist_player_t* plp);
el_bool playlist_player_is_paused(playlist_player_t* plp);
el_bool playlist_player_does_nothing(playlist_player_t* plp);

playlist_player_repeat_t playlist_player_get_repeat(playlist_player_t* plp);

double playlist_player_get_volume(playlist_player_t* plp);
void playlist_player_set_volume(playlist_player_t* plp, double percentage);



#endif
