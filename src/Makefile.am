
if ENABLE_TRAY_ICON
TRAY_ICON_CFLAGS = -DENABLE_TRAY_ICON
TRAY_ICON_files = brightside-tray.c brightside-tray.h eggtrayicon.h eggtrayicon.c
endif

if HAVE_ALSA
HAVE_ALSA_CFLAGS = -DHAVE_ALSA
ALSA_files = brightside-volume-alsa.c brightside-volume-alsa.h
endif

if HAVE_OSS
OSS_CFLAGS = -DHAVE_OSS
OSS_files = brightside-volume-oss.c brightside-volume-oss.h
endif

if HAVE_FB
FB_CFLAGS = -DHAVE_FB
FB_files = brightside-fb-level.c brightside-fb-level.h
endif

INCLUDES =						\
	$(EXTRA_GNOME_CFLAGS)				\
	$(TRAY_ICON_CFLAGS)				\
	$(ALSA_CFLAGS) $(HAVE_ALSA_CFLAGS) $(OSS_CFLAGS) $(FB_CFLAGS)	\
	-DBRIGHTSIDE_DATA=\""$(datadir)/brightside/"\"		\
	-DGNOMELOCALEDIR=\""$(datadir)/locale"\"	\
	-DWNCK_I_KNOW_THIS_IS_UNSTABLE

bin_PROGRAMS = brightside brightside-properties

brightside_SOURCES =						\
	brightside-volume.c brightside-volume.h			\
	brightside-volume-dummy.c brightside-volume-dummy.h		\
	$(TRAY_ICON_files) $(ALSA_files) $(OSS_files) $(FB_files)	\
	brightside-util.c brightside-util.h			\
	brightside.c brightside.h

brightside_LDADD =						\
	$(EXTRA_GNOME_LIBS)				\
	$(ALSA_LIBS)					\
	$(INTLLIBS)					\
	-lX11

brightside_properties_SOURCES =				\
	brightside-properties.c brightside-properties.h	\
	gconf-property-editor.c gconf-property-editor.h \
	gconf-property-editor-marshal.c gconf-property-editor-marshal.h	\
	preview-file-selection.c preview-file-selection.h	\
	custom-action.c custom-action.h brightside-properties-shared.h \
	capplet-util.h

CLEANFILES = *.bak *.gladep $(desktop_DATA) $(schema_DATA)

EGGFILES2 = eggtrayicon.h eggtrayicon.c

brightside_properties_LDADD = 			\
	$(EXTRA_GNOME_LIBS)			\
	$(INTLLIBS)				\
	-lX11

EXTRA_DIST =			\
	$(Data_DATA)		\
	$(schema_in_files)	\
	$(EGGFILES2)		\
	$(desktop_in_files)	\
	brightside-volume-alsa.h brightside-volume-alsa.c \
	brightside-volume-oss.c brightside-volume-oss.h \
	brightside-fb-level.c brightside-fb-level.h \
	brightside-tray.c brightside-tray.h 

Datadir = $(datadir)/brightside/
Data_DATA = 				\
	brightside.glade			\
	brightside-properties.glade		\
	custom-action.glade			\
	gnome-speakernotes.png gnome-speakernotes-muted.png	\
	brightside-brightness.png xscreensaver.png dpms.png	\
	gnome-lockscreen.png	\
	brightside.svg brightside-48.png brightside-16.png

schema_in_files = brightside.schemas.in
schemadir = $(sysconfdir)/gconf/schemas
schema_DATA = $(schema_in_files:.schemas.in=.schemas)
@INTLTOOL_SCHEMAS_RULE@

desktop_in_files = brightside.desktop.in
desktopdir = $(datadir)/control-center-2.0/capplets
desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)
@INTLTOOL_DESKTOP_RULE@

pixmapdir = $(datadir)/pixmaps
pixmap_DATA = brightside-48.png

install-data-local:
	-GCONF_CONFIG_SOURCE=$(GCONF_SCHEMA_CONFIG_SOURCE) $(GCONFTOOL) --makefile-install-rule $(srcdir)/$(schema_DATA)
