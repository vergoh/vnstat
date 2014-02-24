# bin, man and cron dirs
BIN = $(DESTDIR)/usr/bin
MAN = $(DESTDIR)/usr/share/man
CRON = $(DESTDIR)/etc/cron.d

vnstat:
	+make -C src vnstat

64bit:
	+make -C src 64bit

single:
	+make -C src single

64bitsingle:
	+make -C src 64bitsingle

clean:
	make -C src clean

install:
	@echo "Installing vnStat..."

	@if [ -d $(DESTDIR)/var/spool/vnstat ]; then echo "Moving old database(s) to new location..."; mv -f $(DESTDIR)/var/spool/vnstat $(DESTDIR)/var/lib/; fi

	@if [ -x $(DESTDIR)/usr/local/bin/vnstat ]; then echo "Removing old binary..."; rm -f $(DESTDIR)/usr/local/bin/vnstat; fi

	@if [ -d $(DESTDIR)/etc/ppp/ip-up.d ]; then echo "Installing ppp/ip-up script"; cp -f pppd/vnstat_ip-up $(DESTDIR)/etc/ppp/ip-up.d/vnstat; fi
	@if [ -d $(DESTDIR)/etc/ppp/ip-down.d ]; then echo "Installing ppp/ip-down script"; cp -f pppd/vnstat_ip-down $(DESTDIR)/etc/ppp/ip-down.d/vnstat; fi

	install -d $(BIN) $(MAN)/man1 $(CRON) $(DESTDIR)/var/lib/vnstat
	install -m 755 src/vnstat $(BIN)
	install -m 644 man/vnstat.1 $(MAN)/man1
	install -m 644 cron/vnstat $(CRON)

uninstall:
	@echo "Uninstalling vnStat..."
	@echo
	@echo "Note: this will remove the database directory"
	@echo "including any database located there"
	@echo
	@echo "Press CTRL-C to abort within 10 sec."
	@sleep 10
	rm -fr $(DESTDIR)/var/lib/vnstat
	rm -f $(BIN)/vnstat
	rm -f $(MAN)/man1/vnstat.1
	rm -f $(CRON)/vnstat
	rm -f $(DESTDIR)/etc/ppp/ip-up.d/vnstat
	rm -f $(DESTDIR)/etc/ppp/ip-down.d/vnstat
