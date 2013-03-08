#include <gtk/gtk.h>
#include <gui/async_call.h>
#include <elementals.h>

#define ASYNC_MS 20

static int asyncfunc(void* d)
{
  async_data_t* data = (async_data_t*) d;
  data->f(data->data);
  mc_free(d);
  return FALSE;
}

void call_async(void (*f)(void*), void* _data)
{
  async_data_t* data = (async_data_t*) mc_malloc(sizeof(async_data_t));
  data->f = f;
  data->data = _data;
  g_timeout_add(ASYNC_MS, asyncfunc, (gpointer) data);  
}
