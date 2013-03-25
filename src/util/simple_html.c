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


char* stripped_title(track_t* t)
{
  const char* title = track_get_title(t);
  hre_t* re = hre_compile("^\\s*[0-9]*\\s*[-]\\s*","");
  char* tt = hre_replace(re, title, "");
  return tt;
}

void open_url(GtkWidget* w, const char* url)
{
  GdkScreen *screen;
  GError *error;

  if (gtk_widget_has_screen (w))
    screen = gtk_widget_get_screen (w);
  else
    screen = gdk_screen_get_default ();

  error = NULL;
  gtk_show_uri (screen, url,
                gtk_get_current_event_time (),
                &error);
  if (error) {
    log_debug2("%s", error->message);
  }
}

char* to_http_get_query(const char* query)
{
  hre_t re_amp = hre_compile("&","");
  hre_t re_wsp = hre_compile("\\s+","");
  char* q = hre_replace_all(re_wsp, query, "+");
  char* q1 = hre_replace_all(re_amp, q, "%26");
  mc_free(q);
  hre_destroy(re_wsp);
  hre_destroy(re_amp);
  return q1;  
}
