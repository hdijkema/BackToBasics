#include <i18n/i18n.h>
#include <elementals.h>
#include <stdarg.h>

static char *languages[] = { "en", "nl", NULL };
static int  lang_index = 0;

static char** copy(char** s) {
  int i,j; 
  for(i = 0;s[i]!=NULL;i++);
  i+=1; 
  char** s1 = (char**) mc_malloc(sizeof(char*) * i);
  for(j = 0;j < i;j++) {
    s1[j]=s[j];
  }
  return s1;
}

static void destroy(char** s) {
  mc_free(s);
}

STATIC_DECLARE_HASH(lang_hash, char*);
STATIC_IMPLEMENT_HASH(lang_hash, char*, copy, destroy);

static lang_hash* H = NULL; 

void i18n_set_language(const char* lang)
{
  int i;
  for(i = 0;languages[i] != NULL && strcasecmp(languages[i], lang) != 0;++i);
  if (languages[i] == NULL) {
    lang_index = 0;
  } else {
    lang_index = i;
  }
}

static void add(const char *s, ...)
{
  va_list valist;
  va_start(valist, s);
  char* args[1000];
  int i = 0;
  args[i++] = (char*) s;
  char *a;
  while ((a = va_arg(valist, char*)) != NULL) {
    args[i++] = a;
  }
  args[i++] = NULL;
  va_end(valist);
  lang_hash_put(H,args[0],args);
}

void i18n_cleanup(void)
{
  lang_hash_destroy(H);
}

void i18n_init(void)
{
  if (H != NULL) 
    return;
  
  atexit(i18n_cleanup);
  H = lang_hash_new(10, HASH_CASE_SENSITIVE);
  
  add("Starting btb_play\n", "Starten btb_play...\n", NULL);
  add("Untitled", "Geen naam", NULL);
}

const char* _(const char* str) {
  i18n_init();
  char** result = lang_hash_get(H, (char*) str);
  if (result == NULL) {
    log_debug2("Unknown i18n string '%s'", str);
    return str;
  }
  return result[lang_index];
}


