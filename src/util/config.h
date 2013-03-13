#ifndef __CONFIG__HOD
#define __CONFIG__HOD

#include <elementals.h>

typedef struct __el_config__ el_config_t; 

el_config_t* el_config_new(void);
void el_config_destroy(el_config_t* cfg);

void el_config_save(el_config_t* cfg, const char* filename);
el_bool el_config_load(el_config_t* cfg, const char* filename);

void el_config_set_int(el_config_t* cfg, const char* path, int val);
void el_config_set_double(el_config_t* cfg, const char* path, double val);
void el_config_set_string(el_config_t* cfg, const char* path, const char* str);

char* el_config_get_string(el_config_t* cfg, const char* path, const char* default_val);
int el_config_get_int(el_config_t* cfg, const char* path, int default_val);
double el_config_get_double(el_config_t* cfg, const char* path, double default_val);

#endif