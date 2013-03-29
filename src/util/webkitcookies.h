#ifndef __WEBKIT__COOKIES__H
#define __WEBKIT__COOKIES__H

#include <gtk/gtk.h>
#include <webkit/webkit.h>

void add_webkit_cookie_support(WebKitWebView* web_view, const char* cookie_store_filename);
void destroy_webkit_cookie_support(WebKitWebView* web_view);

#endif
