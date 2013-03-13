/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * backtobasics.c
 * Copyright (C) 2013 Hans Oesterholt <debian@oesterholt.net>
 * 
 * BackToBasics is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * BackToBasics is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "backtobasics.h"

#include <glib/gi18n.h>

int log_this_severity(int severity) 
{
  return 1;
}

FILE* log_handle()
{
  return stderr;
}


/* For testing propose use the local (not installed) ui file */
/* #define UI_FILE PACKAGE_DATA_DIR"/ui/backtobasics.ui" */
#define UI_FILE "./backtobasics.ui"
#define TOP_WINDOW "window"

G_DEFINE_TYPE (Backtobasics, backtobasics, GTK_TYPE_APPLICATION);

/* Define the private structure in the .c file */
#define BACKTOBASICS_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE((obj), BACKTOBASICS_TYPE_APPLICATION, BacktobasicsPrivate))

struct _BacktobasicsPrivate
{
	/* ANJUTA: Widgets declaration for backtobasics.ui - DO NOT REMOVE */
	void* nothing;
};

GtkBuilder* backtobasics_builder(Backtobasics* app)
{
  return app->builder;
}

library_t* backtobasics_library(Backtobasics* app)
{
  return app->library;
}

playlist_player_t* backtobasics_player(Backtobasics* app)
{
  return app->player;
}

const char* backtobasics_logo(Backtobasics* app)
{
  return "../images/logo_vague.png";
}

/* Create a new window */
static void backtobasics_new_window (GApplication *app)
{
	GtkWidget *window;

	GtkBuilder *builder;
	GError* error = NULL;

	BacktobasicsPrivate *priv = BACKTOBASICS_GET_PRIVATE(app);

	/* Load UI from file */
	builder = gtk_builder_new ();
	if (!gtk_builder_add_from_file (builder, UI_FILE, &error)) {
		g_critical ("Couldn't load builder file: %s", error->message);
		g_error_free (error);
	}

  Backtobasics* btb = (Backtobasics*) app;
  btb->builder = builder;
  
  /* create views */
  btb->library_view = library_view_new(btb, btb->library);
  library_view_init(btb->library_view);
  btb->active_view = btb->library_view;
  
  btb->radio_view = radio_view_new(btb, btb->radio_library);
  radio_view_init(btb->radio_view);

	/* Auto-connect signal handlers */
	gtk_builder_connect_signals (builder, app);

	/* Get the window object from the ui file */
	window = GTK_WIDGET (gtk_builder_get_object (builder, TOP_WINDOW));
  if (!window) {
    g_critical ("Widget \"%s\" is missing in file %s.", TOP_WINDOW, UI_FILE);
  }
  int x = btb_config_get_int(btb, "main.window.x", 25);
  int y = btb_config_get_int(btb, "main.window.y", 25);
  int w = btb_config_get_int(btb, "main.window.w", 500);
  int h = btb_config_get_int(btb, "main.window.h", 400);
  gtk_window_move(GTK_WINDOW(window), x, y);
  gtk_window_resize(GTK_WINDOW(window), w, h);
  
  g_object_set_data(G_OBJECT(window), "btb", btb);
	
	/* ANJUTA: Widgets initialization for backtobasics.ui - DO NOT REMOVE */

	//g_object_unref (builder);
	
	gtk_window_set_application (GTK_WINDOW (window), GTK_APPLICATION (app));
	gtk_widget_show_all (GTK_WIDGET (window));

	// Hide things we want to hide
  GtkToolButton* normal_window = GTK_TOOL_BUTTON(gtk_builder_get_object(builder, "tbtn_normal"));
  gtk_widget_hide(GTK_WIDGET(normal_window));
  
  // Initialize things we want to initialize
  GtkScale* volume_scale = GTK_SCALE(gtk_builder_get_object(builder, "sc_volume"));
  gtk_range_set_range(GTK_RANGE(volume_scale),0.0,100.0);
  
  double volperc = (double) btb_config_get_int(btb, "previous_volume", 100);  
  gtk_range_set_value(GTK_RANGE(volume_scale), volperc);

}


/* GApplication implementation */
static void backtobasics_activate (GApplication *application)
{
  backtobasics_new_window (application);
}

static void backtobasics_init (Backtobasics *btb)
{
  config_init(btb_config(btb));
  
  btb->volume_setter_active = el_false;
  
  file_info_t* btb_cfg_dir = file_info_new_home(".backtobasics");
  if (!file_info_is_dir(btb_cfg_dir)) {
    file_info_mkdir_p(btb_cfg_dir, S_IXUSR|S_IWUSR|S_IRUSR|S_IXGRP|S_IRGRP);
  }
  
  file_info_t* info = file_info_combine(btb_cfg_dir, "btb.cfg");
  config_read_file(btb_config(btb), file_info_absolute_path(info));
  file_info_destroy(info);
  
  // Library
  info = file_info_combine(btb_cfg_dir, "btb.lib");
  btb->library = library_new();
  if (file_info_exists(info)) {
    library_load(btb->library, file_info_absolute_path(info));
  }
  
  file_info_t* libhome = file_info_new_home("Music");
  char* path = btb_config_get_string(btb, "library.path", file_info_absolute_path(libhome));
  file_info_destroy(libhome);
  library_set_basedir(btb->library, path);
  mc_free(path);
  
  //scan_library(NULL, NULL, btb->library);
  
  file_info_destroy(info);
  
  
  // Radio library
  btb->radio_library = radio_library_new();
  info = file_info_combine(btb_cfg_dir, "btb_radio.cfg");
  if (file_info_exists(info)) {
    radio_library_load(btb->radio_library, file_info_absolute_path(info));
  }
  file_info_destroy(info);
  
  file_info_destroy(btb_cfg_dir);
  
  btb->player = playlist_player_new();
}

static void backtobasics_finalize (GObject *object)
{
  Backtobasics* btb = (Backtobasics*) object;
  
  log_debug("BackToBasics finalizing");  
  
  // writing
  
	file_info_t* btb_cfg_dir = file_info_new_home(".backtobasics");
	file_info_t* info;
	
  info = file_info_combine(btb_cfg_dir, "btb.lib");
  library_save(btb->library, file_info_absolute_path(info));
  file_info_destroy(info);
  
  btb_config_set_string(btb, "library.path", library_get_basedir(btb->library));
  
  // finalizing
  
  GtkWidget* window = GTK_WIDGET(gtk_builder_get_object (btb->builder, TOP_WINDOW));
  gtk_widget_destroy(window);
  
  library_view_destroy(btb->library_view); 
  radio_view_destroy(btb->radio_view);

  playlist_player_destroy(btb->player);
  library_destroy(btb->library);
  radio_library_destroy(btb->radio_library);

  // Writing configuration
  
	info = file_info_combine(btb_cfg_dir, "btb.cfg");
  config_write_file(btb_config(btb), file_info_absolute_path(info));
  file_info_destroy(info);
  
  file_info_destroy(btb_cfg_dir);
  
  // Finalizing rest
  
	G_OBJECT_CLASS (backtobasics_parent_class)->finalize (object);
  
}

static void backtobasics_class_init (BacktobasicsClass *klass)
{
	G_APPLICATION_CLASS (klass)->activate = backtobasics_activate;
	g_type_class_add_private (klass, sizeof (BacktobasicsPrivate));
	G_OBJECT_CLASS (klass)->finalize = backtobasics_finalize;
}

Backtobasics *backtobasics_new (void)
{
	g_type_init ();
	return g_object_new (backtobasics_get_type (),
	                     "BackToBasics", "net.oesterholt.backtobasics",
	                     NULL);
}

config_t* btb_config(Backtobasics* btb)
{
  return &btb->config;
}

config_setting_t* btb_get_setting(Backtobasics* btb, const char* path, int type)
{
  config_setting_t *root, *setting;
  
  root = config_root_setting(btb_config(btb));
  char* pt = mc_strdup(path);
  char* p[100];
  int k = 0, i, l;
  p[k++] = &pt[0];
  for(i = 0, l = strlen(pt);i < l; ++i) {
    if (pt[i] == '.') {
      pt[i] = '\0';
      p[k] = &pt[i+1];
      k += 1;
    }
  }
  p[k++] = NULL;
  
  setting = root;
  for(i = 0; p[i] != NULL; ++i) {
    int t = CONFIG_TYPE_GROUP;
    if (p[i+1] == NULL) { t = type; }
    config_setting_t* tmp = setting;
    setting = config_setting_get_member(tmp, p[i]);
    if (!setting) 
      setting = config_setting_add(tmp, p[i], t);
  }
  
  mc_free(pt);

  return setting;
}

int btb_config_get_int(Backtobasics* btb, const char* path, int default_val)
{
  int v = default_val;
  config_t* config = btb_config(btb);
  config_lookup_int(config, path, (long*) &v);
  return v;
}

char* btb_config_get_string(Backtobasics* btb, const char* path, const char* default_val)
{
  const char* val = NULL;
  config_t* config = btb_config(btb);
  config_lookup_string(config, path, &val);
  if (val == NULL) {
    return mc_strdup(default_val);
  } else {
    return mc_strdup(val);
  }
}

void btb_config_set_int(Backtobasics* btb, const char* path, int val)
{
  config_setting_t* setting = btb_get_setting(btb, path, CONFIG_TYPE_INT);
  config_setting_set_int(setting, val);
}

void btb_config_set_string(Backtobasics* btb, const char* path, const char* val)
{
  config_setting_t* setting = btb_get_setting(btb, path, CONFIG_TYPE_STRING);
  config_setting_set_string(setting, val);
}

/************************************************************************
 * Implementing signal handlers
 ************************************************************************/
 
void btb_setup(GtkWidget* menu_item, GtkWidget* window)
{
  Backtobasics* btb = g_object_get_data(G_OBJECT(window), "btb");
  GtkDialog* setupdlg = GTK_DIALOG(gtk_builder_get_object(btb->builder, "dlg_setup"));
  gtk_dialog_run(setupdlg);
  gtk_widget_hide(GTK_WIDGET(setupdlg));
  
  GtkFileChooserButton* fc = GTK_FILE_CHOOSER_BUTTON(gtk_builder_get_object(btb->builder, "btn_choose_library_loc"));
  char* path = gtk_file_chooser_get_uri(GTK_FILE_CHOOSER(fc));
  library_set_basedir(btb->library, path);
}

void btb_scan_library(GtkWidget* menu_item, GtkWidget* window)
{
  Backtobasics* btb = g_object_get_data(G_OBJECT(window), "btb");
  GtkFileChooserButton* fc = GTK_FILE_CHOOSER_BUTTON(gtk_builder_get_object(btb->builder, "btn_choose_library_loc"));
  char* path = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(fc));
  library_set_basedir(btb->library, path);
  log_debug2("choosen: %s", path);
  run_scan_job_dialog(_("Scanning music library"), scan_library, btb->builder, btb->library);
  library_view_reset_models(btb->library_view);
}

void btb_fullscreen(GtkToolButton* btn, GtkWidget* window)
{
  Backtobasics* btb = g_object_get_data(G_OBJECT(window), "btb");
  GtkWidget* fbtn = GTK_WIDGET(gtk_builder_get_object(btb->builder, "tbtn_fullscreen"));
  GtkWidget* nbtn = GTK_WIDGET(gtk_builder_get_object(btb->builder, "tbtn_normal"));
  gtk_widget_hide(fbtn);
  gtk_widget_show_all(nbtn);
  gtk_window_fullscreen(GTK_WINDOW(window));
}

void btb_normal_screen(GtkToolButton* btn, GtkWidget* window)
{
  Backtobasics* btb = g_object_get_data(G_OBJECT(window), "btb");
  GtkWidget* fbtn = GTK_WIDGET(gtk_builder_get_object(btb->builder, "tbtn_fullscreen"));
  GtkWidget* nbtn = GTK_WIDGET(gtk_builder_get_object(btb->builder, "tbtn_normal"));
  gtk_widget_show_all(fbtn);
  gtk_widget_hide(nbtn);
  gtk_window_unfullscreen(GTK_WINDOW(window));
}


static gboolean volume_setter(gpointer _btb)
{
  Backtobasics* btb = (Backtobasics*) _btb;
  double perc = (double) btb_config_get_int(btb, "previous_volume", 100);
  playlist_player_set_volume(btb->player, perc);
  btb->volume_setter_active = el_false;
  return FALSE;
}

void btb_set_volume(GtkRange* volume, GtkWidget* window)
{
  Backtobasics* btb = g_object_get_data(G_OBJECT(window), "btb");
  double perc = gtk_range_get_value(volume);
  if (!btb->volume_setter_active == el_true) {
    btb->volume_setter_active = el_true;
    g_timeout_add(100, volume_setter, btb);
  }
  btb_config_set_int(btb, "previous_volume", (int) perc);
}


void btb_volume_mute(GtkToggleButton* volume, GtkWidget* window)
{
  Backtobasics* btb = g_object_get_data(G_OBJECT(window), "btb");
  if (gtk_toggle_button_get_active(volume)) {
    playlist_player_set_volume(btb->player, 0.0);
  } else {
    double perc = (double) btb_config_get_int(btb, "previous_volume", 100);
    playlist_player_set_volume(btb->player, perc);
  }
}
 
static void select_view(gpointer data, const char* name)
{
  Backtobasics* btb = (Backtobasics*) data;
  GtkNotebook* book = GTK_NOTEBOOK(gtk_builder_get_object(btb->builder, "views"));
  GtkWidget* widget = GTK_WIDGET(gtk_builder_get_object(btb->builder, name));
  gint pn = gtk_notebook_page_num(book, widget);
  gtk_notebook_set_current_page(book, pn);
}
 
void go_radio(GObject* object, gpointer data)
{
  Backtobasics* btb = (Backtobasics*) data;
  select_view(data, "radio_view");
  //btb->active_view = btb->radio_view;
}

void go_library(GObject* object, gpointer data) 
{
  Backtobasics* btb = (Backtobasics*) data;
  select_view(data, "view_library");
  btb->active_view = btb->library_view;
  
}

void menu_quit(GObject* object, gpointer data)
{
  GApplication* app = (GApplication*) data; 
  Backtobasics* btb = (Backtobasics*) app;
  GtkBuilder* builder = btb->builder;
     
  GtkWindow* window = GTK_WINDOW (gtk_builder_get_object (builder, TOP_WINDOW));
  gint x, y;
  gtk_window_get_position(window, &x, &y);
  gint w,h;
  gtk_window_get_size(window, &w, &h);
  btb_config_set_int(btb, "main.window.x", x);
  btb_config_set_int(btb, "main.window.y", y);
  btb_config_set_int(btb, "main.window.w", w);
  btb_config_set_int(btb, "main.window.h", h);
  
  g_application_quit(app);
  g_object_unref(app);
}
