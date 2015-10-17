TESTS = check_vnstat
check_PROGRAMS = check_vnstat
check_vnstat_CFLAGS = @CHECK_CFLAGS@ -DPROCNETDEV=\"testdir/proc/dev\" -DSYSCLASSNET=\"testdir/sysclassnet\"
check_vnstat_LDADD = @CHECK_LIBS@ -lgd
check_vnstat_SOURCES = vnstat_tests.c vnstat_tests.h common_tests.c common_tests.h database_tests.c database_tests.h config_tests.c config_tests.h ifinfo_tests.c ifinfo_tests.h misc_tests.c misc_tests.h daemon_tests.c daemon_tests.h
check_vnstat_SOURCES += $(top_builddir)/src/cfg.c $(top_builddir)/src/cfg.h $(top_builddir)/src/common.c $(top_builddir)/src/common.h $(top_builddir)/src/daemon.c $(top_builddir)/src/daemon.h $(top_builddir)/src/dbaccess.c $(top_builddir)/src/dbaccess.h $(top_builddir)/src/dbcache.c $(top_builddir)/src/dbcache.h $(top_builddir)/src/dbjson.c $(top_builddir)/src/dbjson.h $(top_builddir)/src/dbmerge.c $(top_builddir)/src/dbmerge.h $(top_builddir)/src/dbshow.c $(top_builddir)/src/dbshow.h $(top_builddir)/src/dbxml.c $(top_builddir)/src/dbxml.h $(top_builddir)/src/ibw.c $(top_builddir)/src/ibw.h $(top_builddir)/src/ifinfo.c $(top_builddir)/src/ifinfo.h $(top_builddir)/src/image.c $(top_builddir)/src/image.h $(top_builddir)/src/misc.c $(top_builddir)/src/misc.h $(top_builddir)/src/traffic.c $(top_builddir)/src/traffic.h

clean-local:
	-rm -fr *.gc* test.log test.xml testdir
