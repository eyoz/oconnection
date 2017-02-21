MAINTAINERCLEANFILES = Makefile.in

INCLUDES = -I. \
	   -I$(top_srcdir) \
	   -I$(includedir) \
	   @E_CFLAGS@ @NM_CFLAGS@

pkgdir = $(datadir)/$(MODULE_ARCH)
pkg_LTLIBRARIES = module.la
module_la_SOURCES = e_mod_main.h \
                    e_mod_main.c \
                    e_mod_config.c \
                    oconnection_network_manager.c \
                    oconnection_network_manager.h \
                    oconnection.h \
                    oconnection.c


module_la_LIBADD = @E_LIBS@ @NM_LIBS@
module_la_LDFLAGS = -module -avoid-version
module_la_DEPENDENCIES = $(top_builddir)/config.h

clean-local:
	rm -rf *~
