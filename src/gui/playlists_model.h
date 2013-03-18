#ifndef __PLAYLISTS_MODEL__HOD
#define __PLAYLISTS_MODEL__HOD

#include <gtk/gtk.h>
#include <gtklistmodel.h>
#include <library/library.h>

typedef enum {
  PLAYLISTS_MODEL_COL_NAME = 0,
  PLAYLISTS_MODEL_N_COLUMNS
} playlists_column_enum;

typedef struct {
  GtkListModel* model;
  library_t* library;
} playlists_model_t;

playlists_model_t* playlists_model_new(library_t* lib);
void playlists_model_destroy(playlists_model_t* model);

GtkTreeModel* playlists_model_gtk_model(playlists_model_t* model);
const char* playlists_model_i18n_column_name(playlists_column_enum e);
const char* playlists_model_column_id(playlists_column_enum e);


#endif

