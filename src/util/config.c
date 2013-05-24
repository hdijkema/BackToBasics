#include <util/config.h>
#include <libconfig.h>

struct __el_config__ {
  config_t cfg;
}; 

static config_setting_t* get_setting(config_t* cfg, const char* path, int type);

/*****************************************************************************
 * Implementation
 *****************************************************************************/

el_config_t* el_config_new(void)
{
  el_config_t* c = (el_config_t*) mc_malloc(sizeof(struct __el_config__));
  config_init(&c->cfg);
  return c;
}

void el_config_destroy(el_config_t* cfg)
{
  config_destroy(&cfg->cfg);
  mc_free(cfg);
}

void el_config_save(el_config_t* cfg, const char* filename)
{
  config_write_file(&cfg->cfg, filename);
}

el_bool el_config_load(el_config_t* cfg, const char* filename)
{
  if (config_read_file(&cfg->cfg, filename) == CONFIG_TRUE) {
    return el_true;
  } else {
    return el_false;
  }
}

int el_config_get_int(el_config_t* cfg, const char* path, int default_val)
{
  int v = default_val;
  config_t* config = &cfg->cfg;
  config_lookup_int(config, path, &v);
  return (int) v;
}

double el_config_get_double(el_config_t* cfg, const char* path, double default_val)
{
  double v = default_val;
  config_t* config = &cfg->cfg;
  config_lookup_float(config, path, &v);
  return v;
}

char* el_config_get_string(el_config_t* cfg, const char* path, const char* default_val)
{
  const char* val = NULL;
  config_t* config = &cfg->cfg;
  config_lookup_string(config, path, &val);
  if (val == NULL) {
    return mc_strdup(default_val);
  } else {
    return mc_strdup(val);
  }
}

void el_config_set_int(el_config_t* cfg, const char* path, int val)
{
  config_setting_t* setting = get_setting(&cfg->cfg, path, CONFIG_TYPE_INT);
  config_setting_set_int(setting, val);
}

void el_config_set_double(el_config_t* cfg, const char* path, double val)
{
  config_setting_t* setting = get_setting(&cfg->cfg, path, CONFIG_TYPE_INT);
  config_setting_set_float(setting, val);
}

void el_config_set_string(el_config_t* cfg, const char* path, const char* val)
{
  config_setting_t* setting = get_setting(&cfg->cfg, path, CONFIG_TYPE_STRING);
  config_setting_set_string(setting, val);
}


/*****************************************************************************
 * Internal
 *****************************************************************************/

static config_setting_t* get_setting(config_t* cfg, const char* path, int type)
{
  config_setting_t *root, *setting;
  
  root = config_root_setting(cfg);
  char* pt = mc_strdup(path);
  char* p[100];
  int k = 0, i, l;
  p[k++] = &pt[0];
  for(i = 0, l = strlen(pt);i < l; ++i) {
    if (pt[i] == '.') {
      pt[i] = '\0';
      p[k] = &pt[i+1];
      k += 1;
    }
  }
  p[k++] = NULL;
  
  setting = root;
  for(i = 0; p[i] != NULL; ++i) {
    int t = CONFIG_TYPE_GROUP;
    if (p[i+1] == NULL) { t = type; }
    config_setting_t* tmp = setting;
    setting = config_setting_get_member(tmp, p[i]);
    if (!setting) 
      setting = config_setting_add(tmp, p[i], t);
  }
  
  mc_free(pt);

  return setting;
}

