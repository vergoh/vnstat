CC = gcc
CFLAGS = -O2
OBJS = ifinfo.o db.o dbaccess.o misc.o cfg.o

default: vnstat

vnstat: $(OBJS) vnstat.c vnstat.h
	$(CC) $(CFLAGS) -lm -o vnstat vnstat.c $(OBJS)

ifinfo.o: ifinfo.c ifinfo.h vnstat.h db.h misc.h cfg.h
	$(CC) $(CFLAGS) -c ifinfo.c

db.o: db.c db.h dbaccess.h ifinfo.h misc.h vnstat.h
	$(CC) $(CFLAGS) -c db.c

dbaccess.o: dbaccess.c dbaccess.h vnstat.h
	$(CC) $(CFLAGS) -c dbaccess.c

misc.o: misc.c misc.h vnstat.h
	$(CC) $(CFLAGS) -c misc.c

cfg.o: cfg.c cfg.h
	$(CC) $(CFLAGS) -c cfg.c

clean:
	rm -f *.o *~ core *.i vnstat
