#include <gui/playlists_model.h>
#include <i18n/i18n.h>

static int n_columns(void* data)
{
  return (int) PLAYLISTS_MODEL_N_COLUMNS;
}

static GType column_type(void* data, int col)
{
  switch(col) {
    case PLAYLISTS_MODEL_COL_NAME: return G_TYPE_STRING;
    default: return G_TYPE_NONE;
  }
}

static int n_rows(void* data)
{
  playlists_model_t* model = (playlists_model_t*) data;
  if (model->library != NULL) {
    return library_playlists_count(model->library);
  } else {
    return 0;
  }
}

static void cell_value(void* data, int row, int col, GValue* value)
{
  playlists_model_t* model = (playlists_model_t*) data;
  
  if (model->library == NULL) {
    return;
  }
  
  playlist_t* pl = library_playlists_get(model->library, row);
  switch(col) {
    case PLAYLISTS_MODEL_COL_NAME: g_value_set_static_string(value, playlist_name(pl));
    break;
    default:
    break;
  }
}

const char* playlists_model_i18n_column_name(playlists_column_enum e)
{
  switch (e) {
    case PLAYLISTS_MODEL_COL_NAME: return _("Playlist");
    default: return "";
  }
}

const char* playlists_model_column_id(playlists_column_enum e)
{
  switch (e) {
    case PLAYLISTS_MODEL_COL_NAME: return "playlists_col_name";
    default: return "col_none";
  }
}

playlists_model_t* playlists_model_new(library_t* lib) 
{
  playlists_model_t* model = (playlists_model_t*) mc_malloc(sizeof(playlists_model_t));
  model->library = lib;
  model->model = gtk_list_model_new(model, n_columns, column_type, n_rows, cell_value);
  return model;
}

void playlists_model_destroy(playlists_model_t* model) 
{
  gtk_list_model_destroy(model->model);
  mc_free(model);
}

GtkTreeModel* playlists_model_gtk_model(playlists_model_t* model)
{
  return GTK_TREE_MODEL(model->model);
}


