/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * main.c
 * Copyright (C) Hans Oesterholt 2013 <debian@oesterholt.net>
 * 
 */

#include <config.h>
#include <gtk/gtk.h>
#include "backtobasics.h"
#include <elementals.h>
#include <curl/curl.h>

#include <glib/gi18n.h>

static char* prgname;

int main (int argc, char *argv[])
{
	Backtobasics *app;
	int status;

	mc_init();
	prgname = mc_strdup(argv[0]);
	
	curl_global_init(CURL_GLOBAL_ALL);

#ifdef ENABLE_NLS
	bindtextdomain (GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR);
	bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
	textdomain (GETTEXT_PACKAGE);
#endif
	
  app = backtobasics_new ();
  status = g_application_run (G_APPLICATION (app), argc, argv);
  g_application_quit(G_APPLICATION(app));
  g_object_unref (app);
  
  mc_free(prgname);

  return status;
}

const char* dollar0(void)
{
  return prgname;
}
