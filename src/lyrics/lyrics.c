#include <lyrics/lyrics.h>
#include <metadata/track.h>
#include <elementals.h>
#include <pthread.h>
#include <curl/curl.h>
#include <gtk/gtk.h>
#include <util/simple_html.h>

/*
Look also at:

wikia.com, lyricsplugin.com, lyricstime.com, lyricsreg.com, 
metrolyrics.com, seeklyrics.com, azlyrics.com, jamendo.com, 
darklyrics.com, mp3lyrics.org, songlyrics.com, elyrics.net, 
lyricsdownload.com, lyrics.com, lyriki.com, lyricsmode.com, 
lyricsbay.com, loudson.gs, lyricsfreak.com, sing365.com, 
allreggaelyrics.com, stixoi.info, teksty.org, tekstowo.pl, 
vagalume.uol.com.br, google.com



*/

struct fetcher {
  pthread_t thread;
  track_t*  track;
  void (*f)(char* lyric, void* data);
  char* lyric;
  void* data;
};

static gboolean callback(gpointer p) {
  struct fetcher* data = (struct fetcher*) p;
  data->f(mc_strdup(data->lyric), data->data);
  mc_free(data->lyric);
  track_destroy(data->track);
  mc_free(data);
  //pthread_join(data->thread, NULL);
  return FALSE;
}

static size_t write_data(void *buffer, size_t size, size_t nmemb, void *userp)
{
  memblock_t* block = (memblock_t*) userp;
  size_t s = size * nmemb;
  memblock_write(block, buffer, s);
  return nmemb;
}

static char* fetch_from_lyricsondemand_dot_com(struct fetcher* data)
{
  track_t* t = data->track;
  
  hre_t re_compress = hre_compile("(\\s|[,'\"?!.()_-])+","i");
  hre_t re_removenum = hre_compile("^[0-9]+","i");
  hre_t re_removeyear = hre_compile("[(][0-9]+[)]","i");
  hre_t re_and = hre_compile("[&]","");
  
  char* artist = mc_strdup(track_get_artist(t));
  char* title = mc_strdup(track_get_title(t));
  char* qa = hre_replace_all(re_removeyear, artist, "");
  char* qt = hre_replace_all(re_removeyear, title, "");
  char* qa1 = hre_replace_all(re_compress, qa, "");
  char* qt1 = hre_replace_all(re_compress, qt, "");
  char* qa1a = hre_replace_all(re_and, qa1, "and");
  char* qt1a = hre_replace_all(re_and, qt1, "and");

  char* query_artist;
  char* query_title;
  
  gsize bytes_written;
  GError* error = NULL;
  char* qa2 = g_convert(qa1a, strlen(qa1a),
                        "ASCII//TRANSLIT", "UTF-8", NULL, &bytes_written,
                        &error);
  if (error == NULL) {
    query_artist = mc_strdup(qa2);
    g_free(qa2);
  } else {
    log_debug2("conversion didn't succeed, %s", error->message);
    query_artist = mc_strdup(qa1a);
  }

  char* qt2 = g_convert(qt1a, strlen(qt1a),
                        "ASCII//TRANSLIT", "UTF-8", NULL, &bytes_written,
                        &error);
  if (error == NULL) {
    query_title =hre_replace(re_removenum, qt2, "");
    g_free(qt2);
  } else {
    log_debug2("conversion didn't succeed, %s", error->message);
    query_title = mc_strdup(qt1a);
  }
 
  mc_free(qa);mc_free(qt);mc_free(qa1);mc_free(qt1);
  mc_free(qa1a);mc_free(qt1a);
    
  hre_lc(query_artist);
  hre_lc(query_title);
    
  char url[10240];
  char first_char = query_artist[0];
  if (first_char >= '0' && first_char <= '9') { first_char = '0'; }
  snprintf(url, 10240, "http://www.lyricsondemand.com/%c/%slyrics/%slyrics.html",
           first_char, query_artist, query_title
           );
  log_debug2("fetching %s", url);
  CURL *curl;
  
  memblock_t* block = memblock_new();
 
  curl = curl_easy_init();
  curl_easy_setopt(curl, CURLOPT_URL, url);
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, block);
  curl_easy_perform(curl); /* ignores error */ 
  curl_easy_cleanup(curl);
  
  char c = '\0';
  memblock_write(block, &c, sizeof(char));
  
  char* buf = (char*) mc_malloc((memblock_size(block)+1) * sizeof(char));
  memblock_seek(block, 0);
  memblock_read(block, buf, memblock_size(block));
  
  char* p1 = strstr(buf, "<script>crbt1");
  data->lyric = NULL;
  if (p1) { 
    p1 = strstr(p1, "</script>");
    if (p1) {
      p1 += 9;
      char *p2 = strstr(p1, "<script");
      if (p2) {
        p2[0] = '\0';
        log_debug2("lyric: %s", p1);
        data->lyric = lyric_html_to_text(p1);
      }
    }
  }
      
  mc_free(buf);
  memblock_destroy(block);
  mc_free(artist);
  mc_free(title);
  mc_free(query_artist);
  mc_free(query_title);
  hre_destroy(re_compress);
  hre_destroy(re_removenum);
  hre_destroy(re_removeyear);
  hre_destroy(re_and);
  
  return data->lyric;
}

static char* read_lyric_from_local(struct fetcher* d)
{
  char* lyric = NULL;
  track_t* t = d->track;
  file_info_t* info = file_info_new(track_get_file(t));
  file_info_t* dn = file_info_new(file_info_dirname(info));
  char* s = (char*) mc_malloc( ( strlen(track_get_artist(t)) + sizeof("-") + strlen(track_get_title(t)) + sizeof(".lyric") + 1 ) * sizeof(char));
  sprintf(s,"%s-%s.lyric", track_get_artist(t), track_get_title(t));  
  file_info_t *lf = file_info_combine(dn, s);
  memblock_t* blk = memblock_new();
  if (file_info_exists(lf)) {
    FILE* f = fopen(file_info_absolute_path(lf), "rt");
    char buf[8192];
    size_t s;
    while ((s = fread(buf, 1, 8192, f)) > 0) {
      memblock_write(blk, buf, s);
    }
    fclose(f);
    char c = '\0';
    memblock_write(blk, &c, sizeof(c));
    lyric = mc_strdup(memblock_as_str(blk));
  } 
  memblock_destroy(blk);
  mc_free(s);
  file_info_destroy(info);
  file_info_destroy(dn);
  file_info_destroy(lf);
  return lyric;
}

void write_lyric(track_t* t, const char* lyric, el_bool overwrite) {
  file_info_t* info = file_info_new(track_get_file(t));
  file_info_t* dn = file_info_new(file_info_dirname(info));
  char* s = (char*) mc_malloc( ( strlen(track_get_artist(t)) + sizeof("-") + strlen(track_get_title(t)) + sizeof(".lyric") + 1 ) * sizeof(char));
  sprintf(s,"%s-%s.lyric", track_get_artist(t), track_get_title(t));  
  file_info_t *lf = file_info_combine(dn, s);
  if (!file_info_exists(lf) || overwrite) {
    FILE* f = fopen(file_info_absolute_path(lf), "wt");
    if (f != NULL) {
      fprintf(f, "%s", lyric);
      fclose(f);
    }
  }
  mc_free(s);
  file_info_destroy(info);
  file_info_destroy(dn);
  file_info_destroy(lf);
}

char* lyric_html_to_text(const char* lyric)
{
  log_debug("to html");
  return html_to_text(lyric);
}

char* lyric_text_to_html(const char* lyric) 
{
  char* r1;
  char* r2;
  r1 = text_to_html(lyric);
  r2 = hre_concat3("<html><head></head><body><span style=\"font-size: 7pt; font-family: sans;\">",
                   r1,
                   "</span></body></html>"
                   );
  mc_free(r1);
  return r2;
}

static void* fetch(void* tt)
{
  struct fetcher* data = (struct fetcher*) tt;
  char* lyric = read_lyric_from_local(data);
  
  if (lyric == NULL) {
    lyric = fetch_from_lyricsondemand_dot_com(data); 
    if (lyric == NULL) {
      // try other providers
      
      lyric = mc_strdup("");
    }
  }

  //printf("__RESULT__\n%s\n__RESULT_END\n",lyric);
  
  data->lyric = lyric;  
  gdk_threads_enter();
  g_timeout_add(10, callback, data);
  gdk_threads_leave();

  return NULL;
}


void fetch_lyric(track_t* t, void (*f)(char* lyric, void* data), void* dt)
{
  struct fetcher* data = (struct fetcher*) mc_malloc(sizeof(struct fetcher));
  data->track = track_copy(t);
  log_track(data->track);
  data->f = f;
  data->lyric = NULL;
  data->data = dt;
  int thread_id = pthread_create(&data->thread, NULL, fetch, data);
}



void lyrics_search_lyricsondemand(GtkButton* btn, GObject* data)
{
  track_t* t = g_object_get_data(data, "track");
  char s[10240];
  strcpy(s, "http://www.google.com/custom?q=%s&btnG=Search&domains=http://www.lyricsondemand.com&sitesearch=http://www.lyricsondemand.com&client=pub-1187925111528992");
  char url[20480];
  char query[10240];
  
  char* tt = stripped_title(t);
  sprintf(query,"%s+%s",track_get_artist(t), tt);
  mc_free(tt);
  char* q = to_http_get_query(query);

  sprintf(url, s, q);
  open_url(GTK_WIDGET(btn), url);
  
  mc_free(q);
}

void lyrics_search_lyricsworld(GtkButton* btn, GObject* data)
{
  track_t* t = g_object_get_data(data, "track");
  char s[10240];
  strcpy(s, "http://www.elyricsworld.com/search.php?search=%s");
  char url[20480];
  char query[10240];

  char* tt = stripped_title(t);
  sprintf(query,"%s+%s",track_get_artist(t), tt);
  mc_free(tt);
  hre_t re_wsp = hre_compile("\\s+","");
  char* q = hre_replace_all(re_wsp, query, "+");

  sprintf(url, s, q);
  open_url(GTK_WIDGET(btn), url);
  
  mc_free(q);
}

void lyrics_search_lyricsty(GtkButton* btn, GObject* data)
{
  track_t* t = g_object_get_data(data, "track");
  char s[10240];
  strcpy(s, "http://www.lyricsty.com/search.php?what=songs&s=%s");
  char url[20480];
  char query[10240];

  char* tt = stripped_title(t);
  sprintf(query,"%s+%s",track_get_artist(t), tt);
  mc_free(tt);
  char* q = to_http_get_query(query);

  sprintf(url, s, q);
  open_url(GTK_WIDGET(btn), url);
  
  mc_free(q);
}

void lyrics_search_google(GtkButton* btn, GObject* data)
{
  track_t* t = g_object_get_data(data, "track");
  char s[10240];
  strcpy(s, "https://www.google.nl/search?q=%s+lyrics");
  char url[20480];
  char query[10240];

  char* tt = stripped_title(t);
  sprintf(query,"%s+%s",track_get_artist(t), tt);
  mc_free(tt);
  char* q = to_http_get_query(query);

  sprintf(url, s, q);
  open_url(GTK_WIDGET(btn), url);
  
  mc_free(q);
}


