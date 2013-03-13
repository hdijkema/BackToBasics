#ifndef STATION_MODEL_HOD
#define STATION_MODEL_HOD

#include <gtk/gtk.h>
#include <gtklistmodel.h>
#include <library/radio.h>

typedef enum {
  STATION_MODEL_COL_NAME = 0,
  STATION_MODEL_COL_RECORDING, 
  STATION_MODEL_N_COLUMNS
} station_column_enum;

typedef struct {
  GtkListModel* model;
  radio_library_t* library;
} station_model_t;

station_model_t* station_model_new(radio_library_t* library);
void station_model_destroy(station_model_t* model);
GtkTreeModel* station_model_gtk_model(station_model_t* model);

const char* station_model_i18n_column_name(station_column_enum col);
const char* station_model_column_id(station_column_enum col);

void station_model_library_changed(station_model_t* model);


#endif
