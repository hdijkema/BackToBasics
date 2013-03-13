#ifndef RADIO_VIEW_HOD
#define RADIO_VIEW_HOD

#include <gtk/gtk.h>
#include <webkit/webkit.h>

typedef struct __radio_view_t__  radio_view_t; 

#include <backtobasics.h>
#include <elementals.h>
#include <gui/station_model.h>

typedef struct __radio_view_t__ {
  Backtobasics* btb;
  GtkBuilder* builder;
  
  radio_library_t* library;
  
  WebKitWebView* web_view;
  GtkTreeView* tview;
  GtkTreeViewColumn** cols;
  
  station_model_t* station_model;
  
  track_t* current_station;
  
} radio_view_t;
  
  
radio_view_t* radio_view_new(Backtobasics* btb, radio_library_t* library);
void radio_view_destroy(radio_view_t* view);
void radio_view_init(radio_view_t* view);

#endif
