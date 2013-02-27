## Process this file with automake to produce Makefile.in

## Created by Anjuta


uidir = $(pkgdatadir)/ui
ui_DATA = backtobasics.ui


AM_CPPFLAGS = \
	-DPACKAGE_LOCALE_DIR=\""$(localedir)"\" \
	-DPACKAGE_SRC_DIR=\""$(srcdir)"\" \
	-DPACKAGE_DATA_DIR=\""$(pkgdatadir)"\" \
	$(BACKTOBASICS_CFLAGS)

AM_CFLAGS =\
	 -Wall\
	 -g

bin_PROGRAMS = backtobasics \
	btb_play

backtobasics_SOURCES = \
	main.c \
	backtobasics.h \
	backtobasics.c	

backtobasics_LDFLAGS = \
	-Wl,--export-dynamic

backtobasics_LDADD = $(BACKTOBASICS_LIBS)

btb_play_SOURCES =  \
	audio/aodev.c \
	audio/aodev.h \
	audio/audio.c \
	audio/audio.h \
	audio/mp3.c \
	audio/mp3.h \
	btb_play.c


EXTRA_DIST = $(ui_DATA)

# Remove ui directory on uninstall
uninstall-local:
	-rm -r $(uidir)
	-rm -r $(pkgdatadir)

