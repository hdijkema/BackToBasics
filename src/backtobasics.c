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
  
	
	/* ANJUTA: Widgets initialization for backtobasics.ui - DO NOT REMOVE */

	//g_object_unref (builder);
	
	gtk_window_set_application (GTK_WINDOW (window), GTK_APPLICATION (app));
	gtk_widget_show_all (GTK_WIDGET (window));
}


/* GApplication implementation */
static void backtobasics_activate (GApplication *application)
{
  backtobasics_new_window (application);
}

static void backtobasics_init (Backtobasics *btb)
{
  config_init(btb_config(btb));
  
  file_info_t* info = file_info_new_home(".backtobasics_cfg");
  config_read_file(btb_config(btb), file_info_absolute_path(info));
  file_info_destroy(info);
  
  btb->library = library_new();
  scan_library(btb->library,"/home/hans/Muziek", NULL);
  
  btb->player = playlist_player_new();
}

static void backtobasics_finalize (GObject *object)
{
  Backtobasics* btb = (Backtobasics*) object;
  
  log_debug("BackToBasics finalizing");  
  
  GtkWidget* window = GTK_WIDGET(gtk_builder_get_object (btb->builder, TOP_WINDOW));
  gtk_widget_destroy(window);
  
  library_view_destroy(btb->library_view); 

  playlist_player_destroy(btb->player);
  library_destroy(btb->library);

	G_OBJECT_CLASS (backtobasics_parent_class)->finalize (object);

	file_info_t* info = file_info_new_home(".backtobasics_cfg");
  config_write_file(btb_config(btb), file_info_absolute_path(info));
  file_info_destroy(info);
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

void btb_config_set_int(Backtobasics* btb, const char* path, int val)
{
  config_setting_t* setting = btb_get_setting(btb, path, CONFIG_TYPE_INT);
  config_setting_set_int(setting, val);
}

/************************************************************************
 * Implementing signal handlers
 ************************************************************************/
 
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
  select_view(data, "radio_view");
}

void go_library(GObject* object, gpointer data) 
{
  select_view(data, "view_library");
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
