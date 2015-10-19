bin_PROGRAMS = vnstat vnstatd
if HAVE_LIBGD
bin_PROGRAMS += vnstati
endif
vnstati_LDADD = $(IMAGELIBS)

vnstat_SOURCES = vnstat.c vnstat.h \
  ifinfo.c ifinfo.h \
  dbxml.c dbxml.h \
  dbjson.c dbjson.h \
  dbshow.c dbshow.h \
  dbaccess.c dbaccess.h \
  dbmerge.c dbmerge.h \
  common.c common.h \
  misc.c misc.h \
  cfg.c cfg.h \
  ibw.c ibw.h \
  traffic.c traffic.h

vnstatd_SOURCES = vnstatd.c vnstatd.h \
  ifinfo.c ifinfo.h \
  dbaccess.c dbaccess.h \
  dbcache.c dbcache.h \
  common.c common.h \
  misc.c misc.h \
  cfg.c cfg.h \
  ibw.c ibw.h \
  daemon.c daemon.h

vnstati_SOURCES = vnstati.c vnstati.h \
  image.c image.h \
  dbaccess.c dbaccess.h \
  dbmerge.c dbmerge.h \
  common.c common.h \
  misc.c misc.h \
  cfg.c cfg.h

clean-local:
	-rm -f *~ core *.i
