SUBDIRS = src tests

debug:
	CFLAGS='-Wall -Wextra -g' ./configure && make

profile: check_vnstat_CFLAGS += -fprofile-arcs -ftest-coverage
profile: check_vnstat_LDLIBS += -fprofile-arcs -ftest-coverage
profile: check
	lcov -c -d $(PWD) -o coverage.lcov
	genhtml coverage.lcov --num-spaces 4 -p . -o coverage

myclean: maintainer-clean

clean-local:
	-rm -fr coverage *.lcov
