#include <elementals.h>
#include <util/simple_html.h>

char* html_to_text(const char* in)
{
  hre_t* re_wsp = hre_compile("\\s","");
  hre_t* re_br = hre_compile("[<]br[^>]*[>]","i");
  hre_t* re_p = hre_compile("<p>","i");
  hre_t* re_tag = hre_compile("[<][^>]*[>]","i");
  hre_t* re_amp = hre_compile("[&]amp[;]","i");
  hre_t* re_lt = hre_compile("[&]lt[;]","i");
  hre_t* re_gt = hre_compile("[&]gt[;]","i");
  hre_t* re_quot = hre_compile("[&]quot[;]","i");
  hre_t* res[] = { re_wsp, re_br, re_p, re_tag,  re_amp, re_lt, re_gt, re_quot, NULL };
  char* repl[] = { " ",    "\n",  "\n\n", "",      "&",    "<",   ">",   "\"",    NULL };
  int i;
  char* r1, *r2;
  r1 = mc_strdup(in);
  for(i = 0;res[i] != NULL; ++i) {
    r2 = hre_replace_all(res[i], r1, repl[i]);
    mc_free(r1);
    hre_destroy(res[i]);
    r1 = r2;
  }
  hre_trim(r1);
  return r1;
}

char* text_to_html(const char* in)
{
  hre_t* re_ret = hre_compile("\n","sm");
  hre_t* re_amp = hre_compile("[&]","");
  hre_t* re_lt = hre_compile("[<]","");
  hre_t* re_gt = hre_compile("[>]","");
  hre_t* re_quot = hre_compile("[\"]","");
  hre_t* res[] = { re_amp, re_lt, re_gt, re_quot, re_ret, NULL };
  char* repl[] = { "&amp;","&lt;", "&gt;", "&quot;", "<br />", NULL };
  int i;
  char* r1, *r2;
  r1 = mc_strdup(in);
  for(i = 0;res[i] != NULL; ++i) {
    r2 = hre_replace_all(res[i], r1, repl[i]);
    mc_free(r1);
    hre_destroy(res[i]);
    r1 = r2;
  }
  return r1;
}

