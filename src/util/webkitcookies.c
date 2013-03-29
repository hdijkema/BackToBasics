#include <util/webkitcookies.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/file.h>
#include <elementals.h>

#define sessiontime (3600*14)

static void newrequest(SoupSession *s, SoupMessage *msg, gpointer v); 
static const char *getcookies(SoupURI *uri, const char* cookiefile); 
static void setcookie(SoupCookie *c, const char* cookiefile); 
static void gotheaders(SoupMessage *msg, gpointer v);

static el_bool initialized = el_false;
static char* cookiefile = NULL;

void add_webkit_cookie_support(WebKitWebView* web_view, const char* cookie_store_filename)
{
  if (initialized) {
    log_warning("add_webkit_cookie_support is global to all webkit sessions");
    log_warning("it's of no use adding it twice. However, this may change");
    log_warning("in the future.");
    log_warning2("please note that this cookie store '%s' is ignored", cookie_store_filename); 
  } else {
    initialized = el_true;
    
    SoupSession* session;
    
    cookiefile = mc_strdup(cookie_store_filename);
    
    session = webkit_get_default_session();
    soup_session_remove_feature_by_type(session, soup_cookie_get_type());
    soup_session_remove_feature_by_type(session, soup_cookie_jar_get_type());
    g_signal_connect_after(G_OBJECT(session), 
                           "request-started", 
                           G_CALLBACK(newrequest), 
                           NULL
                           );
  }
}

void destroy_webkit_cookie_support(WebKitWebView* web_view)
{
  if (cookiefile != NULL) {
    mc_free(cookiefile);
    cookiefile = NULL;
  }
}

void newrequest(SoupSession *s, SoupMessage *msg, gpointer v) 
{
	SoupMessageHeaders *h = msg->request_headers;
	SoupURI *uri;
	const char* c;
	
	log_debug2("cookiefile = %s", cookiefile);
	
	soup_message_headers_remove(h, "Cookie");
	uri = soup_message_get_uri(msg);
	if ((c = getcookies(uri, cookiefile))) {
		soup_message_headers_append(h, "Cookie", c);
	}
	
	g_signal_connect_after(G_OBJECT(msg), 
	                       "got-headers", 
	                       G_CALLBACK(gotheaders), 
	                       (gpointer) cookiefile
	                       );
}

const char *getcookies(SoupURI *uri, const char* cookiefile) 
{
	const char *c;
	SoupCookieJar *j = soup_cookie_jar_text_new(cookiefile, TRUE);
	c = soup_cookie_jar_get_cookies(j, uri, TRUE);
	g_object_unref(j);
	return c;
}

void gotheaders(SoupMessage *msg, gpointer v) 
{
	//SoupURI *uri;
	GSList *l, *p;
	
	log_debug2("cookiefile = %s", cookiefile);

	//uri = soup_message_get_uri(msg);
	for(p = l = soup_cookies_from_response(msg); p;
		p = g_slist_next(p))  {
		setcookie((SoupCookie *)p->data, cookiefile);
	}
	soup_cookies_free(l);
}

	
	
void setcookie(SoupCookie *c, const char* cookiefile) 
{
	int lock;

	lock = open(cookiefile, 0);
	flock(lock, LOCK_EX);
	SoupDate *e;
	SoupCookieJar *j = soup_cookie_jar_text_new(cookiefile, FALSE);
	c = soup_cookie_copy(c);
	if(c->expires == NULL && sessiontime) {
		e = soup_date_new_from_time_t(time(NULL) + sessiontime);
		soup_cookie_set_expires(c, e);
	}
	soup_cookie_jar_add_cookie(j, c);
	g_object_unref(j);
	flock(lock, LOCK_UN);
	close(lock);
}


