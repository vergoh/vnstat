if HAVE_CHECK
TESTS = check_vnstat
check_PROGRAMS = check_vnstat
check_vnstat_CFLAGS = @CHECK_CFLAGS@ \
  -DPROCNETDEV=\"testdir/proc/dev\" \
  -DSYSCLASSNET=\"testdir/sysclassnet\" \
  -DCFGFILE=\"$(top_srcdir)/cfg/vnstat.conf\" \
  -I$(top_srcdir)/src
check_vnstat_LDADD = @CHECK_LIBS@

check_vnstat_SOURCES = vnstat_tests.c vnstat_tests.h \
  common_tests.c common_tests.h \
  database_tests.c database_tests.h \
  config_tests.c config_tests.h \
  ifinfo_tests.c ifinfo_tests.h \
  misc_tests.c misc_tests.h \
  daemon_tests.c daemon_tests.h

check_vnstat_SOURCES += $(top_srcdir)/src/cfg.c $(top_srcdir)/src/cfg.h \
  $(top_srcdir)/src/common.c $(top_srcdir)/src/common.h \
  $(top_srcdir)/src/daemon.c $(top_srcdir)/src/daemon.h \
  $(top_srcdir)/src/dbaccess.c $(top_srcdir)/src/dbaccess.h \
  $(top_srcdir)/src/dbcache.c $(top_srcdir)/src/dbcache.h \
  $(top_srcdir)/src/dbjson.c $(top_srcdir)/src/dbjson.h \
  $(top_srcdir)/src/dbmerge.c $(top_srcdir)/src/dbmerge.h \
  $(top_srcdir)/src/dbshow.c $(top_srcdir)/src/dbshow.h \
  $(top_srcdir)/src/dbxml.c $(top_srcdir)/src/dbxml.h \
  $(top_srcdir)/src/ibw.c $(top_srcdir)/src/ibw.h \
  $(top_srcdir)/src/ifinfo.c $(top_srcdir)/src/ifinfo.h \
  $(top_srcdir)/src/misc.c $(top_srcdir)/src/misc.h \
  $(top_srcdir)/src/traffic.c $(top_srcdir)/src/traffic.h

endif

clean-local:
	-rm -fr *.gc* test.log test.xml testdir
