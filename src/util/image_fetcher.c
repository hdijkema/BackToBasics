#include <elementals.h>
#include <gtk/gtk.h>
#include <curl/curl.h>
#include <stdio.h>
#include <time.h>
#include <util/image_fetcher.h>


struct fetcher {
  pthread_t thread;
  char* url;
  char* art_file;
  ImageFetchedCallback cb;
  void* data;
  time_t timestamp;
  el_bool fetched;
};

static gboolean callback(gpointer p)
{
  struct fetcher* fetch = (struct fetcher*) p;
  fetch->cb(fetch->data, fetch->fetched);
  mc_free(fetch->url);
  mc_free(fetch->art_file);
  return FALSE;
}

static int progress(void* userp, double dltotal, double dlnow, double ultotal, double ulnow)
{
  struct fetcher* fetch = (struct fetcher*) userp;
  if (dlnow > 0.0) {
    fetch->timestamp = time(NULL);
  } else {
    time_t t = time(NULL);
    if ((t-30) > fetch->timestamp) {
      log_error2("Timeout: Cancelling fetch of %s", fetch->url);
      fetch->fetched = el_false;
      return 1;
    }
  }
  return 0;
}

static size_t write_data(void* buffer, size_t size, size_t nmemb, void* userp)
{
  FILE* f = userp;
  return fwrite(buffer, size, nmemb, f);
}

static void* fetchimg(void* _fetch)
{
  struct fetcher* fetch = (struct fetcher*) _fetch;
  FILE* f = fopen(fetch->art_file, "wb");
  if (f != NULL) {
    CURL* curl = curl_easy_init();
    curl_easy_setopt(curl, CURLOPT_URL, fetch->url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, f);
    curl_easy_setopt(curl, CURLOPT_PROGRESSFUNCTION, progress);
    curl_easy_perform(curl);
    curl_easy_cleanup(curl);
    fclose(f);
  } else {
    fetch->fetched = el_false;
  }
  
  gdk_threads_enter();
  g_timeout_add(10, callback, fetch);
  gdk_threads_leave();  
  
  return NULL;
}


void fetch_image(const char* url, const char* art_file, ImageFetchedCallback cb, void* data)
{
  struct fetcher* fetch = (struct fetcher*) mc_malloc(sizeof(struct fetcher));
  fetch->url = mc_strdup(url);
  fetch->art_file = mc_strdup(art_file);
  fetch->cb = cb;
  fetch->data = data;
  fetch->fetched = el_true;
  fetch->timestamp = time(NULL);
  int thread_id = pthread_create(&fetch->thread, NULL, fetchimg, fetch); 
}
