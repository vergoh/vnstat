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

	@if [ -d /var/spool/vnstat ]; then echo "Moving old database(s) to new location..."; mv -f /var/spool/vnstat /var/lib/; fi
	mkdir -p /var/lib/vnstat

	@if [ -x /usr/local/bin/vnstat ]; then echo "Removing old binary..."; rm -f /usr/local/bin/vnstat; fi

	@if [ -d /etc/ppp/ip-up.d ]; then echo "Installing ppp/ip-up script"; cp -f pppd/vnstat_ip-up /etc/ppp/ip-up.d/vnstat; fi
	@if [ -d /etc/ppp/ip-down.d ]; then echo "Installing ppp/ip-down script"; cp -f pppd/vnstat_ip-down /etc/ppp/ip-down.d/vnstat; fi

	install -m 755 src/vnstat /usr/bin
	install -m 644 cron/vnstat /etc/cron.d

uninstall:
	@echo "Uninstalling vnStat..."
	@echo
	@echo "Note: this will remove the database directory"
	@echo "including any database located there"
	@echo
	@echo "Press CTRL-C to abort within 10 sec."
	@sleep 10
	rm -fr /var/lib/vnstat
	rm -f /usr/bin/vnstat
	rm -f /etc/cron.d/vnstat
	rm -f /etc/ppp/ip-up.d/vnstat
	rm -f /etc/ppp/ip-down.d/vnstat
