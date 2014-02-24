CC = gcc
CFLAGS = -O2
OBJS = proc.o db.o misc.o

default: vnstat

vnstat: $(OBJS) vnstat.c vnstat.h
	$(CC) $(CFLAGS) -o vnstat vnstat.c $(OBJS)

single: $(OBJS) vnstat.c vnstat.h
	$(CC) $(CFLAGS) -o vnstat vnstat.c $(OBJS) -DSINGLE

proc.o: proc.c proc.h vnstat.h db.h
	$(CC) $(CFLAGS) -c proc.c

db.o: db.c db.h vnstat.h
	$(CC) -O2 -c db.c

misc.o: misc.c misc.h
	$(CC) $(CFLAGS) -c misc.c

clean:
	rm -f *.o *~ core *.i vnstat

install: vnstat
	@echo "Installing vnStat..."
	mkdir -p /var/spool/vnstat
	install -m 755 vnstat /usr/local/bin
	install -m 644 cron/vnstat /etc/cron.d

uninstall:
	@echo "Uninstalling vnStat..."
	rm -fr /var/spool/vnstat
	rm -f /usr/local/bin/vnstat
	rm -f /etc/cron.d/vnstat

plop:
	make -C plop

plip:
	make -C plop plip
