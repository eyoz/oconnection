ACLOCAL_AMFLAGS = -I m4
MAINTAINERCLEANFILES = Makefile.in aclocal.m4 config.guess config.h.in \
		       config.sub configure depcomp install-sh ltmain.sh \
		       missing module.desktop config.rpath mkinstalldirs

SUBDIRS = src po

EDJE_FLAGS = -v \
	     -id $(top_srcdir)/images

filesdir = $(datadir)
files_DATA = module.desktop e-module-oconnection.edj

EXTRA_DIST = module.desktop.in \
	     e_modules-oconnection.spec.in \
	     e-module-oconnection.edc \
	     images/module_icon.png \
	     images/connection.png

%.edj:  %.edc
	$(EDJE_CC) $(EDJE_FLAGS) $< $@

clean-local:
	 rm -rf e-module-oconnection.edj module.desktop e_modules-oconnection.spec *~

uninstall:
	 rm -rf $(DESTDIR)$(datadir)

