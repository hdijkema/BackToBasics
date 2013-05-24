#include <gui/radio_view.h>
#include <i18n/i18n.h>
#include <curl/curl.h>
#include <metadata/track.h>
#include <util/config.h>
#include <util/webkitcookies.h>


static void radio_determine_stream_url(radio_t* radio, const char* start_url);

radio_view_t* radio_view_new(Backtobasics* btb, radio_library_t* library)
{
  radio_view_t* view = (radio_view_t*) mc_malloc(sizeof(radio_view_t));
  view->btb = btb; 
  view->builder = btb->builder;
  view->library = library;
  view->current_station = NULL;
  
  view->station_model = mc_take_over(station_model_new(library));
 
  return view;
}

static void start_stop_recording(GtkCellRendererToggle* cell,
                                 gchar* path_str,
                                 radio_view_t* view)
{
  GtkTreeIter iter;
  GtkTreeModel* model = station_model_gtk_model(view->station_model);
  gtk_tree_model_get_iter_from_string(model, &iter, path_str);
  int row = gtk_list_model_iter_to_row(GTK_LIST_MODEL(model), iter);
  radio_t* station = radio_library_get(view->library, row);
  gtk_list_model_begin_change(GTK_LIST_MODEL(model));
  if (radio_is_recording(station)) {
    radio_stop_recording(station);
  } else {
    radio_start_recording(station, radio_library_rec_location(view->library));
  }
  gtk_list_model_commit_change(GTK_LIST_MODEL(model));
}

void radio_view_init(radio_view_t* view)
{
  GObject* object = gtk_builder_get_object(view->builder, "view_radio");
  g_object_set_data(object, "radio_view_t", (gpointer) view);
  
  // radio list
  GtkTreeView *tview = GTK_TREE_VIEW(gtk_builder_get_object(view->builder, "tv_radio"));
  view->tview = tview;
  
  GtkTreeViewColumn* col;
  GtkCellRenderer* renderer;
  GtkCellRenderer* toggle;
  
  renderer = gtk_cell_renderer_text_new();
  toggle = gtk_cell_renderer_toggle_new();
  g_object_set(toggle, "editable", TRUE, NULL);
  g_signal_connect(toggle, "toggled", (GCallback) start_stop_recording, (gpointer) view);
  
  view->cols = (GtkTreeViewColumn**) mc_malloc(sizeof(GtkTreeViewColumn*) * STATION_MODEL_N_COLUMNS);
  station_column_enum e;
  for(e = STATION_MODEL_COL_NAME; e < STATION_MODEL_N_COLUMNS; ++e) {
    if (e != STATION_MODEL_COL_RECORDING) {
      col = gtk_tree_view_column_new_with_attributes(station_model_i18n_column_name(e), 
                                                     renderer, "text", e, NULL);
    } else {
      col = gtk_tree_view_column_new_with_attributes(station_model_i18n_column_name(e), 
                                                     toggle, "active", e, NULL);
    }
    gtk_tree_view_column_set_sizing(col, GTK_TREE_VIEW_COLUMN_FIXED);
    char path[500];
    sprintf(path, "radio.column.%s.width", station_model_column_id(e));
    int width = el_config_get_int(btb_config(view->btb), path, 100);
    gtk_tree_view_column_set_fixed_width(col, width);
    gtk_tree_view_column_set_reorderable(col, TRUE);
    gtk_tree_view_column_set_resizable(col, TRUE);
    gtk_tree_view_column_set_clickable(col, TRUE);
    //g_signal_connect(col, "clicked", (GCallback) radio_view_library_col_sort, view);
    view->cols[e] = col;
    g_object_ref(view->cols[e]);
    gtk_tree_view_append_column(tview, col);
  }
  
  gtk_tree_view_set_model(tview, GTK_TREE_MODEL(station_model_gtk_model(view->station_model)));
  
  // Web page
  view->web_view = WEBKIT_WEB_VIEW(webkit_web_view_new());
  file_info_t* btb_cfg_dir = file_info_new_home(".backtobasics");
  file_info_t* btb_cookie_file = file_info_combine(btb_cfg_dir, "btb.cookies");
  add_webkit_cookie_support(view->web_view, file_info_absolute_path(btb_cookie_file));
  file_info_destroy(btb_cookie_file);
  file_info_destroy(btb_cfg_dir);

  //WebKitWebContext* c = webkit_web_context_get_default();
  GtkBox* webp = GTK_BOX(gtk_builder_get_object(view->builder, "scw_radio"));
  //gtk_box_pack_start(webp, GTK_WIDGET(view->web_view), TRUE, TRUE, 4);
  gtk_container_add(GTK_CONTAINER(webp), GTK_WIDGET(view->web_view));
}

void radio_view_destroy(radio_view_t* view)
{
  // first store the column widths
  station_column_enum e;
  for(e = STATION_MODEL_COL_NAME; e < STATION_MODEL_N_COLUMNS; ++e) {
    char path [500];
    sprintf(path, "radio.column.%s.width", station_model_column_id(e));
    int w = gtk_tree_view_column_get_width(view->cols[e]);
    el_config_set_int(btb_config(view->btb), path, w);
    g_object_unref(view->cols[e]);  
  }
  mc_free(view->cols);
  
  if (view->current_station) {
    track_destroy(view->current_station);
  }
  
  station_model_destroy(view->station_model);
  
  destroy_webkit_cookie_support(view->web_view);
  
  mc_free(view);
}

gboolean radio_view_station_clicked(GtkTreeView* tview, GdkEvent* event, GObject* rview)
{
  radio_view_t* view = (radio_view_t*) g_object_get_data(rview, "radio_view_t");
  GtkTreeSelection* sel = gtk_tree_view_get_selection(tview);
  if (gtk_tree_selection_count_selected_rows(sel) == 0) {
    // do nothing
  } else {
    GtkTreeModel* model;
    GtkTreeIter iter;
    if (gtk_tree_selection_get_selected(sel, &model, &iter)) {
      int row = gtk_list_model_iter_to_row(GTK_LIST_MODEL(model), iter);
      radio_t* station = radio_library_get(view->library, row);
      const char* web_url = radio_webpage_url(station);
      if (web_url != NULL) 
        webkit_web_view_load_uri(view->web_view, web_url);
    }
  }
  
  return FALSE;
}

static void play_station(radio_view_t* view, radio_t* station)
{
  //const char* url = radio_stream_url(station);
  track_t* radio_track = track_new();
  track_set_title(radio_track, radio_name(station));
  track_set_stream(radio_track, radio_stream_url(station));
  library_add(radio_library_lib(view->library), radio_track);
  playlist_t* pl = playlist_new(radio_library_lib(view->library),_("Internet Radio Stream"));
  playlist_append(pl, radio_track);
  Backtobasics* btb = view->btb;
  playlist_player_set_playlist(btb->player, pl);
  if (view->current_station) { track_destroy(view->current_station); }
  view->current_station = radio_track;
  log_debug2("track = %s", track_get_id(radio_track));
  playlist_player_play(btb->player);
}
  

void radio_view_play(GtkToolButton* btn, radio_view_t* view)
{
  GtkTreeSelection* sel = gtk_tree_view_get_selection(view->tview);
  if (gtk_tree_selection_count_selected_rows(sel) == 0) {
    // do nothing
  } else {
    GtkTreeModel* model;
    GtkTreeIter iter;
    if (gtk_tree_selection_get_selected(sel, &model, &iter)) {
      int row = gtk_list_model_iter_to_row(GTK_LIST_MODEL(model), iter);
      radio_t* station = radio_library_get(view->library, row);
      play_station(view, station);
    }
  }
}

void radio_view_station_activated(GtkTreeView* tview, GtkTreePath* path, GtkTreeViewColumn* col, GObject* rview)
{
  radio_view_t* view = (radio_view_t*) g_object_get_data(rview, "radio_view_t");
  int row = gtk_tree_path_get_indices(path)[0];
  radio_t* station = radio_library_get(view->library, row);
  play_station(view, station);
}

void radio_view_edit_station(GtkMenuItem* item, GObject* rview)
{
  radio_view_t* view = (radio_view_t*) g_object_get_data(rview, "radio_view_t");
  GtkTreeSelection* sel = gtk_tree_view_get_selection(view->tview);
  GtkTreeModel* model;
  GtkTreeIter iter;
  
  if (gtk_tree_selection_get_selected(sel, &model, &iter)) {

    GtkDialog *dlg = GTK_DIALOG(gtk_builder_get_object(view->builder, "dlg_radio"));
    gtk_window_set_title(GTK_WINDOW(dlg), _("Edit an internet radio station"));
    GtkEntry *url = GTK_ENTRY(gtk_builder_get_object(view->builder, "txt_radio_url"));
    GtkEntry *name = GTK_ENTRY(gtk_builder_get_object(view->builder, "txt_radio_name"));
    GtkEntry *web = GTK_ENTRY(gtk_builder_get_object(view->builder, "txt_radio_web"));
    
    int row = gtk_list_model_iter_to_row(GTK_LIST_MODEL(model), iter);
    radio_t* station = radio_library_get(view->library, row);
    
    gtk_entry_set_text(url, radio_stream_url(station));
    gtk_entry_set_text(name, radio_name(station));
    gtk_entry_set_text(web, radio_webpage_url(station));
   
    el_bool go_on = el_true;
    while (go_on) {
      int response = gtk_dialog_run(dlg);
      if (response) {
        char* r_url = mc_strdup(gtk_entry_get_text(url));
        char* r_name = mc_strdup(gtk_entry_get_text(name));
        char* r_web = mc_strdup(gtk_entry_get_text(web));
        hre_trim(r_url);
        hre_trim(r_name);
        hre_trim(r_web);
        if (strcmp(r_url, "") == 0 || strcmp(r_name, "") == 0) {
          // notify the user that this is not ok
        } else {
          if (strcmp(r_url, radio_name(station)) != 0) {
            radio_set_stream_url(station, r_url);
            radio_set_name(station, r_name);
            radio_set_webpage_url(station, r_web);
            radio_determine_stream_url(station, r_url);
            gtk_list_model_refilter(GTK_LIST_MODEL(station_model_gtk_model(view->station_model)));
            go_on = el_false;
          } else {
            radio_set_name(station, r_name);
            radio_set_webpage_url(station, r_web);
            gtk_list_model_refilter(GTK_LIST_MODEL(station_model_gtk_model(view->station_model)));
            go_on = el_false;
          }
        }
        mc_free(r_url);
        mc_free(r_name);
        mc_free(r_web);
      } else {
        go_on = el_false;
      }
    }
    gtk_widget_hide(GTK_WIDGET(dlg));
  }
}

void radio_view_add_station(GtkMenuItem* item, GObject* rview)
{
  radio_view_t* view = (radio_view_t*) g_object_get_data(rview, "radio_view_t");
  GtkDialog *dlg = GTK_DIALOG(gtk_builder_get_object(view->builder, "dlg_radio"));
  gtk_window_set_title(GTK_WINDOW(dlg), _("Add an internet radio station"));
  GtkEntry *url = GTK_ENTRY(gtk_builder_get_object(view->builder, "txt_radio_url"));
  GtkEntry *name = GTK_ENTRY(gtk_builder_get_object(view->builder, "txt_radio_name"));
  GtkEntry *web = GTK_ENTRY(gtk_builder_get_object(view->builder, "txt_radio_web"));
  
  gtk_entry_set_text(url,"");
  gtk_entry_set_text(name,"");
  gtk_entry_set_text(web,"");
  
  el_bool go_on = el_true;
  while (go_on) {
    int response = gtk_dialog_run(dlg);
    if (response) {
      char* r_url = mc_strdup(gtk_entry_get_text(url));
      char* r_name = mc_strdup(gtk_entry_get_text(name));
      char* r_web = mc_strdup(gtk_entry_get_text(web));
      hre_trim(r_url);
      hre_trim(r_name);
      hre_trim(r_web);
      
      log_debug4("url = %s, name = %s, web = %s", r_url, r_name, r_web);
      if (strcmp(r_url, "") == 0 || strcmp(r_name, "") == 0) {
        // notify the user that this is not correct? 
      } else {
        
        radio_t* station = radio_new(r_url, r_name, r_web);
        radio_determine_stream_url(station, r_url);
        radio_library_append(view->library, station);
        radio_destroy(station);
        gtk_list_model_refilter(GTK_LIST_MODEL(station_model_gtk_model(view->station_model)));
        go_on = el_false;
      }
      log_debug("refilter done");
      mc_free(r_url);
      mc_free(r_name);
      mc_free(r_web);
    } else {
      go_on = el_false;
    }
  }
  gtk_widget_hide(GTK_WIDGET(dlg));
}

void radio_view_remove_station(GtkMenuItem* item, GObject* rview)
{
  radio_view_t* view = (radio_view_t*) g_object_get_data(rview, "radio_view_t");
  log_debug("Remove");
}

static size_t write_data(void *buffer, size_t size, size_t nmemb, void *userp)
{
  memblock_t* block = (memblock_t*) userp;
  size_t s = size * nmemb;
  memblock_write(block, buffer, s);
  if (memblock_size(block) >= 1024) {
    return 0; 
  } else {
    return nmemb;
  }
}

// determines the real stream urls.
// Checks if we got a playlist, or m3u file, by getting the first 
// 1024 bytes from the given url and checking if it is valid 
// stuff. 
static void radio_determine_stream_url(radio_t* radio, const char* start_url) 
{
  CURL *curl;
  
  memblock_t* block = memblock_new();
 
  curl = curl_easy_init();
  curl_easy_setopt(curl, CURLOPT_URL, start_url);
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, block);
  curl_easy_setopt(curl, CURLOPT_RANGE, "0-500");
  curl_easy_perform(curl); /* ignores error */ 
  curl_easy_cleanup(curl);
  
  char c = '\0';
  memblock_write(block, &c, sizeof(char));
  
  log_debug2("buffer: '%s'", memblock_as_str(block));
  char* buffer = mc_strdup(memblock_as_str(block));
  memblock_destroy(block);
  
  hre_trim(buffer);
  if (strncasecmp(buffer, "http://", 7) == 0) {
    radio_set_stream_url(radio, buffer);
  } else if (strncasecmp(buffer, "[playlist]", 10) == 0) {
    log_debug("playlist");
    hre_t re_get_url = hre_compile("file1\\s*=(.*)","im");
    if (hre_has_match(re_get_url, buffer)) {
      log_debug("HAS MATCH");
      hre_matches m = mc_take_over(hre_match(re_get_url, buffer));
      const char* url = hre_match_string(hre_matches_get(m, 1));
      radio_set_stream_url(radio, url);
      hre_matches_destroy(m);
    }
    hre_destroy(re_get_url);
  }
  mc_free(buffer);
    
  log_debug2("real radio stream = %s", radio_stream_url(radio));
}
