#ifndef __AO__HOD
#define __AO__HOD

#include <ao/ao.h>

typedef struct {
  ao_device *device;
  int driver;
  ao_sample_format format;
} ao_t;

ao_t* aodev_new(void);
void aodev_set_format(ao_t* ao, int bits, int rate, int channels);
void aodev_set_endian(ao_t* ao, int endian);
void aodev_open(ao_t* ao);
void aodev_play_buffer(ao_t* ao,void *buffer, long size);
void aodev_close(ao_t* ao);
void aodev_destroy(ao_t* ao);

#endif
