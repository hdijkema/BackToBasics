#include <audio/aodev.h>
#include <elementals.h>


ao_t* aodev_new(void) 
{
  static int initialize = 1;
  if (initialize) {
    initialize = 0;
    ao_initialize();
    atexit(ao_shutdown);
  }
  
  ao_t* handle = (ao_t*) mc_malloc(sizeof(ao_t));
  handle->driver = ao_default_driver_id();
  
  return handle;
}

void aodev_set_format(ao_t* handle, int bits, int rate, int channels) 
{
  handle->format.bits = bits;
  handle->format.rate = rate;
  handle->format.channels = channels;
  handle->format.byte_format = AO_FMT_NATIVE;
  handle->format.matrix = 0;
}

void aodev_open(ao_t* handle)
{
  handle->device = ao_open_live(handle->driver, &handle->format, NULL);
}

void aodev_play_buffer(ao_t* handle, void* buffer, long size)
{
  ao_play(handle->device, buffer, size);
}

void aodev_close(ao_t* handle)
{
  ao_close(handle->device);
  handle->device = NULL;
}

void aodev_destroy(ao_t* handle)
{
  if (handle->device != NULL) {
    aodev_close(handle);
  }
  mc_free(handle);
}

