#ifndef __SIMPLE__HTML_H
#define __SIMPLE__HTML_H

#include <metadata/track.h>
#include <gtk/gtk.h>

char* text_to_html(const char* in);
char* html_to_text(const char* in);
char* stripped_title(track_t* t);
void open_url(GtkWidget* w, const char* url);
char* to_http_get_query(const char* in);

#endif
