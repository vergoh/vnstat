CC = gcc
CFLAGS = -O2
OBJS = proc.o db.o misc.o

default: vnstat

vnstat: $(OBJS) vnstat.c vnstat.h
	$(CC) $(CFLAGS) -o vnstat vnstat.c $(OBJS)

64bit: $(OBJS) vnstat.c vnstat.h
	$(CC) $(CFLAGS) -o vnstat vnstat.c $(OBJS) -DBLIMIT

single: $(OBJS) vnstat.c vnstat.h
	$(CC) $(CFLAGS) -o vnstat vnstat.c $(OBJS) -DSINGLE

64bitsingle: $(OBJS) vnstat.c vnstat.h
	$(CC) $(CFLAGS) -o vnstat vnstat.c $(OBJS) -DSINGLE -DBLIMIT

proc.o: proc.c proc.h vnstat.h db.h
	$(CC) $(CFLAGS) -c proc.c

db.o: db.c db.h vnstat.h
	$(CC) -O2 -c db.c

misc.o: misc.c misc.h
	$(CC) $(CFLAGS) -c misc.c

clean:
	rm -f *.o *~ core *.i vnstat
