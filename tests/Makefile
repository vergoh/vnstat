ifeq "$(origin CC)" "default"
CC = gcc
endif
CFLAGS ?= -Wall -Wextra -g
CFLAGS += -DPROCNETDEV=\"testdir/proc/dev\" -DSYSCLASSNET=\"testdir/sysclassnet\"
LDLIBS = -lm -lsqlite3 -lcheck -lrt -lpthread
OBJS = vnstat_tests.o common_tests.o dbsql_tests.o database_tests.o config_tests.o ifinfo_tests.o misc_tests.o daemon_tests.o
OBJS += common.o ifinfo.o dbshow.o dbaccess.o dbcache.o dbsql.o cfg.o ibw.o misc.o daemon.o

vpath %.c ../src
vpath %.h ../src
CFLAGS += -I../src

default: runtests

runtests: vnstat_tests
	./vnstat_tests

profile: CFLAGS += -fprofile-arcs -ftest-coverage
profile: LDLIBS += -fprofile-arcs -ftest-coverage
profile: runtests
	lcov -c -d $(PWD) -o coverage.lcov
	genhtml coverage.lcov --num-spaces 4 -p $(shell dirname $(PWD)) -o coverage

vnstat_tests: $(OBJS)
	$(CC) $(LDFLAGS) $(OBJS) $(LDLIBS) -o vnstat_tests

vnstat_tests.o: vnstat_tests.c vnstat_tests.h common_tests.h database_tests.h common.h
common_tests.o: common_tests.c common_tests.h vnstat_tests.h common.h dbaccess.h cfg.h
dbsql_tests.o: dbsql_tests.c dbsql_tests.h vnstat_tests.h common.h dbsql.h cfg.h
database_tests.o: database_tests.c database_tests.h vnstat_tests.h common.h ifinfo.h dbaccess.h dbcache.h dbshow.h cfg.h ibw.h
config_tests.o: config_tests.c config_tests.h vnstat_tests.h common.h cfg.h ibw.h
ifinfo_tests.o: ifinfo_tests.c ifinfo_tests.h vnstat_tests.h common.h ifinfo.h dbaccess.h dbsql.h misc.h cfg.h ibw.h
misc_tests.o: misc_tests.c misc_tests.h vnstat_tests.h common.h misc.h
daemon_tests.o: daemon_tests.c daemon_tests.h common.h dbaccess.h dbcache.h cfg.h daemon.h

common.o: common.c common.h
ifinfo.o: ifinfo.c ifinfo.h common.h dbaccess.h misc.h cfg.h ibw.h
dbsql.o: dbsql.c dbsql.h common.h misc.h
dbaccess.o: dbaccess.c dbaccess.h common.h
dbcache.o: dbcache.c dbcache.h dbaccess.h common.h ifinfo.h cfg.h ibw.h
dbshow.o: dbshow.c dbshow.h misc.h common.h
cfg.o: cfg.c cfg.h common.h
ibw.o: ibw.c ibw.h cfg.h ifinfo.h common.h
misc.o: misc.c misc.h common.h
daemon.o: daemon.c daemon.h common.h ifinfo.h dbaccess.h dbcache.h misc.h cfg.h ibw.h

clean:
	rm -fr *.o *~ core *.i *.gc* test.log test.xml *.lcov coverage vnstat_tests testdir
