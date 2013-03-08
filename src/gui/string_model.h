#ifndef STRING_MODEL_HOD
#define STRING_MODEL_HOD

#include <gtk/gtk.h>
#include <gtklistmodel.h>
#include <elementals.h>

DECLARE_EL_ARRAY(string_model_array, char);

typedef struct {
  string_model_array array;
  set_t* valid_strings;
  GtkListModel* model;
  el_bool destroy;
} string_model_t;

string_model_t* string_model_new();
GtkTreeModel* string_model_gtk_model(string_model_t* model);
void string_model_destroy(string_model_t* model);
void string_model_set_array(string_model_t* model, string_model_array a, el_bool destroy);
string_model_array string_model_get_array(string_model_t* model);
void string_model_set_valid(string_model_t* model, set_t* valid); // NULL is no filtering

#endif

