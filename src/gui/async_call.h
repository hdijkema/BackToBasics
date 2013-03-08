#ifndef ASYNC_CALL_HOD
#define ASYNC_CALL_HOD

typedef struct {
  void (*f)(void* data);
  void* data;
} async_data_t;

void call_async(void (*f)(void* data), void* data);

#define CALL_ASYNC(f, d) call_async((void (*)(void*)) f, (void*) d)

#endif
