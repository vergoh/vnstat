CC = gcc
CFLAGS = -O2

default: multiuser

singleuser: vnstat.c
	@echo "Compiling singleuser version..."
	$(CC) $(CFLAGS) -DMULTIUSER=0 vnstat.c -o vnstat

multiuser: vnstat.c
	@echo "Compiling multiuser version..."
	$(CC) $(CFLAGS) -DMULTIUSER=1 vnstat.c -o vnstat

install:
	@echo "Installing vnStat..."
	mkdir /var/spool/vnstat
	install -m 755 vnstat /usr/local/bin
	install -m 644 cron/vnstat /etc/cron.d

uninstall:
	@echo "Uninstalling vnStat..."
	rm -fr /var/spool/vnstat
	rm -f /usr/local/bin/vnstat
	rm -f /etc/cron.d/vnstat

clean:
	rm -f *.o *~ core *.i vnstat
