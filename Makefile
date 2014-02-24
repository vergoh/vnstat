# bin, man and cron dirs
BIN = $(DESTDIR)/usr/bin
MAN = $(DESTDIR)/usr/share/man
CRON = $(DESTDIR)/etc/cron.d

vnstat:
	+make -C src vnstat

clean:
	make -C src clean

install:
	@echo "Installing vnStat..."

# move some really old version database(s) if found
	@if [ -d $(DESTDIR)/var/spool/vnstat ]; \
	then echo "Moving old database(s) to new location..."; \
	mv -f $(DESTDIR)/var/spool/vnstat $(DESTDIR)/var/lib/; \
	fi

# remove some really old version binary if found
	@if [ -x $(DESTDIR)/usr/local/bin/vnstat ]; \
	then echo "Removing old binary..."; \
	rm -f $(DESTDIR)/usr/local/bin/vnstat; \
	fi

# install ppp scripts if directory is found
	@if [ -d $(DESTDIR)/etc/ppp/ip-up.d ]; \
	then echo "Installing ppp/ip-up script"; \
	cp -f pppd/vnstat_ip-up $(DESTDIR)/etc/ppp/ip-up.d/vnstat; \
	chmod 755 $(DESTDIR)/etc/ppp/ip-up.d/vnstat; \
	fi
	@if [ -d $(DESTDIR)/etc/ppp/ip-down.d ]; \
	then echo "Installing ppp/ip-down script"; \
	cp -f pppd/vnstat_ip-down $(DESTDIR)/etc/ppp/ip-down.d/vnstat; \
	chmod 755 $(DESTDIR)/etc/ppp/ip-down.d/vnstat; \
	fi

# install default config if such doesn't exist
	@if [ ! -f $(DESTDIR)/etc/vnstat.conf ]; \
	then install -m 644 cfg/vnstat.conf $(DESTDIR)/etc; \
	fi

	install -d $(BIN) $(MAN)/man1 $(CRON) $(DESTDIR)/var/lib/vnstat
	install -s -m 755 src/vnstat $(BIN)

# update man page, gzip it if previous version was done so	
	@if [ -f $(MAN)/man1/vnstat.1.gz ]; \
	then install -m 644 man/vnstat.1 $(MAN)/man1; \
	gzip -f9 $(MAN)/man1/vnstat.1; \
	else install -m 644 man/vnstat.1 $(MAN)/man1; \
	fi

	install -m 644 cron/vnstat $(CRON)

uninstall:
	@echo "Uninstalling vnStat..."
	@echo
	@echo "Note: this will also remove the database directory"
	@echo "including any database located there"
	@echo
	@echo "Press CTRL-C to abort within 10 sec."
	@sleep 10
	rm -fr $(DESTDIR)/var/lib/vnstat
	rm -f $(BIN)/vnstat
	rm -f $(MAN)/man1/vnstat.1*
	rm -f $(CRON)/vnstat
	rm -f $(DESTDIR)/etc/vnstat.conf
	rm -f $(DESTDIR)/etc/ppp/ip-up.d/vnstat
	rm -f $(DESTDIR)/etc/ppp/ip-down.d/vnstat
