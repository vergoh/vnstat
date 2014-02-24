# bin and man dirs for Linux
BIN = $(DESTDIR)/usr/bin
SBIN = $(DESTDIR)/usr/sbin
MAN = $(DESTDIR)/usr/share/man

# bin and man dirs for *BSD
BIN_BSD = $(DESTDIR)/usr/local/bin
SBIN_BSD = $(DESTDIR)/usr/local/sbin
MAN_BSD = $(DESTDIR)/usr/local/man

default: vnstat

vnstat:
	+make -C src

all:
	+make -C src all

clean:
	make -C src clean

install:
	@echo "Installing vnStat..."

# check that system is really Linux
	@if [ `uname` != "Linux" ]; \
	then echo "This isn't a Linux system. Maybe 'make bsdinstall' is what you need?"; \
	false; \
	fi

# check that there's something to install
	@if [ ! -f "src/vnstat" ] || [ ! -f "src/vnstatd" ]; \
	then echo "Nothing to install, run make first."; \
	false; \
	fi

# move some really old version database(s) if found
	@if [ -d "$(DESTDIR)/var/spool/vnstat" ]; \
	then echo "Moving old database(s) to new location..."; \
	mv -f $(DESTDIR)/var/spool/vnstat $(DESTDIR)/var/lib/; \
	fi

# remove some really old version binary if found
	@if [ -x "$(DESTDIR)/usr/local/bin/vnstat" ]; \
	then echo "Removing old binary..."; \
	rm -f $(DESTDIR)/usr/local/bin/vnstat; \
	fi

# install default config if such doesn't exist
	@if [ ! -f "$(DESTDIR)/etc/vnstat.conf" ]; \
	then echo "Installing config to $(DESTDIR)/etc/vnstat.conf"; \
	install -D -m 644 cfg/vnstat.conf $(DESTDIR)/etc/vnstat.conf; \
	fi

# install everything else
	install -d -m 755 $(BIN) $(SBIN) $(MAN)/man1 $(MAN)/man5 $(DESTDIR)/var/lib/vnstat
	install -s -m 755 src/vnstat $(BIN)
	install -s -m 755 src/vnstatd $(SBIN)
	@if [ -f "src/vnstati" ]; \
	then echo install -s -m 755 src/vnstati $(BIN); \
	install -s -m 755 src/vnstati $(BIN); \
	fi

# update man pages, gzip it if previous version was done so	
	install -m 644 man/vnstat.1 $(MAN)/man1
	install -m 644 man/vnstatd.1 $(MAN)/man1
	install -m 644 man/vnstat.conf.5 $(MAN)/man5
	@if [ -f "src/vnstati" ]; \
	then echo install -m 644 man/vnstati.1 $(MAN)/man1; \
	install -m 644 man/vnstati.1 $(MAN)/man1; \
	fi
	
	@if [ -f $(MAN)/man1/vnstat.1.gz ]; \
	then gzip -f9 $(MAN)/man1/vnstat.1; \
	gzip -f9 $(MAN)/man1/vnstatd.1; \
	gzip -f9 $(MAN)/man5/vnstat.conf.5; \
	if [ -f "src/vnstati" ]; \
	then gzip -f9 $(MAN)/man1/vnstati.1; \
	fi; \
	fi

# remove vnstat.conf.1 is such exists in the wrong place
	@if [ -f $(MAN)/man1/vnstat.conf.1.gz ]; \
	then rm -f $(MAN)/man1/vnstat.conf.1.gz; \
	fi
	@if [ -f $(MAN)/man1/vnstat.conf.1 ]; \
	then rm -f $(MAN)/man1/vnstat.conf.1; \
	fi

	@echo " "
	@echo "No startup script or cron entry has been installed. See the"
	@echo "INSTALL document for instructions on how to enable vnStat."

uninstall:
	@echo "Uninstalling vnStat..."
	@echo
	@echo "Note: this will also remove the database directory"
	@echo "including any database located there."
	@echo
	@echo "Press CTRL-C within 10 seconds to abort."
	@sleep 10
	rm -fr $(DESTDIR)/var/lib/vnstat
	rm -f $(BIN)/vnstat
	rm -f $(BIN)/vnstati
	rm -f $(SBIN)/vnstatd
	rm -f $(MAN)/man1/vnstat*
	rm -f $(MAN)/man5/vnstat*
	rm -f $(DESTDIR)/etc/cron.d/vnstat
	rm -f $(DESTDIR)/etc/vnstat.conf
	rm -f $(DESTDIR)/etc/ppp/ip-up.d/vnstat
	rm -f $(DESTDIR)/etc/ppp/ip-down.d/vnstat

bsdinstall:
	@echo "Installing vnStat (BSD)..."

# check that system isn't Linux
	@if [ `uname` = "Linux" ]; \
	then echo "This is a Linux system. You shouldn't be using 'bsdinstall'"; \
	false; \
	fi

# check that there's something to install
	@if [ ! -f "src/vnstat" ] || [ ! -f "src/vnstatd" ]; \
	then echo "Nothing to install, run make first."; \
	false; \
	fi

# install binaries
	install -d -m 755 $(DESTDIR)/var/db/vnstat
	install -s -m 755 src/vnstat $(BIN_BSD)
	install -s -m 755 src/vnstatd $(SBIN_BSD)

	@if [ -f "src/vnstati" ]; \
	then echo install -s -m 755 src/vnstati $(BIN_BSD); \
	install -s -m 755 src/vnstati $(BIN_BSD); \
	fi

# install default config if such doesn't exist
	@if [ ! -f $(DESTDIR)/etc/vnstat.conf ]; \
	then echo "Installing config to $(DESTDIR)/etc/vnstat.conf"; \
	install -d -m 755 $(DESTDIR)/etc; \
	install -m 644 cfg/vnstat.conf $(DESTDIR)/etc/vnstat.conf; \
	sed -e 's/lib/db/g' $(DESTDIR)/etc/vnstat.conf >$(DESTDIR)/etc/vnstat.conf.bsd; \
	mv -f $(DESTDIR)/etc/vnstat.conf.bsd $(DESTDIR)/etc/vnstat.conf; \
	fi

# update man page	
	install -m 644 man/vnstat.1 $(MAN_BSD)/man1
	install -m 644 man/vnstatd.1 $(MAN_BSD)/man1
	install -m 644 man/vnstat.conf.5 $(MAN_BSD)/man5
	gzip -f9 $(MAN_BSD)/man1/vnstat.1
	gzip -f9 $(MAN_BSD)/man1/vnstatd.1
	gzip -f9 $(MAN_BSD)/man5/vnstat.conf.5
	@if [ -f "src/vnstati" ]; \
	then echo install -m 644 man/vnstati.1 $(MAN_BSD)/man1; \
	install -m 644 man/vnstati.1 $(MAN_BSD)/man1; \
	echo gzip -f9 $(MAN_BSD)/man1/vnstati.1; \
	gzip -f9 $(MAN_BSD)/man1/vnstati.1; \
	fi

# remove vnstat.conf.1 is such exists in the wrong place
	@if [ -f $(MAN_BSD)/man1/vnstat.conf.1.gz ]; \
	then rm -f $(MAN_BSD)/man1/vnstat.conf.1.gz; \
	fi
	@if [ -f $(MAN_BSD)/man1/vnstat.conf.1 ]; \
	then rm -f $(MAN_BSD)/man1/vnstat.conf.1; \
	fi

	@echo " "
	@echo "No startup script or cron entry has been installed. See the"
	@echo "INSTALL_BSD document for instructions on how to enable vnStat."

bsduninstall:
	@echo "Uninstalling vnStat (BSD)..."
	@echo
	@echo "Note: this will also remove the database directory"
	@echo "including any database located there."
	@echo
	@echo "Press CTRL-C within 10 seconds to abort."
	@sleep 10
	rm -fr $(DESTDIR)/var/db/vnstat
	rm -f $(BIN_BSD)/vnstat
	rm -f $(BIN_BSD)/vnstati
	rm -f $(SBIN_BSD)/vnstatd
	rm -f $(MAN_BSD)/man1/vnstat*
	rm -f $(MAN_BSD)/man5/vnstat*
	rm -f $(DESTDIR)/etc/vnstat.conf
	@echo "A possible cron entry needs to be removed manually if such exists."
