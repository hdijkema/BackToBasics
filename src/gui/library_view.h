#ifndef GUI_LIBRARY_VIEW
#define GUI_LIBRARY_VIEW

#include <gtk/gtk.h>
#include <backtobasics.h>
#include <gui/playlist_model.h>
#include <gui/string_model.h>
#include <elementals.h>

typedef enum {
  GENRE_ASPECT = 0,
  ARTIST_ASPECT,
  ALBUM_ASPECT
} aspect_enum;

typedef struct {
  Backtobasics* btb;
  GtkBuilder* builder;
  
  playlist_model_t* playlist_model;
  el_bool library_list_changed;
  long long library_list_hash;
  
  string_model_t* genre_model;
  string_model_t* artist_model;
  string_model_t* album_model;
  
  library_t* library;
  aspect_enum aspect;
  
  el_bool run_timeout;
  
  long time_in_ms;
  long len_in_ms;
  int track_index;
  long track_id;
  el_bool sliding;
  long long playing_list_hash;
  
  int img_w;
  int img_h;
  
  GtkWidget* btn_play;
  GtkWidget* btn_pause;
  GtkTreeViewColumn** cols;
  GtkTreeView* tview;
  
} library_view_t;

library_view_t* library_view_new(Backtobasics* btb, library_t* library);
void library_view_init(library_view_t* view);
void library_view_destroy(library_view_t* view);
void library_view_genre_aspect(library_view_t* view);
void library_view_artist_aspect(library_view_t* view);
void library_view_album_aspect(library_view_t* view);

void library_view_stop_info_updater(library_view_t* view);

#endif
