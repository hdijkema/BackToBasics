## Process this file with automake to produce Makefile.in

## Created by Anjuta


uidir = $(pkgdatadir)/ui
ui_DATA = backtobasics.ui btb.png ../images/*

menu1dir = $(datarootdir)/applications
menu1_DATA = share/applications/*

menu2dir = $(datarootdir)/menu
menu2_DATA = share/menu/*

pixmapsdir = $(datarootdir)/pixmaps
pixmaps_DATA = share/pixmaps/*

iconsdir = $(datarootdir)/icons/64x64/apps
icons_DATA = share/icons/64x64/apps/*

btbdir = $(prefix)/bin
btb_DATA = backtobasics

AM_CPPFLAGS = \
	-DPACKAGE_LOCALE_DIR=\""$(localedir)"\" \
	-DPACKAGE_SRC_DIR=\""$(srcdir)"\" \
	-DPACKAGE_DATA_DIR=\""$(pkgdatadir)"\" \
	$(BACKTOBASICS_CFLAGS) \
	$(BTB_PLAY_CFLAGS)

AM_CFLAGS =\
	 -Wall\
	 -g

bin_PROGRAMS = backtobasics_bin 
	
GENERICSRC = \
	util/config.c \
	util/config.h \
	audio/aodev.c \
	audio/aodev.h \
	audio/audio.c \
	audio/audio.h \
	audio/mp3.c \
	audio/mp3.h \
	audio/ogg.c \
	audio/ogg.h \
	audio/flac.c \
	audio/flac.h \
	audio/ripstream.c \
	audio/ripstream.h \
	metadata/track.c \
	metadata/track.h \
	metadata/cue.c \
	metadata/cue.h \
	metadata/tracks_source.c \
	metadata/tracks_source.h \
	playlist/playlist.c \
	playlist/playlist.h \
	playlist/playlist_player.c \
	playlist/playlist_player.h \
	i18n/i18n.c \
	i18n/i18n.h \
	library/library.c \
	library/library.h \
	library/radio.c \
	library/radio.h


backtobasics_bin_SOURCES = \
	$(GENERICSRC) \
	main.c \
	backtobasics.h \
	backtobasics.c \
	gui/library_view.c \
	gui/library_view.h \
	gui/playlist_model.c \
	gui/playlist_model.h \
	gui/string_model.c \
	gui/string_model.h \
	gui/async_call.c \
	gui/async_call.h \
	gui/scanjob.c \
	gui/scanjob.h \
	gui/radio_view.c \
	gui/radio_view.h \
	gui/station_model.c \
	gui/station_model.h \
	gui/playlists_model.c \
	gui/playlists_model.h \
	lyrics/lyrics.c \
	lyrics/lyrics.h


backtobasics_bin_LDFLAGS = \
	-Wl,--export-dynamic

backtobasics_bin_LDADD = $(BACKTOBASICS_LIBS)

EXTRA_DIST = $(ui_DATA) $(icons_DATA) $(pixmaps_DATA) $(menu1_DATA) $(menu2_DATA)

# Remove ui directory on uninstall
uninstall-local:
	-rm -r $(uidir)
	-rm -r $(pkgdatadir)

