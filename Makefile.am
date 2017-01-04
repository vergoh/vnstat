SUBDIRS = .

dist_man_MANS = man/vnstat.1 man/vnstat.conf.5 man/vnstatd.1 man/vnstati.1

EXTRA_DIST = cfg examples UPGRADE CHANGES FAQ INSTALL_BSD INSTALL_OSX UNINSTALL

#bin_PROGRAMS = vnstat
sbin_PROGRAMS = vnstatd
#if HAVE_LIBGD
#bin_PROGRAMS += vnstati
#endif
#vnstati_LDADD = $(IMAGELIBS)

#vnstat_SOURCES = src/vnstat.c src/vnstat.h \
#  src/ifinfo.c src/ifinfo.h \
#  src/dbsql.c src/dbsql.h \
#  src/dbxml.c src/dbxml.h \
#  src/dbjson.c src/dbjson.h \
#  src/dbshow.c src/dbshow.h \
#  src/dbaccess.c src/dbaccess.h \
#  src/dbmerge.c src/dbmerge.h \
#  src/common.c src/common.h \
#  src/misc.c src/misc.h \
#  src/cfg.c src/cfg.h \
#  src/ibw.c src/ibw.h \
#  src/fs.c src/fs.h \
#  src/id.c src/id.h \
#  src/traffic.c src/traffic.h

vnstatd_SOURCES = src/vnstatd.c src/vnstatd.h \
  src/ifinfo.c src/ifinfo.h \
  src/dbsql.c src/dbsql.h \
  src/dbaccess.c src/dbaccess.h \
  src/datacache.c src/datacache.h \
  src/common.c src/common.h \
  src/misc.c src/misc.h \
  src/cfg.c src/cfg.h \
  src/ibw.c src/ibw.h \
  src/fs.c src/fs.h \
  src/id.c src/id.h \
  src/daemon.c src/daemon.h

#vnstati_SOURCES = src/vnstati.c src/vnstati.h \
#  src/image.c src/image.h \
#  src/dbsql.c src/dbsql.h \
#  src/dbaccess.c src/dbaccess.h \
#  src/dbmerge.c src/dbmerge.h \
#  src/common.c src/common.h \
#  src/misc.c src/misc.h \
#  src/fs.c src/fs.h \
#  src/id.c src/id.h \
#  src/cfg.c src/cfg.h

if HAVE_CHECK
TESTS = check_vnstat
check_PROGRAMS = check_vnstat
check_vnstat_CFLAGS = @CHECK_CFLAGS@ \
  -DPROCNETDEV=\"testdir/proc/dev\" \
  -DSYSCLASSNET=\"testdir/sysclassnet\" \
  -DCFGFILE=\"$(top_srcdir)/cfg/vnstat.conf\" \
  -I$(top_srcdir)/src
check_vnstat_LDADD = @CHECK_LIBS@

check_vnstat_SOURCES = tests/vnstat_tests.c tests/vnstat_tests.h \
  tests/common_tests.c tests/common_tests.h \
  tests/dbsql_tests.c tests/dbsql_tests.h \
  tests/datacache_tests.c tests/datacache_tests.h \
  tests/config_tests.c tests/config_tests.h \
  tests/fs_tests.c tests/fs_tests.h \
  tests/id_tests.c tests/id_tests.h \
  tests/daemon_tests.c tests/daemon_tests.h

#check_vnstat_SOURCES = tests/vnstat_tests.c tests/vnstat_tests.h \
#  tests/common_tests.c tests/common_tests.h \
#  tests/database_tests.c tests/database_tests.h \
#  tests/dbsql_tests.c tests/dbsql_tests.h \
#  tests/config_tests.c tests/config_tests.h \
#  tests/ifinfo_tests.c tests/ifinfo_tests.h \
#  tests/misc_tests.c tests/misc_tests.h \
#  tests/fs_tests.c tests/fs_tests.h \
#  tests/id_tests.c tests/id_tests.h \
#  tests/daemon_tests.c tests/daemon_tests.h

check_vnstat_SOURCES += src/cfg.c src/cfg.h \
  src/common.c src/common.h \
  src/daemon.c src/daemon.h \
  src/dbsql.c src/dbsql.h \
  src/dbaccess.c src/dbaccess.h \
  src/datacache.c src/datacache.h \
  src/dbjson.c src/dbjson.h \
  src/dbmerge.c src/dbmerge.h \
  src/dbshow.c src/dbshow.h \
  src/dbxml.c src/dbxml.h \
  src/ibw.c src/ibw.h \
  src/ifinfo.c src/ifinfo.h \
  src/misc.c src/misc.h \
  src/fs.c src/fs.h \
  src/id.c src/id.h \
  src/traffic.c src/traffic.h
endif

setdebug:
	CFLAGS='-Wall -Wextra -g' ./configure

profile:
	CFLAGS='-Wall -Wextra -g -fprofile-arcs -ftest-coverage' LDLIBS='-fprofile-arcs -ftest-coverage' ./configure
	make check
	lcov -c -d $(PWD) -o coverage.lcov
	genhtml coverage.lcov --num-spaces 4 -p $(PWD) -o coverage

myclean: maintainer-clean

release: distcheck
	tar zxf $(PACKAGE_NAME)-$(PACKAGE_VERSION).tar.gz
	rm -f $(PACKAGE_NAME)-$(PACKAGE_VERSION).tar.gz
	fakeroot tar zcf $(PACKAGE_NAME)-$(PACKAGE_VERSION).tar.gz $(PACKAGE_NAME)-$(PACKAGE_VERSION)
	rm -fr $(PACKAGE_NAME)-$(PACKAGE_VERSION)
	ls -l $(PACKAGE_NAME)-$(PACKAGE_VERSION).tar.gz

install-data-hook:
	$(DESTDIR)$(bindir)/vnstat$(EXEEXT) --showconfig >vnstat.conf && \
	  $(MKDIR_P) $(DESTDIR)$(sysconfdir) && \
	  mv -f vnstat.conf $(DESTDIR)$(sysconfdir)/vnstat.conf
	@echo
	@echo "============================================================================"
	@echo "  Note: No service file has been installed for starting the daemon"
	@echo "        automatically. Such setup will need to be created manually"
	@echo "        depending on the used environment. See the included documentation."
	@echo "============================================================================"
	@echo

uninstall-hook:
	-rm -f $(DESTDIR)$(sysconfdir)/vnstat.conf
	-rmdir $(DESTDIR)$(sysconfdir)
	@echo
	@echo "=================================================================="
	@echo "  Note: The database directory has been left untouched."
	@echo "        It will need to be removed manually if no longer needed."
	@echo "=================================================================="
	@echo

clean-local:
	-rm -fr coverage *.lcov *.gz test*.log test*.xml src/*.gc* tests/*.gc*

check-local:
	-tail -n 2 test.log
