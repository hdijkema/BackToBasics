#include <gui/string_model.h>

static char* copystr(char* s)
{
  return mc_strdup(s);
}

static void destroystr(char* s)
{
  mc_free(s);
}

IMPLEMENT_EL_ARRAY(string_model_array, char, copystr, destroystr);

static int n_columns(void* data)
{
  return 1;
}

static GType column_type(void* data, int col)
{
  if (col == 0) { return G_TYPE_STRING; }
  else { return G_TYPE_NONE; }
}

static int n_rows(void* data)
{
  string_model_t* model = (string_model_t*) data;
  //log_debug2("nrows, returning %d", string_model_array_count(model->array));
  return string_model_array_count(model->array);
}

static void cell_value(void* data, int row, int col, GValue* value)
{
  string_model_t* model = (string_model_t*) data;
  if (col!=0) { return; }
  g_value_set_static_string(value, string_model_array_get(model->array, row));
}

static gboolean filter(void* strmodel, int row)
{
  string_model_t* m = (string_model_t*) strmodel;
  if (m->valid_strings == NULL) {
    return TRUE;
  } else {
    if (set_contains(m->valid_strings, string_model_array_get(m->array, row))) {
      return TRUE;
    } else {
      return FALSE;
    }
  }
}

string_model_t* string_model_new()
{
  string_model_t* model = (string_model_t*) mc_malloc(sizeof(string_model_t));
  model->array = mc_take_over(string_model_array_new());
  model->model = gtk_list_model_new(model, n_columns, column_type, n_rows, cell_value);
  model->destroy = el_true;
  model->valid_strings = NULL;
  gtk_list_model_set_filter(model->model, filter);
  return model;
}

void string_model_destroy(string_model_t* model)
{
  gtk_list_model_destroy(model->model);
  if (model->destroy) string_model_array_destroy(model->array);
  mc_free(model);
}

void string_model_set_valid(string_model_t* model, set_t* valid)
{
  model->valid_strings = valid;
  gtk_list_model_refilter(model->model);
}

void string_model_set_array(string_model_t* model, string_model_array a, el_bool destroy)
{
  if (model->destroy) string_model_array_destroy(model->array);
  model->array = a;
  model->destroy = destroy;
  gtk_list_model_refilter(model->model);
}

string_model_array string_model_get_array(string_model_t* model)
{
  return model->array;
}

GtkTreeModel *string_model_gtk_model(string_model_t* model)
{
  return GTK_TREE_MODEL(model->model);
}

