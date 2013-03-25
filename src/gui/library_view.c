#include <backtobasics.h>
#include <gui/library_view.h>
#include <gui/string_model.h>
#include <gui/async_call.h>
#include <i18n/i18n.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <lyrics/lyrics.h>
#include <util/config.h>
#include <util/simple_html.h>
#include <util/image_fetcher.h>

static gboolean library_view_update_info(library_view_t* view);
static void get_column_layout(library_view_t* view, long long hash);

library_view_t* library_view_new(Backtobasics* btb, library_t* library)
{
  library_view_t* view = (library_view_t*) mc_malloc(sizeof(library_view_t));
  view->btb = btb;
  view->builder = btb->builder;
  view->library = library;
  
  view->playlist_model = mc_take_over(playlist_model_new());
  //view->library_list_changed = el_false;
  //view->library_list_hash = playlist_model_tracks_hash(view->playlist_model);
  
  view->genre_model = string_model_new();
  view->artist_model = string_model_new();
  view->album_model = string_model_new();
  
  view->aspect = GENRE_ASPECT;
  view->previous_aspect = NONE_ASPECT;
  
  playlist_model_set_playlist(view->playlist_model, library_current_selection(view->library, _("Library")));
  
  string_model_set_array(view->genre_model, library_genres(view->library), el_false);
  string_model_set_array(view->artist_model, library_artists(view->library), el_false);
  string_model_set_array(view->album_model, library_albums(view->library), el_false);
  
  view->run_timeout = el_true;
  view->time_in_ms = -1;
  view->len_in_ms = -1;
  view->track_index = -1;
  view->track_id = NULL;
  view->sliding = 0;
  view->repeat = -1;
  
  view->img_w = 200;
  view->img_h = 200;
  
  view->btn_play = NULL;
  view->btn_pause = NULL;
  view->cols = NULL;
  
  view->ignore_sel_changed = el_false;
  
  view->playlists_model = playlists_model_new(view->library);
  
  view->current_lyric_track = NULL;
  
  view->column_layout_changing = el_false;
  
  return view;
}

void library_view_destroy(library_view_t* view)
{
  // first store the column widths
  playlist_column_enum e;
  for(e = PLAYLIST_MODEL_COL_NR; e < PLAYLIST_MODEL_N_COLUMNS; ++e) {
    char path [500];
    sprintf(path, "library.column.%s.width", column_id(e));
    int w = gtk_tree_view_column_get_width(view->cols[e]);
    log_debug3("%s = %d", path, w);
    el_config_set_int(btb_config(view->btb), path, w);
    g_object_unref(view->cols[e]);
  }
  mc_free(view->cols);
  
  if (view->run_timeout) {
    log_error("timeout has not been stopped before destroying!");
    library_view_stop_info_updater(view);
  }
  
  playlist_model_destroy(view->playlist_model);
  string_model_destroy(view->genre_model);
  string_model_destroy(view->artist_model);
  string_model_destroy(view->album_model);
  playlists_model_destroy(view->playlists_model);
  mc_free(view);
}

void library_view_stop_info_updater(library_view_t* view)
{
  view->run_timeout = el_false;
}

void library_view_clear_playlist(library_view_t* view) 
{
  playlist_t* pl = playlist_new(view->library, "nil");
  playlist_model_set_playlist(view->playlist_model, pl);
}

static void library_view_aspect_page(library_view_t* view, aspect_enum aspect);
static void library_view_adjust_aspect_buttons(library_view_t* view, aspect_enum aspect);
static void library_view_library_col_sort(GtkTreeViewColumn* col, library_view_t* view);

void library_view_init(library_view_t* view)
{
  // library view.
  GObject* object = gtk_builder_get_object(view->builder,"view_library");
  g_object_set_data(object, "library_view_t", (gpointer) view);
  
  // playlists (initially not viewed)
  //GtkWidget* scw_playlists = GTK_WIDGET(gtk_builder_get_object(view->builder,"scw_playlists"));
  //gtk_widget_hide(scw_playlists);

  // library list
  GtkTreeView* tview = GTK_TREE_VIEW(gtk_builder_get_object(view->builder, "tv_library"));
  view->tview = tview;
  GtkTreeViewColumn *col;
  GtkCellRenderer* renderer;
  
  renderer = gtk_cell_renderer_text_new();
  
  view->cols = (GtkTreeViewColumn**) mc_malloc(sizeof(GtkTreeViewColumn*) * PLAYLIST_MODEL_N_COLUMNS);
  playlist_column_enum e;
  for(e = PLAYLIST_MODEL_COL_NR; e < PLAYLIST_MODEL_N_COLUMNS; ++e) {
    col = gtk_tree_view_column_new_with_attributes(i18n_column_name(e), renderer, "text", e, NULL);
    gtk_tree_view_column_set_sizing(col, GTK_TREE_VIEW_COLUMN_FIXED);
    char path [500];
    sprintf(path, "library.column.%s.width", column_id(e));
    int width = el_config_get_int(btb_config(view->btb), path, 100);
    if (width < 10) { width = 100; }
    gtk_tree_view_column_set_fixed_width(col, width);
    gtk_tree_view_column_set_reorderable(col, TRUE);
    gtk_tree_view_column_set_resizable(col, TRUE);
    gtk_tree_view_column_set_clickable(col, TRUE);
    g_signal_connect(col, "clicked", (GCallback) library_view_library_col_sort, view);
    view->cols[e] = col;
    g_object_ref(view->cols[e]);
    gtk_tree_view_append_column(tview, col);
  }
  
  gtk_tree_view_set_model(tview, GTK_TREE_MODEL(playlist_model_gtk_model(view->playlist_model)));
  
  // Aspect lists
  int width;
  
  // Genres
  tview = GTK_TREE_VIEW(gtk_builder_get_object(view->builder, "tv_genre_aspect"));
  col = gtk_tree_view_column_new_with_attributes(_("Genre"), renderer, "text", 0, NULL);
  gtk_tree_view_column_set_sizing(col, GTK_TREE_VIEW_COLUMN_FIXED);
  //width = el_config_get_int(btb_config(view->btb), "library.aspects.column_width", 200);
  //gtk_tree_view_column_set_fixed_width(col, width);
  gtk_tree_view_append_column(tview, col);
  gtk_tree_view_set_model(tview, string_model_gtk_model(view->genre_model));
  
  // Artists
  tview = GTK_TREE_VIEW(gtk_builder_get_object(view->builder, "tv_artist_aspect"));
  col = gtk_tree_view_column_new_with_attributes(_("Artists"), renderer, "text", 0, NULL);
  gtk_tree_view_column_set_sizing(col, GTK_TREE_VIEW_COLUMN_FIXED);
  //width = el_config_get_int(btb_config(view->btb), "library.aspects.column_width", 200);
  //gtk_tree_view_column_set_fixed_width(col, width);
  gtk_tree_view_append_column(tview, col);
  gtk_tree_view_set_model(tview, string_model_gtk_model(view->artist_model));
  
  // Albums
  tview = GTK_TREE_VIEW(gtk_builder_get_object(view->builder, "tv_album_aspect"));
  col = gtk_tree_view_column_new_with_attributes(_("Artists"), renderer, "text", 0, NULL);
  gtk_tree_view_column_set_sizing(col, GTK_TREE_VIEW_COLUMN_FIXED);
  //width = el_config_get_int(btb_config(view->btb), "library.aspects.column_width", 200);
  //gtk_tree_view_column_set_fixed_width(col, width);
  gtk_tree_view_append_column(tview, col);
  gtk_tree_view_set_model(tview, string_model_gtk_model(view->album_model));
  
  // Activate genres
  library_view_aspect_page(view, GENRE_ASPECT);
  GtkToggleToolButton* g_btn = GTK_TOGGLE_TOOL_BUTTON(gtk_builder_get_object(view->builder, "tbtn_genres"));
  gtk_toggle_tool_button_set_active(g_btn, TRUE);
  
  // playback scale, song info
  GtkScale* sc_playback = GTK_SCALE(gtk_builder_get_object(view->builder, "sc_library_playback"));
  gtk_range_set_range(GTK_RANGE(sc_playback), 0.0, 100.0);
  { 
    char ss[300];
    sprintf(ss,"<span size=\"x-small\"><i><b> </b></i></span>");
    GtkLabel* lbl = GTK_LABEL(gtk_builder_get_object(view->builder, "lbl_song_info"));
    gtk_label_set_markup(lbl, ss);
  }
  
  // Set logo
  {
    char *path = backtobasics_logo(view->btb); 
    file_info_t* info = file_info_new(path);
    mc_free(path);
    if (file_info_is_file(info)) {
      GError *err = NULL;
      GdkPixbuf* pb = gdk_pixbuf_new_from_file_at_scale(file_info_path(info),
                                                        view->img_w, view->img_h,
                                                        TRUE,
                                                        &err
                                                        );
      if (pb != NULL) {
        GtkImage* img = GTK_IMAGE(gtk_builder_get_object(view->builder, "img_art"));
        gtk_image_set_from_pixbuf(img, pb);
        g_object_unref(pb);
      } else {
        log_error3("error loading image art: %d, %s", err->code, err->message);
        //g_free(err);
      }
    }
    file_info_destroy(info);
  }
  
  // Playlists
  tview = GTK_TREE_VIEW(gtk_builder_get_object(view->builder, "tv_playlists"));
  col = gtk_tree_view_column_new_with_attributes(_("Playlist"), renderer, "text", 0, NULL);
  gtk_tree_view_column_set_sizing(col, GTK_TREE_VIEW_COLUMN_FIXED);
  width = el_config_get_int(btb_config(view->btb), "library.playlists.column_width", 200);
  gtk_tree_view_column_set_fixed_width(col, width);
  gtk_tree_view_append_column(tview, col);
  
  gtk_tree_view_set_model(tview, playlists_model_gtk_model(view->playlists_model));
  
  // Lyric view
  view->lyric_view = WEBKIT_WEB_VIEW(webkit_web_view_new());
  view->lbl_lyric_track = GTK_LABEL(gtk_builder_get_object(view->builder, "lbl_lyric_track"));
  GtkScrolledWindow* scw_lyric = GTK_SCROLLED_WINDOW(gtk_builder_get_object(view->builder, "scw_lyric"));
  gtk_container_add(GTK_CONTAINER(scw_lyric), GTK_WIDGET(view->lyric_view));
  
  // visibility of columns
  {
    const char* names[] = {
      "chk_col_nr", "chk_col_title", "chk_col_artist", "chk_col_composer",
      "chk_col_piece", "chk_col_album", "chk_col_albumartist", "chk_col_genre",
      "chk_col_year", "chk_col_length", NULL
    };
    
    const playlist_column_enum es[] = {
      PLAYLIST_MODEL_COL_NR, PLAYLIST_MODEL_COL_TITLE, PLAYLIST_MODEL_COL_ARTIST,
      PLAYLIST_MODEL_COL_COMPOSER, PLAYLIST_MODEL_COL_PIECE, 
      PLAYLIST_MODEL_COL_ALBUM_TITLE, PLAYLIST_MODEL_COL_ALBUM_ARTIST,
      PLAYLIST_MODEL_COL_GENRE, PLAYLIST_MODEL_COL_YEAR, PLAYLIST_MODEL_COL_LENGTH,
      PLAYLIST_MODEL_N_COLUMNS
    };
    
    int i;
    for(i = 0;names[i] != NULL; ++i) {
      GtkCheckMenuItem* item = GTK_CHECK_MENU_ITEM(gtk_builder_get_object(view->builder, names[i]));
      gtk_widget_set_name(GTK_WIDGET(item), names[i]);
      char cfgitem[100];
      sprintf(cfgitem, "library.cols.%s", names[i]);
      int yes = el_config_get_int(btb_config(view->btb), cfgitem, 1);
      gtk_check_menu_item_set_active(item, yes);
      gtk_tree_view_column_set_visible(view->cols[es[i]], yes);
    }
  }
  
  // Start timeout every 250 ms
  g_timeout_add(250, (GSourceFunc) library_view_update_info, view); 
}

void library_view_reset_models(library_view_t* view)
{
  library_filter_none(view->library);
  
  playlist_model_set_playlist(view->playlist_model, library_current_selection(view->library, _("Library")));

  string_model_set_array(view->genre_model, library_genres(view->library), el_false);
  string_model_set_array(view->artist_model, library_artists(view->library), el_false);
  string_model_set_array(view->album_model, library_albums(view->library), el_false);
  
  GtkTreeView* tview = GTK_TREE_VIEW(gtk_builder_get_object(view->builder, "tv_library"));
  gtk_tree_view_set_model(tview, GTK_TREE_MODEL(playlist_model_gtk_model(view->playlist_model)));
  
  tview = GTK_TREE_VIEW(gtk_builder_get_object(view->builder, "tv_genre_aspect"));
  gtk_tree_view_set_model(tview, string_model_gtk_model(view->genre_model));
  
  tview = GTK_TREE_VIEW(gtk_builder_get_object(view->builder, "tv_artist_aspect"));
  gtk_tree_view_set_model(tview, string_model_gtk_model(view->artist_model));
  
  tview = GTK_TREE_VIEW(gtk_builder_get_object(view->builder, "tv_album_aspect"));
  gtk_tree_view_set_model(tview, string_model_gtk_model(view->album_model));
  
}


#define SET_TOGGLE(btn, VAL) \
  if (!VAL && gtk_toggle_tool_button_get_active(btn)) { \
    g_object_set_data(G_OBJECT(btn), "ignore", btn); \
  } else if (VAL && !gtk_toggle_tool_button_get_active(btn)) { \
    g_object_set_data(G_OBJECT(btn), "ignore", btn); \
  } \
  gtk_toggle_tool_button_set_active(btn, VAL)

static void library_view_adjust_aspect_buttons(library_view_t* view, aspect_enum aspect)
{
  GtkToggleToolButton* g_btn = GTK_TOGGLE_TOOL_BUTTON(gtk_builder_get_object(view->builder, "tbtn_genres"));
  GtkToggleToolButton* ar_btn = GTK_TOGGLE_TOOL_BUTTON(gtk_builder_get_object(view->builder, "tbtn_artists"));
  GtkToggleToolButton* al_btn = GTK_TOGGLE_TOOL_BUTTON(gtk_builder_get_object(view->builder, "tbtn_albums"));

  switch (aspect) {
    case GENRE_ASPECT: {
        SET_TOGGLE(ar_btn, FALSE);
        SET_TOGGLE(al_btn, FALSE);
        SET_TOGGLE(g_btn, TRUE);
    }
    break;
    case ARTIST_ASPECT: {
        SET_TOGGLE(ar_btn, TRUE);
        SET_TOGGLE(al_btn, FALSE);
        SET_TOGGLE(g_btn, FALSE);
    }
    break;
    case ALBUM_ASPECT: {
        SET_TOGGLE(ar_btn, FALSE);
        SET_TOGGLE(al_btn, TRUE);
        SET_TOGGLE(g_btn, FALSE);
    }
    break;
    default:
    break;
  }
}

static void library_view_aspect_page(library_view_t* view, aspect_enum aspect)
{
  GtkNotebook* nb = GTK_NOTEBOOK(gtk_builder_get_object(view->builder, "nb_aspects"));
  gtk_notebook_set_current_page(nb, aspect);
  library_view_adjust_aspect_buttons(view, aspect);
  if (aspect != view->aspect) {
    view->previous_aspect = view->aspect;
    view->aspect = aspect;
  }
}

static void library_set_aspect_filter(library_view_t* view, aspect_enum aspect)
{
  log_debug("aspect");
  
  set_t* filtered_artists = library_filtered_artists(view->library);
  set_t* filtered_albums = library_filtered_albums(view->library);
  
  log_debug3("albums %d, artists %d", set_count(filtered_albums), set_count(filtered_artists));
  
  if (aspect == GENRE_ASPECT) {
    string_model_set_valid(view->artist_model, filtered_artists);
    string_model_set_valid(view->album_model, filtered_albums);
    if (set_count(filtered_artists) <= set_count(filtered_albums)) {
      library_view_aspect_page(view, ARTIST_ASPECT);
    } else {
      library_view_aspect_page(view, ALBUM_ASPECT);
    }
  } else if (aspect == ALBUM_ASPECT) {
    string_model_set_valid(view->artist_model, filtered_artists);
  } else if (aspect == ARTIST_ASPECT) {
    string_model_set_valid(view->album_model, filtered_albums);
    library_view_aspect_page(view, ALBUM_ASPECT);
  }
  
  playlist_model_set_valid(view->playlist_model, library_filtered_tracks(view->library));
  get_column_layout(view, playlist_model_tracks_hash(view->playlist_model));
}

gboolean library_view_genre_clicked(GtkTreeView* tview, GdkEvent* event, GObject* lview)
{
  log_debug("Genre clicked");
  
  GdkEventButton* ebtn = (GdkEventButton*) event; 
  if (ebtn->button != 1) {
    return FALSE;
  }
  
  library_view_t* view = (library_view_t*) g_object_get_data(lview, "library_view_t");
  
  GtkTreeSelection* sel = gtk_tree_view_get_selection(tview);
  if (gtk_tree_selection_count_selected_rows(sel) == 0) {
    library_filter_none(view->library);
  } else {
    GtkTreeModel* model;
    GtkTreeIter iter;
    if (gtk_tree_selection_get_selected(sel, &model, &iter)) {
      int row = gtk_list_model_iter_to_row(GTK_LIST_MODEL(model), iter);
      string_model_array arr = string_model_get_array(view->genre_model);
      library_filter_none(view->library);
      library_filter_genre(view->library, string_model_array_get(arr, row));
    }
  }

  tview = GTK_TREE_VIEW(gtk_builder_get_object(view->builder, "tv_artist_aspect"));
  sel = gtk_tree_view_get_selection(tview);
  if (gtk_tree_selection_count_selected_rows(sel) > 0) {
    gtk_tree_selection_unselect_all(sel);
  }  
  
  tview = GTK_TREE_VIEW(gtk_builder_get_object(view->builder, "tv_album_aspect"));
  sel = gtk_tree_view_get_selection(tview);
  if (gtk_tree_selection_count_selected_rows(sel) > 0) {
    gtk_tree_selection_unselect_all(sel);
  }  

  library_set_aspect_filter(view, GENRE_ASPECT);
  
  return FALSE;
}

gboolean library_view_artist_clicked(GtkTreeView* tview, GdkEvent* event, GObject* lview)
{
  log_debug("Album clicked");

  GdkEventButton* ebtn = (GdkEventButton*) event; 
  if (ebtn->button != 1) {
    return FALSE;
  }
  
  library_view_t* view = (library_view_t*) g_object_get_data(lview, "library_view_t");
  
  GtkTreeSelection* sel = gtk_tree_view_get_selection(tview);
  if (gtk_tree_selection_count_selected_rows(sel) == 0) {
    library_filter_album_artist(view->library, NULL);
  } else {
    GtkTreeModel* model;
    GtkTreeIter iter;
    if (gtk_tree_selection_get_selected(sel, &model, &iter)) {
      int row = gtk_list_model_iter_to_row(GTK_LIST_MODEL(model), iter);
      string_model_array arr = string_model_get_array(view->artist_model);
      const char* artist = string_model_array_get(arr, row);
      log_debug2("artist = %s", artist);
      library_filter_album_artist(view->library, string_model_array_get(arr, row));
    }
  }

  tview = GTK_TREE_VIEW(gtk_builder_get_object(view->builder, "tv_genre_aspect"));
  sel = gtk_tree_view_get_selection(tview);
  if (gtk_tree_selection_count_selected_rows(sel) > 0) {
    gtk_tree_selection_unselect_all(sel);
  }  
  
  tview = GTK_TREE_VIEW(gtk_builder_get_object(view->builder, "tv_album_aspect"));
  sel = gtk_tree_view_get_selection(tview);
  if (gtk_tree_selection_count_selected_rows(sel) > 0) {
    gtk_tree_selection_unselect_all(sel);
  }  

  library_set_aspect_filter(view, ARTIST_ASPECT);
  
  return FALSE;
}

gboolean library_view_album_clicked(GtkTreeView* tview, GdkEvent* event, GObject* lview)
{
  log_debug("Artist clicked");

  GdkEventButton* ebtn = (GdkEventButton*) event; 
  if (ebtn->button != 1) {
    return FALSE;
  }
  
  library_view_t* view = (library_view_t*) g_object_get_data(lview, "library_view_t");
  
  GtkTreeSelection* sel = gtk_tree_view_get_selection(tview);
  if (gtk_tree_selection_count_selected_rows(sel) == 0) {
    library_filter_none(view->library);
  } else {
    GtkTreeModel* model;
    GtkTreeIter iter;
    if (gtk_tree_selection_get_selected(sel, &model, &iter)) {
      int row = gtk_list_model_iter_to_row(GTK_LIST_MODEL(model), iter);
      string_model_array arr = string_model_get_array(view->album_model);
      library_filter_album_title(view->library, string_model_array_get(arr, row));
    }
  }

  tview = GTK_TREE_VIEW(gtk_builder_get_object(view->builder, "tv_genre_aspect"));
  sel = gtk_tree_view_get_selection(tview);
  if (gtk_tree_selection_count_selected_rows(sel) > 0) {
    gtk_tree_selection_unselect_all(sel);
  }  
  
  tview = GTK_TREE_VIEW(gtk_builder_get_object(view->builder, "tv_artist_aspect"));
  sel = gtk_tree_view_get_selection(tview);
  if (gtk_tree_selection_count_selected_rows(sel) > 0) {
    gtk_tree_selection_unselect_all(sel);
  }  

  library_set_aspect_filter(view, ALBUM_ASPECT);
  
  return FALSE;
}


void library_view_artists(GtkToggleToolButton *btn, GObject* lview)
{
  if (g_object_steal_data(G_OBJECT(btn), "ignore"))
    return;
  
  library_view_t* view = (library_view_t*) g_object_get_data(lview, "library_view_t");
  library_view_aspect_page(view, ARTIST_ASPECT);
}

void library_view_albums(GtkToggleToolButton *btn, GObject* lview)
{
  if (g_object_steal_data(G_OBJECT(btn), "ignore"))
    return;

  library_view_t* view = (library_view_t*) g_object_get_data(lview, "library_view_t");
  library_view_aspect_page(view, ALBUM_ASPECT);
}

void library_view_genres(GtkToggleToolButton *btn, GObject* lview)
{
  if (g_object_steal_data(G_OBJECT(btn), "ignore"))
    return;

  library_view_t* view = (library_view_t*) g_object_get_data(lview, "library_view_t");
  library_view_aspect_page(view, GENRE_ASPECT);
}

static void library_view_play_at(library_view_t* view, int track)
{
  playlist_player_t* player = backtobasics_player(view->btb);

  log_debug3("model: %lld, player: %lld",
              playlist_model_tracks_hash(view->playlist_model),
              playlist_player_get_hash(player)
              );
              
  if (playlist_model_tracks_hash(view->playlist_model) != playlist_player_get_hash(player) ) {
    playlist_t* pl = playlist_model_get_selected_playlist(view->playlist_model);
    playlist_log(pl);
    playlist_player_set_playlist(player, pl);
    playlist_player_set_repeat(player, PLP_NO_REPEAT);
    view->time_in_ms = -1;
    playlist_player_set_track(player, track); // implies play
  } else {
    if (!playlist_player_is_paused(player)) {
      playlist_player_set_track(player, track); // implies play
    }
  }
}

void library_view_play(GtkToolButton *btn, library_view_t* view)
{
  //library_view_t* view = (library_view_t*) g_object_get_data(lview, "library_view_t");
  playlist_player_t* player = backtobasics_player(view->btb);
  if (!playlist_player_is_playing(player)) {
    GtkTreeView* tview = view->tview;
    GtkTreePath* path = NULL;
    GtkTreeViewColumn* col = NULL;
    gtk_tree_view_get_cursor(tview, &path, &col);
    int index = 0;
    if (path != NULL) {
      int *i = gtk_tree_path_get_indices(path);
      index = i[0];
    }
    library_view_play_at(view, index);
  }
}

void library_view_pause(GtkToolButton *btn, GObject* lview)
{
  library_view_t* view = (library_view_t*) g_object_get_data(lview, "library_view_t");
  playlist_player_t* player = backtobasics_player(view->btb);
  playlist_player_pause(player);
}

void library_view_play_previous(GtkToolButton *btn, GObject* lview)
{
  library_view_t* view = (library_view_t*) g_object_get_data(lview, "library_view_t");
  playlist_player_t* player = backtobasics_player(view->btb);
  playlist_player_previous(player);
}

void library_view_play_next(GtkToolButton *btn, GObject* lview)
{
  library_view_t* view = (library_view_t*) g_object_get_data(lview, "library_view_t");
  playlist_player_t* player = backtobasics_player(view->btb);
  playlist_player_next(player);
}

void library_view_repeat(GtkToolButton* btn, GObject* lview)
{
  library_view_t* view = (library_view_t*) g_object_get_data(lview, "library_view_t");
  playlist_player_t* player = backtobasics_player(view->btb);
  playlist_player_repeat_t r = playlist_player_get_repeat(player);
  switch (r) {
    case PLP_NO_REPEAT: {
      playlist_player_set_repeat(player, PLP_TRACK_REPEAT);
    }
    break;
    case PLP_TRACK_REPEAT: {
      playlist_player_set_repeat(player, PLP_LIST_REPEAT);  
    }
    break;
    case PLP_LIST_REPEAT: {
      playlist_player_set_repeat(player, PLP_NO_REPEAT);  
    }
    break;
  }
}

void library_view_track_activated(GtkTreeView *tview, GtkTreePath *path, GtkTreeViewColumn *column, GObject* lview)
{
  log_debug("entering track activated");
  library_view_t* view = (library_view_t*) g_object_get_data(lview, "library_view_t");
  int* index = gtk_tree_path_get_indices(path);
  library_view_play_at(view, index[0]);
}

void library_view_play_album(GtkTreeView *album_view, GtkTreePath *path, GtkTreeViewColumn* column, GObject* lview)
{
  log_debug("play album");
  library_view_t* view = (library_view_t*) g_object_get_data(lview, "library_view_t");
  library_view_play_at(view, 0);  
}


void library_view_playback_changed(GtkRange* range, GtkScrollType scroll, gdouble value, GObject* lview)
{
  library_view_t* view = (library_view_t*) g_object_get_data(lview, "library_view_t");
  if (view) {
    if (view->sliding == 2) {
      view->sliding = 0;
      playlist_player_t* player = backtobasics_player(view->btb);
      if (playlist_player_is_playing(player)) {
        track_t* t = playlist_player_get_track(player);
        if (t != NULL) {
          double d = value / 100.0;
          long len_in_ms = track_get_length_in_ms(t);
          long seek_in_ms = len_in_ms * d;
          playlist_player_seek(player, seek_in_ms);
        }
      }
      view->sliding = el_false;
    } else if (view->sliding == 1) {
      
    }
  } 
} 

gboolean library_view_slider_set(GtkWidget* widget, GdkEvent* event, GObject* lview)
{
  library_view_t* view = (library_view_t*) g_object_get_data(lview, "library_view_t");
  view->sliding = 1;
  return FALSE;
}

gboolean library_view_slider_unset(GtkWidget* widget, GdkEvent* event, GObject* lview)
{
  library_view_t* view = (library_view_t*) g_object_get_data(lview, "library_view_t");
  view->sliding = 2;
  library_view_playback_changed(GTK_RANGE(widget), GTK_SCROLL_JUMP, 
                                gtk_range_get_value(GTK_RANGE(widget)), 
                                lview);
  return FALSE;
}

struct lyric_cb {
  track_t* track;
  library_view_t* view;
};

static void library_view_process_lyric(char* lyric, void* data)
{
  
  struct lyric_cb* cb = (struct lyric_cb*) data;
  track_t* t = cb->track;
  library_view_t* view = cb->view;
  mc_free(cb);

  view->current_lyric_track = t;
  if (strcmp(lyric, "") != 0) {
    track_set_lyric(t, lyric);
  }
  
  char *artist = text_to_html(track_get_artist(t));
  char *title = text_to_html(track_get_title(t));

  char* s = (char*) mc_malloc(strlen(artist)+sizeof(", ")+strlen(title)+200);
  sprintf(s,"<span size=\"x-small\"><i><b>%s\n%s</b></i></span>", artist, title);
  gtk_label_set_markup(view->lbl_lyric_track,s);
  mc_free(s);
  
  mc_free(title);
  mc_free(artist);
  
  char* html = lyric_text_to_html(lyric);
  write_lyric(t, lyric, el_false); // write but don't overwrite
  webkit_web_view_load_string(view->lyric_view, html, NULL, NULL, "");
  mc_free(html);
  
  mc_free(lyric);
}

void library_view_edit_lyric(GtkToolButton *btn, GObject *lview) 
{
  library_view_t* view = (library_view_t*) g_object_get_data(lview, "library_view_t");
  
  if (view->current_lyric_track != NULL) {
    track_t* t = view->current_lyric_track;
    GtkLabel* lbl = GTK_LABEL(gtk_builder_get_object(view->builder, "lbl_lyr_edit_track"));
    char* s = (char*) mc_malloc(strlen(track_get_artist(t))+sizeof(", ")+strlen(track_get_title(t))+200);
    sprintf(s,"<span size=\"x-small\"><i><b>%s\n%s</b></i></span>", track_get_artist(t), track_get_title(t));
    gtk_label_set_markup(lbl,s);
    mc_free(s);
    
    GtkDialog* dlg = GTK_DIALOG(gtk_builder_get_object(view->builder, "dlg_edit_lyric"));
    g_object_set_data(G_OBJECT(dlg), "track", (gpointer) t );
    
    GtkTextView* tv = GTK_TEXT_VIEW(gtk_builder_get_object(view->builder, "txt_lyric_edit"));
    GtkTextBuffer* buf = gtk_text_view_get_buffer(tv);
    const char* lyric = track_get_lyric(view->current_lyric_track);
    gtk_text_buffer_set_text(buf, lyric, -1);
    int response = gtk_dialog_run(dlg);
    if (response) {
      GtkTextIter start, end;
      gtk_text_buffer_get_iter_at_offset(buf, &start, 0);
      gtk_text_buffer_get_iter_at_offset(buf, &end, -1);
      char* txt = gtk_text_buffer_get_text(buf, &start, &end, FALSE);
      write_lyric(view->current_lyric_track, txt, el_true);
      track_set_lyric(view->current_lyric_track, txt);
      char* html = lyric_text_to_html(txt);
      webkit_web_view_load_string(view->lyric_view, html, NULL, NULL, "");
      mc_free(html);
      g_free(txt);
    }
    gtk_widget_hide(GTK_WIDGET(dlg));
  }
}


static gboolean library_view_update_info(library_view_t* view)
{
  if (view->run_timeout) {
    
    // update play buttons
    playlist_player_t* player = backtobasics_player(view->btb);
    if (view->btn_play == NULL) {
      view->btn_play = GTK_WIDGET(gtk_builder_get_object(view->builder, "tbtn_library_play"));
    }
    if (view->btn_pause == NULL) {
      view->btn_pause = GTK_WIDGET(gtk_builder_get_object(view->builder, "tbtn_library_pause"));
    }
    GtkWidget* btn_play = view->btn_play;
    GtkWidget* btn_pause = view->btn_pause;
    if (!playlist_player_is_playing(player)) {
      if (!gtk_widget_get_visible(btn_play)) gtk_widget_show_all(btn_play);
      if (gtk_widget_get_visible(btn_pause)) gtk_widget_hide(btn_pause);
    } else if (playlist_player_is_playing(player)) {
      if (gtk_widget_get_visible(btn_play)) gtk_widget_hide(btn_play);
      if (!gtk_widget_get_visible(btn_pause)) gtk_widget_show_all(btn_pause);
    }
    
    // update slider and time
    long tr_tm = playlist_player_get_track_position_in_ms(player);
    track_t* track = playlist_player_get_track(player);
    int index = playlist_player_get_track_index(player);
    
    int a = tr_tm / 1000;
    int b = view->time_in_ms / 1000;
    
    if (a != b) {
      view->time_in_ms = tr_tm;
      int min = tr_tm / 1000 / 60;
      int sec = (tr_tm / 1000) % 60;
      { 
        char s[200]; 
        sprintf(s,"<span size=\"x-small\"><b><i>%02d:%02d</i></b></span>", min, sec);
        GtkLabel* lbl = GTK_LABEL(gtk_builder_get_object(view->builder, "lbl_time"));
        gtk_label_set_markup(lbl, s);
      }
      
      GtkScale* sc_playback = GTK_SCALE(gtk_builder_get_object(view->builder, "sc_library_playback"));
      double perc = 0.0;
      if (track != NULL) {
        int len_in_ms = track_get_length_in_ms(track);
        
        if (len_in_ms != view->len_in_ms) {
          view->len_in_ms = len_in_ms;
          int min = len_in_ms / 1000 / 60;
          int sec = (len_in_ms / 1000) % 60;
          {
            char s[200];
            sprintf(s,"<span size=\"x-small\"><b><i>%02d:%02d</i></b></span>", min, sec);
            GtkLabel* lbl = GTK_LABEL(gtk_builder_get_object(view->builder, "lbl_total"));
            gtk_label_set_markup(lbl, s);
          }
        }
        
        perc = (((double) tr_tm) / ((double) len_in_ms)) * 100.0;
        if (!view->sliding) {
          gtk_range_set_value(GTK_RANGE(sc_playback), perc);
        }
      }

      // update track info
      if (index != view->track_index || 
            (track != NULL && track_get_id(track) != view->track_id)) {
        log_debug3("updating track info, index = %d, %p", index, track);
        view->track_index = index;
        if (track != NULL) {
          // fetch lyric if possible
          if (strcmp(track_get_lyric(track),"") == 0) {
            struct lyric_cb* cb = (struct lyric_cb*) mc_malloc(sizeof(struct lyric_cb));
            cb->track = track;
            cb->view = view;
            fetch_lyric(track, library_view_process_lyric, cb);
          } else {
            struct lyric_cb* cb = (struct lyric_cb*) mc_malloc(sizeof(struct lyric_cb));
            cb->track = track;
            cb->view = view;
            library_view_process_lyric(mc_strdup(track_get_lyric(track)), cb);
          }
            
          // Print artist info
          view->track_id = track_get_id(track);
          log_debug2("artid = %s", track_get_artid(track));
          char s[200];
          char c = ',';
          char c1 = ',';
          char *artist = text_to_html(track_get_artist(track));
          char *title = text_to_html(track_get_title(track));
          char *piece = text_to_html(track_get_piece(track));
          
          if (strcmp(track_get_artist(track), "") == 0) { c = ' '; }
          if (strcmp(track_get_piece(track), "") == 0) { c1 = ' '; }
          snprintf(s, 125,
                   "%s%c %s%c %s", 
                   artist,
                   c,
                   piece,
                   c1,
                   title
                   );
          mc_free(artist);
          mc_free(title);
          mc_free(piece);
          char ss[400];
          log_debug2("s = %s", s);
          sprintf(ss,"<span size=\"x-small\"><i><b>%s</b></i></span>",s);
          GtkLabel* lbl = GTK_LABEL(gtk_builder_get_object(view->builder, "lbl_song_info"));
          gtk_label_set_markup(lbl, ss);
          
          log_debug2("artid = %s", track_get_artid(track));
          file_info_t* info = file_info_new(track_get_artid(track));
          if (!file_info_is_file(info)) {
            file_info_destroy(info);
            char *path = backtobasics_logo(view->btb); 
            info = file_info_new(path);
            mc_free(path);
            //info = file_info_new(backtobasics_logo(view->btb));
          }
          
          if (file_info_is_file(info)) {
            GError *err = NULL;
            GdkPixbuf* pb = gdk_pixbuf_new_from_file_at_scale(file_info_path(info),
                                                              view->img_w, view->img_h,
                                                              TRUE,
                                                              &err
                                                              );
            if (pb != NULL) {
              GtkImage* img = GTK_IMAGE(gtk_builder_get_object(view->builder, "img_art"));
              gtk_image_set_from_pixbuf(img, pb);
              g_object_unref(pb);
            } else {
              log_error3("error loading image art: %d, %s", err->code, err->message);
              //g_free(err);
            }
          }
          file_info_destroy(info);
        }

        log_debug("track hash");

        
        // Select the track in the librarylist if the librarylist is still
        // the same
        if (playlist_model_tracks_hash(view->playlist_model) == playlist_player_get_hash(player)) {
          GtkTreeView* tview = view->tview;
          GtkTreePath* path = gtk_tree_path_new();
          gtk_tree_path_append_index(path, index);
          gtk_tree_view_set_cursor(tview, path, NULL, FALSE);
          gtk_tree_path_free(path);
        } 
        log_debug("track hash 2");
        
      }

      //log_debug3("lib hash = %lld, pl hash = %lld", view->library_list_hash, playlist_player_get_hash(player));
      
      // TODO
      if (playlist_model_tracks_hash(view->playlist_model) ==  playlist_player_get_hash(player)) {
        //view->track_index = -1;
      } else {
        
        GtkTreeView* tview = view->tview;
        GtkTreeSelection* sel = gtk_tree_view_get_selection(tview);
        gtk_tree_selection_unselect_all(sel);
      }
      
    }
    
    // update repeat info
    playlist_player_repeat_t repeat = playlist_player_get_repeat(player);
    if (view->repeat != repeat) {
      log_debug3("repeat = %d, view repeat = %d", repeat, view->repeat);
      view->repeat = repeat;
      GtkWidget* r_btn = GTK_WIDGET(gtk_builder_get_object(view->builder, "tbtn_repeat"));
      GtkWidget* r1_btn = GTK_WIDGET(gtk_builder_get_object(view->builder, "tbtn_repeat_one"));
      GtkWidget* rlist_btn = GTK_WIDGET(gtk_builder_get_object(view->builder, "tbtn_repeat_all"));
      log_debug4("r = %p, r1 = %p, rall = %p", r_btn, r1_btn, rlist_btn);
      switch (repeat) {
        case PLP_NO_REPEAT: {
          gtk_widget_show_all(r_btn);
          gtk_widget_hide(rlist_btn);
          gtk_widget_hide(r1_btn);
        }
        break;
        case PLP_TRACK_REPEAT: {
          gtk_widget_hide(r_btn);
          gtk_widget_show_all(r1_btn);
          gtk_widget_hide(rlist_btn);
        }
        break;
        case PLP_LIST_REPEAT: {
          gtk_widget_hide(r1_btn);
          gtk_widget_show_all(rlist_btn);
          gtk_widget_hide(r_btn);
        }
        break;
      }
    }

    
    return TRUE;
  } else {
    return FALSE;
  }
}

static void library_view_library_col_sort(GtkTreeViewColumn* col, library_view_t* view)
{
  playlist_column_enum e;
  for( e = 0; view->cols[e] != col; ++e);
  playlist_model_sort(view->playlist_model, e);
}

void library_view_col_visible(GtkCheckMenuItem* item, GObject* lview)
{
  library_view_t* view = (library_view_t*) g_object_get_data(lview, "library_view_t");
  
  if (view->column_layout_changing) {
    return;
  }
  
  //long long hash = playlist_player_get_hash(backtobasics_player(view->btb));
  long long hash = playlist_model_tracks_hash(view->playlist_model);
  
  const char* names[] = {
    "chk_col_nr", "chk_col_title", "chk_col_artist", "chk_col_composer",
    "chk_col_piece", "chk_col_album", "chk_col_albumartist", "chk_col_genre",
    "chk_col_year", "chk_col_length", NULL
  };
  const playlist_column_enum es[] = {
      PLAYLIST_MODEL_COL_NR, PLAYLIST_MODEL_COL_TITLE, PLAYLIST_MODEL_COL_ARTIST,
      PLAYLIST_MODEL_COL_COMPOSER, PLAYLIST_MODEL_COL_PIECE, 
      PLAYLIST_MODEL_COL_ALBUM_TITLE, PLAYLIST_MODEL_COL_ALBUM_ARTIST,
      PLAYLIST_MODEL_COL_GENRE, PLAYLIST_MODEL_COL_YEAR, PLAYLIST_MODEL_COL_LENGTH,
      PLAYLIST_MODEL_N_COLUMNS
  };
  int i;
  const char* name = gtk_widget_get_name(GTK_WIDGET(item));
  for(i = 0; names[i] != NULL && strcmp(name, names[i]) != 0;++i);
  if (names[i] != NULL) {
    char cfgitem[200];
    sprintf(cfgitem,"library.cols.hash_%lld.%s", hash, names[i]);
    int active =  gtk_check_menu_item_get_active(item);
    el_config_set_int(btb_config(view->btb), cfgitem, active);
    gtk_tree_view_column_set_visible(view->cols[es[i]], gtk_check_menu_item_get_active(item));
  }
}

#define RIGHT_BUTTON 3

gboolean library_view_set_coverart(GtkImage* img, GdkEvent* evt, GObject* lview)
{
  library_view_t* view = (library_view_t*) g_object_get_data(lview, "library_view_t");
  GdkEventButton *button_evt = (GdkEventButton*) evt;
  if (button_evt->button == RIGHT_BUTTON) {
    GtkMenu* mnu = GTK_MENU(gtk_builder_get_object(view->builder, "mnu_coverart"));
    gtk_menu_popup (mnu, NULL, NULL, NULL, NULL, 
                        button_evt->button, button_evt->time);
    return TRUE;
  } else {
    return FALSE;
  }
}

static void image_fetched(void* p, el_bool fetched)
{
  if (fetched) {
    library_view_t* view = (library_view_t*) p;
    view->track_index = -1; // update the image for the current playing track
  }
}

static void web_set_url_cover_art(GtkClipboard* board, const gchar* uri, gpointer p)
{
  library_view_t* view = (library_view_t* ) p;
  if (uri != NULL) {
    char* image_uri = mc_strdup(uri);
    hre_trim(image_uri);
    log_debug3("got uri %s, view = %p", image_uri, view);
    if ((strncasecmp(image_uri, "http://", 7) == 0) ||
      (strncasecmp(image_uri, "https://", 8) == 0)) {
      GtkTreeSelection* sel = gtk_tree_view_get_selection(view->tview);
      GtkTreeModel* model;
      GtkTreeIter iter;
      log_debug2("tree selection: %p", sel);
      if (gtk_tree_selection_get_selected(sel, &model, &iter)) {
        log_debug("ok");
        int row = gtk_list_model_iter_to_row(GTK_LIST_MODEL(model), iter);
        track_t* track = playlist_model_get_track(view->playlist_model, row);
        log_debug2("track = %p", track);
        const char* artid = track_get_artid(track);
        log_debug3("fetch image %s, %s", image_uri, artid);
        fetch_image(image_uri, artid, image_fetched, view);
      } else {
        GtkMessageDialog* dialog = GTK_MESSAGE_DIALOG(
                                    gtk_message_dialog_new(btb_main_window(view->btb),
                                                           GTK_DIALOG_DESTROY_WITH_PARENT,
                                                           GTK_MESSAGE_ERROR,
                                                           GTK_BUTTONS_CLOSE,
                                                           _("Please select a track to set the cover art for")
                                                           )
                                    );
        gtk_dialog_run (GTK_DIALOG (dialog));
        gtk_widget_destroy (GTK_WIDGET(dialog));        
      }
    } else {
      GtkMessageDialog* dialog = GTK_MESSAGE_DIALOG( 
                                  gtk_message_dialog_new (btb_main_window(view->btb),
                                                         GTK_DIALOG_DESTROY_WITH_PARENT,
                                                         GTK_MESSAGE_ERROR,
                                                         GTK_BUTTONS_CLOSE,
                                                         _("Copy an http URL for an image to the clipboard")
                                                         )
                                  );
      gtk_dialog_run (GTK_DIALOG (dialog));
      gtk_widget_destroy (GTK_WIDGET(dialog));        
    }
    mc_free(image_uri);
  }
}

void library_view_cover_art_from_web(GtkMenuItem* item, GObject* lview)
{
  library_view_t* view = (library_view_t*) g_object_get_data(lview, "library_view_t");
  GtkClipboard* board = gtk_clipboard_get(GDK_SELECTION_CLIPBOARD);
  gtk_clipboard_request_text(board, web_set_url_cover_art, view);
}


void library_view_search_cover_art(GtkMenuItem* item, GObject* lview)
{
  library_view_t* view = (library_view_t*) g_object_get_data(lview, "library_view_t");

  GtkTreeSelection* sel = gtk_tree_view_get_selection(view->tview);
  GtkTreeModel* model;
  GtkTreeIter iter;

  if (gtk_tree_selection_get_selected(sel, &model, &iter)) {
    int row = gtk_list_model_iter_to_row(GTK_LIST_MODEL(model), iter);
    track_t* track = playlist_model_get_track(view->playlist_model, row);

    char s[10240];
    strcpy(s, "https://www.google.nl/search?tbm=isch&q=%s");
    char url[20480];
    char query[10240];
  
    sprintf(query,"%s+%s",track_get_artist(track), track_get_album_title(track));
    char* q = to_http_get_query(query);
  
    sprintf(url, s, q);
    open_url(GTK_WIDGET(item), url);
    
    mc_free(q);
  } else {
    GtkMessageDialog* dialog = GTK_MESSAGE_DIALOG(
                                gtk_message_dialog_new(btb_main_window(view->btb),
                                                       GTK_DIALOG_DESTROY_WITH_PARENT,
                                                       GTK_MESSAGE_ERROR,
                                                       GTK_BUTTONS_CLOSE,
                                                       _("Please select a track to set the cover art for")
                                                       )
                                );
    gtk_dialog_run (GTK_DIALOG (dialog));
    gtk_widget_destroy (GTK_WIDGET(dialog));        
  }
  
}

static void get_column_layout(library_view_t* view, long long hash) 
{
  view->column_layout_changing = el_true;
  
  const char* names[] = {
    "chk_col_nr", "chk_col_title", "chk_col_artist", "chk_col_composer",
    "chk_col_piece", "chk_col_album", "chk_col_albumartist", "chk_col_genre",
    "chk_col_year", "chk_col_length", NULL
  };
  const playlist_column_enum es[] = {
      PLAYLIST_MODEL_COL_NR, PLAYLIST_MODEL_COL_TITLE, PLAYLIST_MODEL_COL_ARTIST,
      PLAYLIST_MODEL_COL_COMPOSER, PLAYLIST_MODEL_COL_PIECE, 
      PLAYLIST_MODEL_COL_ALBUM_TITLE, PLAYLIST_MODEL_COL_ALBUM_ARTIST,
      PLAYLIST_MODEL_COL_GENRE, PLAYLIST_MODEL_COL_YEAR, PLAYLIST_MODEL_COL_LENGTH,
      PLAYLIST_MODEL_N_COLUMNS
  };
  
  int i;
  for(i = 0; names[i] != NULL; ++i) {
    GtkCheckMenuItem* item = GTK_CHECK_MENU_ITEM(gtk_builder_get_object(view->builder, names[i]));
    char cfgitem[100];
    sprintf(cfgitem,"library.cols.%lld, %s", hash, names[i]);
    int visible = el_config_get_int(btb_config(view->btb), cfgitem, 1);
    gtk_tree_view_column_set_visible(view->cols[es[i]], visible);
    gtk_check_menu_item_set_active(item, visible);    
  }

  view->column_layout_changing = el_false;
}


/*************************************************************************************
 * Playlists functionality
 *************************************************************************************/

void library_view_playlists_toggle(GtkToggleToolButton* btn, GObject* lview)
{
  library_view_t* view = (library_view_t*) g_object_get_data(lview, "library_view_t");
  GtkWidget* scw_playlists = GTK_WIDGET(gtk_builder_get_object(view->builder,"scw_playlists"));
  
  if (gtk_toggle_tool_button_get_active(btn)) {
    gtk_widget_set_no_show_all(scw_playlists, FALSE);
    gtk_widget_show_all(scw_playlists);
  } else {
    gtk_widget_hide(scw_playlists);
  }
}
