#include <gui/radio_view.h>
#include <i18n/i18n.h>
#include <metadata/track.h>


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

void radio_view_init(radio_view_t* view)
{
  GObject* object = gtk_builder_get_object(view->builder, "view_radio");
  g_object_set_data(object, "radio_view_t", (gpointer) view);
  
  // radio list
  GtkTreeView *tview = GTK_TREE_VIEW(gtk_builder_get_object(view->builder, "tv_radio"));
  view->tview = tview;
  
  GtkTreeViewColumn* col;
  GtkCellRenderer* renderer;
  
  renderer = gtk_cell_renderer_text_new();
  view->cols = (GtkTreeViewColumn**) mc_malloc(sizeof(GtkTreeViewColumn*) * STATION_MODEL_N_COLUMNS);
  station_column_enum e;
  for(e = STATION_MODEL_COL_NAME; e < STATION_MODEL_N_COLUMNS; ++e) {
    col = gtk_tree_view_column_new_with_attributes(station_model_i18n_column_name(e), 
                                                   renderer, "text", e, NULL);
    gtk_tree_view_column_set_sizing(col, GTK_TREE_VIEW_COLUMN_FIXED);
    char path[500];
    sprintf(path, "radio.column.%s.width", station_model_column_id(e));
    int width = btb_config_get_int(view->btb, path, 100);
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
  GtkScrolledWindow* webp = GTK_SCROLLED_WINDOW(gtk_builder_get_object(view->builder, "scw_radio"));
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
    btb_config_set_int(view->btb, path, w);
    g_object_unref(view->cols[e]);  
  }
  mc_free(view->cols);
  
  if (view->current_station) {
    track_destroy(view->current_station);
  }
  
  station_model_destroy(view->station_model);
  mc_free(view);
}

void radio_view_station_clicked(GtkTreeView* tview, GdkEvent* event, GObject* rview)
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
      webkit_web_view_load_uri(view->web_view, web_url);
    }
  }
}

void radio_view_station_activated(GtkTreeView* tview, GtkTreePath* path, GtkTreeViewColumn* col, GObject* rview)
{
  radio_view_t* view = (radio_view_t*) g_object_get_data(rview, "radio_view_t");
  int row = gtk_tree_path_get_indices(path)[0];
  radio_t* station = radio_library_get(view->library, row);
  const char* url = radio_stream_url(station);
  track_t* radio_track = track_new();
  track_set_title(radio_track, radio_name(station));
  track_set_stream(radio_track, radio_stream_url(station));
  playlist_t* pl = playlist_new(_("Internet Radio Stream"));
  playlist_append(pl, radio_track);
  Backtobasics* btb = view->btb;
  playlist_player_set_playlist(btb->player, pl);
  if (view->current_station) { track_destroy(view->current_station); }
  view->current_station = radio_track;
  playlist_player_play(btb->player);
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
      char* r_url = (char*) gtk_entry_get_text(url);
      char* r_name = (char*) gtk_entry_get_text(name);
      char* r_web = (char*) gtk_entry_get_text(web);
      hre_trim(r_url);
      hre_trim(r_name);
      hre_trim(r_web);
      if (strcmp(r_url, "") == 0 || strcmp(r_name, "") == 0) {
        // notify the user that this is not correct? 
      } else {
        radio_t* station = radio_new(r_url, r_name, r_web);
        radio_library_append(view->library, station);
        radio_destroy(station);
        gtk_list_model_refilter(GTK_LIST_MODEL(station_model_gtk_model(view->station_model)));
        go_on = el_false;
      }
      g_free(r_url);
      g_free(r_name);
      g_free(r_web);
    } else {
      go_on = el_false;
    }
  }
  gtk_widget_hide(GTK_WIDGET(dlg));
  
}

void radio_view_edit_station(GtkMenuItem* item, GObject* rview)
{
  radio_view_t* view = (radio_view_t*) g_object_get_data(rview, "radio_view_t");
  log_debug("Edit");
}

void radio_view_remove_station(GtkMenuItem* item, GObject* rview)
{
  radio_view_t* view = (radio_view_t*) g_object_get_data(rview, "radio_view_t");
  log_debug("Remove");
}

