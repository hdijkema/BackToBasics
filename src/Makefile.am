## Process this file with automake to produce Makefile.in

## Created by Anjuta


uidir = $(pkgdatadir)/ui
ui_DATA = backtobasics.ui


AM_CPPFLAGS = \
	-DPACKAGE_LOCALE_DIR=\""$(localedir)"\" \
	-DPACKAGE_SRC_DIR=\""$(srcdir)"\" \
	-DPACKAGE_DATA_DIR=\""$(pkgdatadir)"\" \
	$(BACKTOBASICS_CFLAGS) \
	$(BTB_PLAY_CFLAGS)

AM_CFLAGS =\
	 -Wall\
	 -g

bin_PROGRAMS = backtobasics \
	btb_play
	
GENERICSRC = \
	audio/aodev.c \
	audio/aodev.h \
	audio/audio.c \
	audio/audio.h \
	audio/mp3.c \
	audio/mp3.h \
	metadata/track.c \
	metadata/track.h \
	metadata/cue.c \
	metadata/cue.h \
	metadata/tracks_source.c \
	metadata/tracks_source.h \
	playlist/playlist.c \
	playlist/playlist.h \
	playlist/playlist_player.c \
	playlist/playlist_player.h 


backtobasics_SOURCES = \
  $(GENERICSRC) \
	main.c \
	backtobasics.h \
	backtobasics.c

backtobasics_LDFLAGS = \
	-Wl,--export-dynamic

backtobasics_LDADD = $(BACKTOBASICS_LIBS)

btb_play_SOURCES =  \
  $(GENERICSRC) \
  btb_play.c

btb_play_CFLAGS = $(TARGET_READLINE_INC)
  
btb_play_LDADD = \
	$(BTB_PLAY_LIBS) $(TARGET_READLINE_LIBS)


EXTRA_DIST = $(ui_DATA)

# Remove ui directory on uninstall
uninstall-local:
	-rm -r $(uidir)
	-rm -r $(pkgdatadir)

