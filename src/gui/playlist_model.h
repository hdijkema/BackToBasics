#ifndef PLAYLIST_MODEL_HOD
#define PLAYLIST_MODEL_HOD
 
#include <gtk/gtk.h>
#include <gtklistmodel.h>
#include <playlist/playlist.h>


typedef enum {
  PLAYLIST_MODEL_COL_NR = 0,
  PLAYLIST_MODEL_COL_TITLE,
  PLAYLIST_MODEL_COL_ARTIST,
  PLAYLIST_MODEL_COL_COMPOSER, 
  PLAYLIST_MODEL_COL_PIECE,
  PLAYLIST_MODEL_COL_ALBUM_TITLE,
  PLAYLIST_MODEL_COL_ALBUM_ARTIST,
  PLAYLIST_MODEL_COL_GENRE,
  PLAYLIST_MODEL_COL_YEAR,
  PLAYLIST_MODEL_COL_LENGTH,
  PLAYLIST_MODEL_N_COLUMNS
} playlist_column_enum;
 

typedef struct {
  GtkListModel* model;
  playlist_t* playlist;
  set_t* valid_tracks;
  playlist_column_enum sort_col;
} playlist_model_t;

playlist_model_t* playlist_model_new();
void playlist_model_destroy(playlist_model_t* model);
GtkTreeModel* playlist_model_gtk_model(playlist_model_t* model);
void playlist_model_set_playlist(playlist_model_t* model, playlist_t* playlist);
playlist_t* playlist_model_get_selected_playlist(playlist_model_t* model);
const char* i18n_column_name(playlist_column_enum col);
const char* column_id(playlist_column_enum col);
void playlist_model_set_valid(playlist_model_t* model, set_t* tracks);
long long playlist_model_tracks_hash(playlist_model_t* model) ;
void playlist_model_sort(playlist_model_t* model, playlist_column_enum e);

#endif

