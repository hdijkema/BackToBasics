#include <audio/aodev.h>
#include <elementals.h>


ao_t* aodev_new(void) 
{
  log_debug("new");
  static int initialize = 1;
  if (initialize) {
    initialize = 0;
    ao_initialize();
    atexit(ao_shutdown);
  }
  
  ao_t* handle = (ao_t*) mc_malloc(sizeof(ao_t));
  handle->driver = ao_default_driver_id();
  handle->device = NULL;
  
  return handle;
}

void aodev_set_format(ao_t* handle, int bits, int rate, int channels) 
{
  log_debug("format");
  handle->format.bits = bits;
  handle->format.rate = rate;
  handle->format.channels = channels;
  handle->format.byte_format = AO_FMT_NATIVE;
  handle->format.matrix = 0;
}

void aodev_set_endian(ao_t* handle, int ao_fmt) {
  log_debug("endian");
  handle->format.byte_format = ao_fmt;
}

void aodev_open(ao_t* handle)
{
  log_debug("open");
  handle->device = ao_open_live(handle->driver, &handle->format, NULL);
}

void aodev_play_buffer(ao_t* handle, void* buffer, long size)
{
  ao_play(handle->device, buffer, size);
}

void aodev_close(ao_t* handle)
{
  log_debug("close");
  ao_close(handle->device);
  handle->device = NULL;
}

void aodev_destroy(ao_t* handle)
{
  log_debug("destroy");
  if (handle->device != NULL) {
    aodev_close(handle);
  }
  mc_free(handle);
}

el_bool aodev_is_open(ao_t* handle)
{
  return handle->device != NULL;
}
