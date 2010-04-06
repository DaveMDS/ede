MAINTAINERCLEANFILES = Makefile.in

INCLUDES = -I$(top_srcdir) \
           -I$(top_srcdir)/src/bin \
           -DPACKAGE_DATA_DIR=\"$(datadir)/$(PACKAGE)\" \
           -DPACKAGE_LIB_DIR=\"$(libdir)\" \
           -DPACKAGE_BIN_DIR=\"$(bindir)\" \
           -DGETTEXT_PACKAGE=\"$(PACKAGE)\" \
           -DLOCALEDIR=\"$(localedir)\" \
           @EDE_CFLAGS@

bin_PROGRAMS = ede

ede_LDADD = @EDE_LIBS@
ede_LDFLAGS = -rdynamic
ede_CFLAGS = -Wall
ede_SOURCES = main.c \
              ede_gui.c \
              ede_level.c \
              ede_enemy.c \
              ede_astar.c \
              ede_game.c \
              ede_tower.c \
              ede_bullet.c

noinst_HEADERS = gettext.h
EXTRA_DIST = gettext.h \
             ede.h \
             ede_gui.h \
             ede_level.h \
             ede_enemy.h \
             ede_astar.h \
             ede_game.h \
             ede_tower.h \
             ede_bullet.h

