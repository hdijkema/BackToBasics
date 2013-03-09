#include <lyrics/lyrics.h>
#include <metadata/track.h>
#include <elementals.h>
#include <pthread.h>
#include <curl/curl.h>
#include <gtk/gtk.h>

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
  void (*f)(const char* lyric, void* data);
  char* lyric;
  void* data;
};

static gboolean callback(gpointer p) {
  struct fetcher* data = (struct fetcher*) p;
  data->f(data->lyric, data->data);
  mc_free(data->lyric);
  track_destroy(data->track);
  mc_free(data);
  //pthread_join(data->thread, NULL);
  return FALSE;
}

size_t write_data(void *buffer, size_t size, size_t nmemb, void *userp)
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
  hre_t re_changebr = hre_compile("[<]br[^>]*[>]","i");
  hre_t re_removetag = hre_compile("[<][^>]*[>]","i");
  hre_t re_removeempty = hre_compile("\n+", "m");
  hre_t re_changebr1 = hre_compile("\\s*___BR___\\s*","");
  hre_t re_changebr2 = hre_compile("___BR___\\s*___BR___","");
  hre_t re_changebr3 = hre_compile("___PAR___","");
  hre_t re_changebr4 = hre_compile("__BRBR__","");
  
  char* artist = mc_strdup(track_get_artist(t));
  char* title = mc_strdup(track_get_title(t));
  char* qa = hre_replace_all(re_removeyear, artist, "");
  char* qt = hre_replace_all(re_removeyear, title, "");
  char* qa1 = hre_replace_all(re_compress, qa, "");
  char* qt1 = hre_replace_all(re_compress, qt, "");
  //char* query_artist = hre_replace_all(re_removenum, qa1, "");
  char* query_artist = mc_strdup(qa1);
  char* query_title = hre_replace(re_removenum, qt1, "");
  mc_free(qa);mc_free(qt);mc_free(qa1);mc_free(qt1);
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
      char *p2 = strstr(p1, "<p>");
      if (p2) {
        p2[0] = '\0';
        char* l = hre_replace_all(re_changebr, p1, "___BR___");
        char* l1 = hre_replace_all(re_removetag, l, "");
        char* l2 = hre_replace_all(re_changebr2, l1, "___PAR___");
        char* l3 = hre_replace_all(re_changebr1, l2, " ");
        char* l4 = hre_replace_all(re_changebr3, l3, "__BRBR__");
        data->lyric = hre_replace_all(re_changebr4, l4, "<br /><br />");
        mc_free(l);
        mc_free(l1);
        mc_free(l2);
        mc_free(l3);
        mc_free(l4);
        hre_trim(data->lyric);
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
  hre_destroy(re_removetag);
  hre_destroy(re_removeempty);
  hre_destroy(re_changebr);
  hre_destroy(re_changebr1);
  hre_destroy(re_changebr2);
  hre_destroy(re_changebr3);
  hre_destroy(re_changebr4);
  
  return data->lyric;
}

static void* fetch(void* tt)
{
  struct fetcher* data = (struct fetcher*) tt;
  char* lyric;
  
  lyric = fetch_from_lyricsondemand_dot_com(data); 
  if (lyric == NULL) {
    // try other providers
    
    lyric = mc_strdup("");
  }

  printf("__RESULT__\n%s\n__RESULT_END\n",lyric);
  
  data->lyric = lyric;  
  gdk_threads_enter();
  g_timeout_add(10, callback, data);
  gdk_threads_leave();

  return NULL;
}


void fetch_lyric(track_t* t, void (*f)(const char* lyric, void* data), void* dt)
{
  struct fetcher* data = (struct fetcher*) mc_malloc(sizeof(struct fetcher));
  data->track = track_copy(t);
  data->f = f;
  data->lyric = NULL;
  data->data = dt;
  int thread_id = pthread_create(&data->thread, NULL, fetch, data);
}

