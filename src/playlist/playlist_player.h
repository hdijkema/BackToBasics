#ifndef __PLAYLIST_PLAYER__HOD
#define __PLAYLIST_PLAYER__HOD

#include <playlist/playlist.h>

playlist_player_t *playlist_player_new();

void playlist_player_set_playlist(playlist_player_t* player, playlist_t* playlist);
void playlist_player_play(playlist_player_t* player, int index);
void playlist_player_next(playlist_player_t* player);
void playlist_player_previous(playlist_player_t* player);
int  playlist_player_index(playlist_player_t* player);
long playlist_player_current_offset_in_ms(playlist_player_t* player);
track_t playlist_player_current_track(playlist_player_t* player);

#endif
