vnstat:
	+make -C src vnstat

single:
	+make -C src single
clean:
	make -C src clean

install:
	@echo "Installing vnStat..."
	mkdir -p /var/spool/vnstat
	install -m 755 src/vnstat /usr/local/bin
	install -m 644 cron/vnstat /etc/cron.d

uninstall:
	@echo "Uninstalling vnStat..."
	@echo
	@echo "Note: this will remove the database directory"
	@echo "including any database located there"
	@echo
	@echo "Press CTRL-C to abort within 10 sec."
	@sleep 10
	rm -fr /var/spool/vnstat
	rm -f /usr/local/bin/vnstat
	rm -f /etc/cron.d/vnstat
