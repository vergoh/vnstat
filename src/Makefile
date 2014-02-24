CC = gcc
CFLAGS = -O2
OBJS = proc.o db.o misc.o cfg.o

default: vnstat

vnstat: $(OBJS) vnstat.c vnstat.h
	$(CC) $(CFLAGS) -lm -o vnstat vnstat.c $(OBJS)

single: $(OBJS) vnstat.c vnstat.h
	$(CC) $(CFLAGS) -lm -o vnstat vnstat.c $(OBJS) -DSINGLE

proc.o: proc.c proc.h vnstat.h db.h misc.h cfg.h
	$(CC) $(CFLAGS) -c proc.c

db.o: db.c db.h proc.h vnstat.h
	$(CC) $(CFLAGS) -c db.c

misc.o: misc.c misc.h
	$(CC) $(CFLAGS) -c misc.c

cfg.o: cfg.c cfg.h
	$(CC) $(CFLAGS) -c cfg.c

clean:
	rm -f *.o *~ core *.i vnstat
