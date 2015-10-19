SUBDIRS = src tests

vnstatconfdir = $(sysconfdir)
dist_vnstatconf_DATA = cfg/vnstat.conf
dist_man_MANS = man/vnstat.1 man/vnstat.conf.5 man/vnstatd.1 man/vnstati.1

EXTRA_DIST = examples UPGRADE CHANGES FAQ INSTALL_BSD INSTALL_OSX

debug:
	CFLAGS='-Wall -Wextra -g' ./configure
	make

profile:
	CFLAGS='-Wall -Wextra -g -fprofile-arcs -ftest-coverage' LDLIBS='-fprofile-arcs -ftest-coverage' ./configure
	make check
	lcov -c -d $(PWD) -o coverage.lcov
	genhtml coverage.lcov --num-spaces 4 -p $(PWD) -o coverage

myclean: maintainer-clean

clean-local:
	-rm -fr coverage *.lcov *.gz
