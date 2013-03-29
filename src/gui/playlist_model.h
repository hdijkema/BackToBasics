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
  int col_index[PLAYLIST_MODEL_N_COLUMNS];
  int col_width[PLAYLIST_MODEL_N_COLUMNS];
  el_bool col_visible[PLAYLIST_MODEL_N_COLUMNS];
  long long hash;
  char* hash_as_str;
} col_properties_t;

DECLARE_HASH(col_property_hash, col_properties_t);

typedef struct {
  GtkListModel* model;
  playlist_t* playlist;
  set_t* valid_tracks;
  playlist_column_enum sort_col;
  col_property_hash* col_properties;
} playlist_model_t;

playlist_model_t* playlist_model_new(void);
void playlist_model_destroy(playlist_model_t* model);
GtkTreeModel* playlist_model_gtk_model(playlist_model_t* model);
void playlist_model_set_playlist(playlist_model_t* model, playlist_t* playlist);
playlist_t* playlist_model_get_selected_playlist(playlist_model_t* model);
track_t* playlist_model_get_track(playlist_model_t* model, int row);
const char* i18n_column_name(playlist_column_enum col);
const char* column_id(playlist_column_enum col);
void playlist_model_set_valid(playlist_model_t* model, set_t* tracks);
long long playlist_model_tracks_hash(playlist_model_t* model) ;
void playlist_model_sort(playlist_model_t* model, playlist_column_enum e);

el_bool playlist_model_save_col_properties(playlist_model_t* model, const char* filename);
el_bool playlist_model_load_col_properties(playlist_model_t* model, const char* filename);

col_properties_t* col_properties_new(long long hash);
void col_properties_destroy(col_properties_t* p);
const char* col_properties_key(const col_properties_t* p);

el_bool col_properties_write(const col_properties_t* p, FILE* f);
el_bool col_properties_read(col_properties_t* p, FILE* f);

int col_properties_get_width(const col_properties_t* p, int e);
el_bool col_properties_get_visible(const col_properties_t* p, int e);
int col_properties_get_index(const col_properties_t* p, int e);

void col_properties_set_width(col_properties_t* p, int e, int width);
void col_properties_set_visible(col_properties_t* p, int e, el_bool visible);
void col_properties_set_index(col_properties_t* p, int e, int index);

const col_properties_t* playlist_get_col_properties(playlist_model_t* model, long long hash);
void playlist_set_col_properties(playlist_model_t* model, long long hash, const col_properties_t* p);

#endif

