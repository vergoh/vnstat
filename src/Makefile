CC = gcc
CFLAGS = -O2
LDLIBS = -lm
OBJS = vnstat.o ifinfo.o dbxml.o dbshow.o dbaccess.o dbmerge.o common.o misc.o cfg.o traffic.o
DOBJS = vnstatd.o ifinfo.o dbaccess.o dbcache.o common.o misc.o cfg.o
IOBJS = vnstati.o image.o dbaccess.o dbmerge.o common.o misc.o cfg.o

default: vnstat vnstatd

all: vnstat vnstatd vnstati

vnstat: $(OBJS)
	$(CC) $(LDFLAGS) $(OBJS) $(LDLIBS) -o vnstat
vnstatd: $(DOBJS)
	$(CC) $(LDFLAGS) $(DOBJS) $(LDLIBS) -o vnstatd
vnstati: $(IOBJS)
	$(CC) $(LDFLAGS) $(IOBJS) $(LDLIBS) -lgd -o vnstati

vnstat.o: vnstat.c vnstat.h common.h ifinfo.h traffic.h dbxml.h dbshow.h dbaccess.h dbmerge.h misc.h cfg.h
vnstatd.o: vnstatd.c vnstatd.h common.h ifinfo.h dbaccess.h dbcache.h misc.h cfg.h
vnstati.o: vnstati.c vnstati.h common.h image.h cfg.h dbaccess.h dbmerge.h

ifinfo.o: ifinfo.c ifinfo.h common.h dbaccess.h misc.h cfg.h
traffic.o: traffic.c traffic.h common.h ifinfo.h misc.h
dbxml.o: dbxml.c dbxml.h common.h
dbshow.o: dbshow.c dbshow.h misc.h common.h
dbaccess.o: dbaccess.c dbaccess.h common.h
dbmerge.o: dbmerge.c dbmerge.h dbaccess.h common.h
dbcache.o: dbcache.c dbcache.h dbaccess.h common.h ifinfo.h
common.o: common.c common.h
misc.o: misc.c misc.h common.h
cfg.o: cfg.c cfg.h common.h
image.o: image.c image.h vnstati.h common.h misc.h

clean:
	rm -f *.o *~ core *.i vnstat vnstatd vnstati
