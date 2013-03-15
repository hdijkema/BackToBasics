#include <gui/station_model.h>
#include <library/radio.h>
#include <i18n/i18n.h>

static int n_columns(void* data)
{
  return (int) STATION_MODEL_N_COLUMNS;
}

static GType column_type(void* data, int col)
{
  switch(col) {
    case STATION_MODEL_COL_NAME: return G_TYPE_STRING;
    case STATION_MODEL_COL_RECORDING: return G_TYPE_BOOLEAN;
    default: return G_TYPE_NONE;
  }
}

static int n_rows(void* data)
{
  station_model_t* model = (station_model_t*) data;
  if (model->library != NULL) {
    return radio_library_count(model->library);
  } else {
    return 0;
  }
}

static void cell_value(void* data, int row, int col, GValue* value)
{
  station_model_t* model = (station_model_t*) data;
  
  if (model->library == NULL) {
    return;
  }
  
  radio_t* t = radio_library_get(model->library, row);
  switch(col) {
    case STATION_MODEL_COL_NAME: g_value_set_static_string(value, radio_name(t));
    break;
    case STATION_MODEL_COL_RECORDING: g_value_set_boolean(value, radio_is_recording(t));
    break;
    default:
    break;
  }
}

const char* station_model_i18n_column_name(station_column_enum col)
{
  switch(col) {
    case STATION_MODEL_COL_NAME: return _("Name");
    case STATION_MODEL_COL_RECORDING: return _("Record");
    default: return "";
  }
}

const char* station_model_column_id(station_column_enum col)
{
  switch(col) {
    case STATION_MODEL_COL_NAME: return "station_model_col_name";
    case STATION_MODEL_COL_RECORDING: return "station_model_col_recording";
    default: return "station_model_col_none";
  }
}

station_model_t* station_model_new(radio_library_t* library) 
{
  station_model_t* model = (station_model_t*) mc_malloc(sizeof(station_model_t));
  model->library = library;
  model->model = gtk_list_model_new(model, 
                                    n_columns,
                                    column_type,
                                    n_rows,
                                    cell_value
                                    );
  return model;
}

void station_model_destroy(station_model_t* model)
{
  gtk_list_model_destroy(model->model);
  mc_free(model);
}

GtkTreeModel* station_model_gtk_model(station_model_t* model)
{
  return GTK_TREE_MODEL(model->model);
}

void station_model_library_changed(station_model_t* model) 
{
  gtk_list_model_refilter(model->model);
}


