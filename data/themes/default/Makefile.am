MAINTAINERCLEANFILES = Makefile.in

THEME_NAME=default
EDJE_CC = @edje_cc@
EDJE_FLAGS = -v -id $(top_srcdir)/data/themes/$(THEME_NAME)/images \
                -fd $(top_srcdir)/data/themes/$(THEME_NAME)/fonts

filesdir = $(pkgdatadir)/themes
files_DATA = $(THEME_NAME).edj \
             images/enemy_standard.png \
             images/enemy_flyer.png \
             images/tower_normal_base.png \
             images/tower_normal_cannon.png \
             images/tower_normal_icon.png \
             images/tower_ghost_base.png \
             images/tower_ghost_cannon.png \
             images/tower_ghost_icon.png \
             images/bullet1.png

EXTRA_DIST = $(THEME_NAME).edc


$(THEME_NAME).edj: Makefile $(EXTRA_DIST)
	$(EDJE_CC) $(EDJE_FLAGS) \
	$(top_srcdir)/data/themes/$(THEME_NAME)/$(THEME_NAME).edc \
	$(top_builddir)/data/themes/$(THEME_NAME)/$(THEME_NAME).edj

clean-local:
	rm -f *.edj
