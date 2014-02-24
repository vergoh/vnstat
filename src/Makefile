CC = gcc
CFLAGS = -O2 -Wall
OBJS = proc.o db.o misc.o
OBJS64 = proc64.o db.o misc.o

default: vnstat

vnstat: $(OBJS) vnstat.c vnstat.h
	$(CC) $(CFLAGS) -o vnstat vnstat.c $(OBJS)

64bit: $(OBJS64) vnstat.c vnstat.h
	$(CC) $(CFLAGS) -o vnstat vnstat.c $(OBJS64) -DBLIMIT

single: $(OBJS) vnstat.c vnstat.h
	$(CC) $(CFLAGS) -o vnstat vnstat.c $(OBJS) -DSINGLE

64bitsingle: $(OBJS64) vnstat.c vnstat.h
	$(CC) $(CFLAGS) -o vnstat vnstat.c $(OBJS64) -DSINGLE -DBLIMIT

proc.o: proc.c proc.h vnstat.h db.h
	$(CC) $(CFLAGS) -c proc.c

proc64.o: proc.c proc.h vnstat.h db.h
	$(CC) $(CFLAGS) -c proc.c -o proc64.o -DBLIMIT

db.o: db.c db.h vnstat.h
	$(CC) -O2 -c db.c

misc.o: misc.c misc.h
	$(CC) $(CFLAGS) -c misc.c

clean:
	rm -f *.o *~ core *.i vnstat
