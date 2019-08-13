#include "common.h"
#include "ifinfo.h"
#include "iflist.h"
#include "dbsql.h"
#include "dbaccess.h"
#include "datacache.h"
#include "misc.h"
#include "cfg.h"
#include "ibw.h"
#include "fs.h"
#include "id.h"
#include "daemon.h"

void daemonize(void)
{
	int i;
	char str[10];

	i = (int)fork();

	if (i < 0) { /* fork error */
		perror("Error: fork");
		exit(EXIT_FAILURE);
	}
	if (i > 0) { /* parent exits */
		exit(EXIT_SUCCESS);
	}
	/* child (daemon) continues */

	setsid(); /* obtain a new process group */

	if (!verifylogaccess()) {
		printf("Error: Unable to use logfile. Exiting.\n");
		exit(EXIT_FAILURE);
	}

	/* lock / pid file */
	pidfile = open(cfg.pidfile, O_RDWR | O_CREAT, 0644);
	if (pidfile < 0) {
		perror("Error: pidfile");
		snprintf(errorstring, 1024, "opening pidfile \"%s\" failed (%s), exiting.", cfg.pidfile, strerror(errno));
		printe(PT_Error);
		exit(EXIT_FAILURE); /* can't open */
	}
	if (lockf(pidfile, F_TLOCK, 0) < 0) {
		perror("Error: pidfile lock");
		snprintf(errorstring, 1024, "pidfile \"%s\" lock failed (%s), exiting.", cfg.pidfile, strerror(errno));
		printe(PT_Error);
		exit(EXIT_FAILURE); /* can't lock */
	}

	/* close all descriptors except lock file */
	for (i = getdtablesize(); i >= 0; --i) {
		if (i != pidfile) {
			close(i);
		}
	}

	/* redirect standard i/o to null */
	i = open("/dev/null", O_RDWR); /* stdin */

	if (i < 0) {
		perror("Error: open() /dev/null");
		snprintf(errorstring, 1024, "open() /dev/null failed, exiting.");
		printe(PT_Error);
		exit(EXIT_FAILURE);
	}

	/* stdout */
	if (dup(i) < 0) {
		perror("Error: dup(stdout)");
		snprintf(errorstring, 1024, "dup(stdout) failed, exiting.");
		printe(PT_Error);
		exit(EXIT_FAILURE);
	}
	/* stderr */
	if (dup(i) < 0) {
		perror("Error: dup(stderr)");
		snprintf(errorstring, 1024, "dup(stderr) failed, exiting.");
		printe(PT_Error);
		exit(EXIT_FAILURE);
	}

	close(i);

	umask(027); /* set newly created file permissions */

	/* change running directory */
	if (chdir("/") < 0) {
		perror("Error: chdir(/)");
		snprintf(errorstring, 1024, "directory change to / failed, exiting.");
		printe(PT_Error);
		exit(EXIT_FAILURE);
	}

	/* first instance continues */
	snprintf(str, 10, "%d\n", (int)getpid());

	/* record pid to pidfile */
	if (write(pidfile, str, strlen(str)) < 0) {
		perror("Error: write(pidfile)");
		snprintf(errorstring, 1024, "writing to pidfile \"%s\" failed (%s), exiting.", cfg.pidfile, strerror(errno));
		printe(PT_Error);
		exit(EXIT_FAILURE);
	}

	signal(SIGCHLD, SIG_IGN); /* ignore child */
	signal(SIGTSTP, SIG_IGN); /* ignore tty signals */
	signal(SIGTTOU, SIG_IGN);
	signal(SIGTTIN, SIG_IGN);
}

unsigned int addinterfaces(DSTATE *s)
{
	iflist *ifl = NULL, *ifl_iterator = NULL;
	unsigned int count = 0;
	uint32_t bwlimit = 0;

	timeused_debug(__func__, 1);

	/* get list of currently visible interfaces */
	if (getiflist(&ifl, 0, 1) == 0) {
		iflistfree(&ifl);
		return 0;
	}

	if (ifl == NULL) {
		return 0;
	}

	if (debug) {
		printf("Interface list:");
		ifl_iterator = ifl;
		while (ifl_iterator != NULL) {
			printf(" \"%s\"", ifl_iterator->interface);
			ifl_iterator = ifl_iterator->next;
		}
		printf("\n");
	}

	ifl_iterator = ifl;
	while (ifl_iterator != NULL) {
		if (debug)
			printf("Processing: \"%s\"\n", ifl_iterator->interface);

		/* skip already known interfaces */
		if (db_getinterfacecountbyname(ifl_iterator->interface)) {
			if (debug)
				printf("already known\n");
			ifl_iterator = ifl_iterator->next;
			continue;
		}

		/* create database for interface */
		if (!db_addinterface(ifl_iterator->interface)) {
			if (debug)
				printf("add failed, skip\n");
			ifl_iterator = ifl_iterator->next;
			continue;
		}

		if (!getifinfo(ifl_iterator->interface)) {
			if (debug)
				printf("getifinfo failed, skip\n");
			/* remove empty entry from database since the interface can't provide data */
			db_removeinterface(ifl_iterator->interface);
			ifl_iterator = ifl_iterator->next;
			continue;
		}

		db_setcounters(ifl_iterator->interface, ifinfo.rx, ifinfo.tx);

		count++;
		ibwget(ifl_iterator->interface, &bwlimit);
		if (bwlimit > 0) {
			snprintf(errorstring, 1024, "Interface \"%s\" added with %" PRIu32 " Mbit bandwidth limit.", ifl_iterator->interface, bwlimit);
		} else {
			snprintf(errorstring, 1024, "Interface \"%s\" added. Warning: no bandwidth limit has been set.", ifl_iterator->interface);
		}
		printe(PT_Infoless);
		if (s->running) {
			datacache_add(&s->dcache, ifl_iterator->interface, 1);
		}
		ifl_iterator = ifl_iterator->next;
	}

	if (count && !s->running) {
		if (count == 1) {
			printf("-> %u new interface found.\n", count);
		} else {
			printf("-> %u new interfaces found.\n", count);
		}

		printf("Limits can be modified using the configuration file. See \"man vnstat.conf\".\n");
		printf("Unwanted interfaces can be removed from monitoring with \"vnstat --remove\".\n");
	}

	iflistfree(&ifl);
	timeused_debug(__func__, 0);
	return count;
}

void detectboot(DSTATE *s)
{
	char buffer[32];
	char *btime_buffer;
	uint64_t current_btime, db_btime;

	current_btime = getbtime();
	btime_buffer = db_getinfo("btime");

	if (current_btime == 0) {
		return;
	} else if (strlen(btime_buffer) == 0) {
		snprintf(buffer, 32, "%" PRIu64 "", current_btime);
		db_setinfo("btime", buffer, 1);
		return;
	}
	db_btime = strtoull(btime_buffer, (char **)NULL, 0);

	if (db_btime < (current_btime - (uint32_t)cfg.bvar)) {
		s->bootdetected = 1;
		if (debug)
			printf("System has been booted, %" PRIu64 " < %" PRIu64 " - %d\n", db_btime, current_btime, cfg.bvar);
	}

	snprintf(buffer, 32, "%" PRIu64 "", current_btime);
	db_setinfo("btime", buffer, 1);
}

void debugtimestamp(void)
{
	time_t now;
	char timestamp[22];

	now = time(NULL);
	strftime(timestamp, 22, DATETIMEFORMAT, localtime(&now));
	printf("%s\n", timestamp);
}

void initdstate(DSTATE *s)
{
	db = NULL;
	noexit = 1;		   /* disable exits in functions */
	debug = 0;		   /* debug disabled by default */
	disableprints = 0; /* let prints be visible */
	s->rundaemon = 0;  /* daemon disabled by default */

	s->running = 0;
	s->dbsaved = 1;
	s->showhelp = 1;
	s->sync = 0;
	s->forcesave = 0;
	s->noadd = 0;
	s->alwaysadd = 0;
	s->iflisthash = 0;
	s->cfgfile[0] = '\0';
	s->user[0] = '\0';
	s->group[0] = '\0';
	s->prevdbupdate = 0;
	s->prevdbsave = 0;
	s->dbifcount = 0;
	s->dodbsave = 0;
	s->bootdetected = 0;
	s->cleanuphour = getcurrenthour();
	s->dbretrycount = 0;
	s->dcache = NULL;
	s->prevwaldbcheckpoint = time(NULL);
}

void preparedatabase(DSTATE *s)
{
	s->dbifcount = db_getinterfacecount();

	if (s->dbifcount > 0 && !s->alwaysadd) {
		s->dbifcount = 0;
		return;
	}

	if (debug) {
		printf("db if count: %" PRIu64 "\n", s->dbifcount);
	}

	if (s->noadd) {
		printf("No interfaces found in database, exiting.\n");
		exit(EXIT_FAILURE);
	}

	if (!spacecheck(cfg.dbdir)) {
		printf("Error: Not enough free diskspace available, exiting.\n");
		exit(EXIT_FAILURE);
	}

	if (s->dbifcount == 0) {
		if (importlegacydbs(s) && !s->alwaysadd) {
			s->dbifcount = 0;
			return;
		}
	}

	if (s->dbifcount == 0) {
		printf("No interfaces found in database, adding available interfaces...\n");
	}

	if (!addinterfaces(s) && s->dbifcount == 0) {
		printf("Nothing to do, exiting.\n");
		exit(EXIT_FAILURE);
	}

	/* set counter back to zero so that dbs will be cached later */
	s->dbifcount = 0;
}

unsigned int importlegacydbs(DSTATE *s)
{
	DIR *dir;
	struct dirent *di;
	unsigned int importcount = 0;

	if ((dir = opendir(cfg.dbdir)) == NULL) {
		printf("Error: Unable to open database directory \"%s\": %s\n", cfg.dbdir, strerror(errno));
		printf("Make sure it exists and is at least read enabled for current user.\n");
		printf("Exiting...\n");
		exit(EXIT_FAILURE);
	}

	s->dbifcount = 0;
	while ((di = readdir(dir))) {
		if ((di->d_name[0] != '.') && (strncmp(di->d_name, DATABASEFILE, strlen(DATABASEFILE)) != 0)) {
			/* ignore already known interfaces */
			if (db_getinterfacecountbyname(di->d_name)) {
				continue;
			}
			if (importlegacydb(di->d_name, cfg.dbdir)) {
				importcount++;
			}
		}
	}
	closedir(dir);

	s->dbifcount += importcount;
	return importcount;
}

void setsignaltraps(void)
{
	intsignal = 0;
	if (signal(SIGINT, sighandler) == SIG_ERR) {
		perror("Error: signal SIGINT");
		exit(EXIT_FAILURE);
	}
	if (signal(SIGHUP, sighandler) == SIG_ERR) {
		perror("Error: signal SIGHUP");
		exit(EXIT_FAILURE);
	}
	if (signal(SIGTERM, sighandler) == SIG_ERR) {
		perror("Error: signal SIGTERM");
		exit(EXIT_FAILURE);
	}
}

void filldatabaselist(DSTATE *s)
{
	iflist *dbifl = NULL, *dbifl_iterator = NULL;

	timeused_debug(__func__, 1);

	if (db_getiflist(&dbifl) < 0) {
		errorexitdaemon(s, 1);
	}

	dbifl_iterator = dbifl;

	while (dbifl_iterator != NULL) {
		if (debug) {
			printf("\nProcessing interface \"%s\"...\n", dbifl_iterator->interface);
		}
		if (!datacache_add(&s->dcache, dbifl_iterator->interface, s->sync)) {
			snprintf(errorstring, 1024, "Cache memory allocation failed (%s), exiting.", strerror(errno));
			printe(PT_Error);
			errorexitdaemon(s, 1);
		}
		s->dbifcount++;
		dbifl_iterator = dbifl_iterator->next;
	}

	iflistfree(&dbifl);
	s->sync = 0;

	/* disable update interval check for one loop if database list was refreshed */
	/* otherwise increase default update interval since there's nothing else to do */
	if (s->dbifcount) {
		s->updateinterval = 0;
		intsignal = 42;
		s->prevdbsave = s->current;
		/* list monitored interfaces to log */
		datacache_status(&s->dcache);
	} else {
		s->updateinterval = 120;
	}
	timeused_debug(__func__, 0);
}

void adjustsaveinterval(DSTATE *s)
{
	/* modify active save interval if all interfaces are unavailable */
	if (datacache_activecount(&s->dcache) > 0) {
		s->saveinterval = cfg.saveinterval * 60;
	} else {
		s->saveinterval = cfg.offsaveinterval * 60;
	}
}

void checkdbsaveneed(DSTATE *s)
{
	if ((s->current - s->prevdbsave) >= (s->saveinterval) || s->forcesave) {
		s->dodbsave = 1;
		s->forcesave = 0;
		s->prevdbsave = s->current - (s->current % s->saveinterval);
	} else {
		s->dodbsave = 0;
	}
}

void processdatacache(DSTATE *s)
{
	datacache *iterator = s->dcache;

	timeused_debug(__func__, 1);

	while (iterator != NULL) {

		if (debug) {
			printf("dc: processing %s (%d)...\n", iterator->interface, s->dodbsave);
		}

		if (!iterator->filled) {
			if (!initcachevalues(s, &iterator)) {
				iterator = iterator->next;
				continue;
			}
			s->iflisthash = 0;
		}

		if (iterator->active) {
			if (!getifinfo(iterator->interface)) {
				/* disable interface since we can't access its data */
				iterator->active = 0;
				snprintf(errorstring, 1024, "Interface \"%s\" not available, disabling.", iterator->interface);
				printe(PT_Info);
			} else {
				if (!processifinfo(s, &iterator)) {
					iterator = iterator->next;
					continue;
				}
			}
		} else {
			if (debug)
				printf("dc: interface is disabled\n");
		}

		iterator = iterator->next;
	}

	if (s->bootdetected) {
		s->bootdetected = 0;
	}
	timeused_debug(__func__, 0);

	if (s->dodbsave) {
		flushcachetodisk(s);
		cleanremovedinterfaces(s);
		if (s->cleanuphour != getcurrenthour()) {
			db_removeoldentries();
			s->cleanuphour = getcurrenthour();
		}
		s->dodbsave = 0;
	}
}

int initcachevalues(DSTATE *s, datacache **dc)
{
	interfaceinfo info;

	if (!db_getinterfaceinfo((*dc)->interface, &info)) {
		return 0;
	}

	if (s->bootdetected) {
		(*dc)->currx = 0;
		(*dc)->curtx = 0;
	} else {
		(*dc)->currx = info.rxcounter;
		(*dc)->curtx = info.txcounter;
	}
	(*dc)->updated = info.updated;
	(*dc)->filled = 1;

	return 1;
}

int processifinfo(DSTATE *s, datacache **dc)
{
	uint64_t rxchange, txchange;
	uint64_t maxtransfer;
	uint32_t maxbw;
	time_t interval;
	short detected64bit = 0;

	if ((*dc)->syncneeded) { /* if --sync was used during startup */
		(*dc)->currx = ifinfo.rx;
		(*dc)->curtx = ifinfo.tx;
		(*dc)->syncneeded = 0;
		return 1;
	}

	if ((*dc)->updated > ifinfo.timestamp) {
		/* skip update if previous update is less than a day in the future */
		/* otherwise exit with error message since the clock is problably messed */
		if ((*dc)->updated > (ifinfo.timestamp + 86400)) {
			snprintf(errorstring, 1024, "Interface \"%s\" has previous update date too much in the future, exiting. (%u / %u)", (*dc)->interface, (unsigned int)(*dc)->updated, (unsigned int)ifinfo.timestamp);
			printe(PT_Error);
			errorexitdaemon(s, 1);
		}
		return 0;
	}

	interval = ifinfo.timestamp - (*dc)->updated;
	/* maximum configurable update interval is 5 minutes, limit here is set to 6 minutes (360 seconds) */
	/* in order to be on the safe side and avoid discarding data in case there's some random extra delay */
	if ((interval >= 1) && (interval <= 360)) {

		if ((*dc)->currx > MAX32 || (*dc)->curtx > MAX32 || ifinfo.rx > MAX32 || ifinfo.tx > MAX32) {
			ifinfo.is64bit = 1;
			detected64bit = 1;
		}

		rxchange = countercalc(&(*dc)->currx, &ifinfo.rx, ifinfo.is64bit);
		txchange = countercalc(&(*dc)->curtx, &ifinfo.tx, ifinfo.is64bit);

		/* workaround for interface drivers using only 32-bit range with 64-bit interface counters, */
		/* active only when automatic detection is enabled and all values are within 32-bit range */
		if (cfg.is64bit == -2 || !detected64bit) {
			if ((rxchange / (uint64_t)interval / 1024 / 1024 * 8) > BWMAX || (txchange / (uint64_t)interval / 1024 / 1024 * 8) > BWMAX) {
				ifinfo.is64bit = 0;
				rxchange = countercalc(&(*dc)->currx, &ifinfo.rx, 0);
				txchange = countercalc(&(*dc)->curtx, &ifinfo.tx, 0);
			}
		}

		/* get bandwidth limit for current interface */
		ibwget((*dc)->interface, &maxbw);

		if (maxbw > 0) {

			/* calculate maximum possible transfer since last update based on set maximum rate */
			/* and add 2% in order to be on the safe side */
			maxtransfer = (uint64_t)(ceilf((maxbw / (float)8) * interval * (float)1.02)) * 1024 * 1024;

			if (debug)
				printf("interval: %" PRIu64 "  maxbw: %" PRIu32 "  maxrate: %" PRIu64 "  rxc: %" PRIu64 "  txc: %" PRIu64 "\n", (uint64_t)interval, maxbw, maxtransfer, rxchange, txchange);

			/* sync counters if traffic is greater than set maximum */
			if ((rxchange > maxtransfer) || (txchange > maxtransfer)) {
				snprintf(errorstring, 1024, "Traffic rate for \"%s\" higher than set maximum %" PRIu32 " Mbit (%" PRIu64 "s->%" PRIu64 ", r%" PRIu64 " t%" PRIu64 ", 64bit:%d), syncing.", (*dc)->interface, maxbw, (uint64_t)interval, maxtransfer, rxchange, txchange, ifinfo.is64bit);
				printe(PT_Info);
				rxchange = txchange = 0;
			}
		}

		if (rxchange || txchange || cfg.trafficlessentries) {
			xferlog_add(&(*dc)->log, (*dc)->updated - ((*dc)->updated % 300), rxchange, txchange);
		}
	}
	(*dc)->currx = ifinfo.rx;
	(*dc)->curtx = ifinfo.tx;
	(*dc)->updated = ifinfo.timestamp;

	return 1;
}

void flushcachetodisk(DSTATE *s)
{
	int ret;
	double used_secs = 0.0;
	uint32_t logcount = 0;
	datacache *iterator = s->dcache;
	xferlog *logiterator;
	interfaceinfo info;

	timeused(__func__, 1);

	if (!db_begintransaction()) {
		handledatabaseerror(s);
		return;
	}

	db_errcode = 0;
	while (iterator != NULL) {
		/* ignore interface no longer in database */
		if (!db_getinterfacecountbyname(iterator->interface)) {
			if (db_errcode) {
				handledatabaseerror(s);
				break;
			} else {
				iterator = iterator->next;
				continue;
			}
		}

		/* flush interface specific log to database */
		logcount = 0;
		logiterator = iterator->log;
		while (logiterator != NULL) {
			if (!db_addtraffic_dated(iterator->interface, logiterator->rx, logiterator->tx, (uint64_t)logiterator->timestamp)) {
				handledatabaseerror(s);
				break;
			}
			logiterator = logiterator->next;
			logcount++;
		}
		if (db_errcode) {
			break;
		}

		/* update database counters if new data was inserted */
		if (logcount) {
			if (!db_setcounters(iterator->interface, iterator->currx, iterator->curtx)) {
				handledatabaseerror(s);
				break;
			}
		}

		if (!iterator->active && !logcount) {
			/* throw away if interface hasn't seen any data and is disabled */
			if (!iterator->currx && !iterator->curtx) {
				ret = db_getinterfaceinfo(iterator->interface, &info);
				if (!ret || (!info.rxtotal && !info.txtotal)) {
					snprintf(errorstring, 1024, "Removing interface \"%s\" from database as it is disabled and has seen no data.", iterator->interface);
					printe(PT_Info);
					if (!db_removeinterface(iterator->interface)) {
						if (db_errcode) {
							handledatabaseerror(s);
						}
					}
					break;
				}
			}
		}

		/* update interface timestamp in database */
		if (!db_setupdated(iterator->interface, iterator->updated)) {
			handledatabaseerror(s);
			break;
		}

		/* update interface activity status in database */
		if (!db_setactive(iterator->interface, iterator->active)) {
			handledatabaseerror(s);
			break;
		}

		iterator = iterator->next;
	}

	if (db_intransaction && !db_errcode) {
		if (!db_committransaction()) {
			handledatabaseerror(s);
		} else {
			/* clear xferlog now that everything is in database */
			iterator = s->dcache;
			while (iterator != NULL) {
				xferlog_clear(&iterator->log);
				iterator = iterator->next;
			}
			s->dbretrycount = 0;
		}
	} else {
		db_rollbacktransaction();
	}
	used_secs = timeused(__func__, 0);
	if (used_secs > SLOWDBWARNLIMIT) {
		snprintf(errorstring, 1024, "Writing cached data to database took %.1f seconds.", used_secs);
		printe(PT_Warning);
	}
}

void handledatabaseerror(DSTATE *s)
{
	if (db_iserrcodefatal(db_errcode)) {
		snprintf(errorstring, 1024, "Fatal database error detected, exiting.");
		printe(PT_Error);
		errorexitdaemon(s, 1);
	} else {
		if (db_isdiskfull(db_errcode)) {
			snprintf(errorstring, 1024, "Disk is full, continuing with data caching.");
			printe(PT_Error);
		} else {
			s->dbretrycount++;
			if (s->dbretrycount > DBRETRYLIMIT) {
				snprintf(errorstring, 1024, "Database error retry limit of %d reached, exiting.", DBRETRYLIMIT);
				printe(PT_Error);
				errorexitdaemon(s, 1);
			}
		}
	}
}

void cleanremovedinterfaces(DSTATE *s)
{
	datacache *iterator = s->dcache;
	iflist *dbifl = NULL, *dbifl_iterator = NULL;

	timeused_debug(__func__, 1);

	while (iterator != NULL) {
		if (!db_getinterfacecountbyname(iterator->interface)) {
			iflistadd(&dbifl, iterator->interface, 0);
		}
		iterator = iterator->next;
	}

	if (dbifl != NULL) {
		dbifl_iterator = dbifl;
		while (dbifl_iterator != NULL) {
			snprintf(errorstring, 1024, "Removing interface \"%s\" from update list.", dbifl_iterator->interface);
			printe(PT_Info);
			datacache_remove(&s->dcache, dbifl_iterator->interface);
			s->dbifcount--;
			dbifl_iterator = dbifl_iterator->next;
		}
		datacache_status(&s->dcache);
		iflistfree(&dbifl);
	}
	timeused_debug(__func__, 0);
}

void handleintsignals(DSTATE *s)
{
	switch (intsignal) {

		case SIGHUP:
			snprintf(errorstring, 1024, "SIGHUP received, flushing data to disk and reloading config.");
			printe(PT_Info);
			flushcachetodisk(s);
			datacache_clear(&s->dcache);
			s->dbifcount = 0;
			ibwflush();
			db_close();
			loadcfg(s->cfgfile);
			ibwloadcfg(s->cfgfile);
			if (!db_open_rw(1)) {
				snprintf(errorstring, 1024, "Opening database after SIGHUP failed (%s), exiting.", strerror(errno));
				printe(PT_Error);
				if (s->rundaemon && !debug) {
					close(pidfile);
					unlink(cfg.pidfile);
				}
				exit(EXIT_FAILURE);
			}
			break;

		case SIGINT:
			snprintf(errorstring, 1024, "SIGINT received, exiting.");
			printe(PT_Info);
			s->running = 0;
			break;

		case SIGTERM:
			snprintf(errorstring, 1024, "SIGTERM received, exiting.");
			printe(PT_Info);
			s->running = 0;
			break;

		/* from filldatabaselist() */
		case 42:
			break;

		case 0:
			break;

		default:
			snprintf(errorstring, 1024, "Unkown signal %d received, ignoring.", intsignal);
			printe(PT_Info);
			break;
	}

	intsignal = 0;
}

void preparedirs(DSTATE *s)
{
	/* database directory */
	if (mkpath(cfg.dbdir, 0775)) {
		updatedirowner(cfg.dbdir, s->user, s->group);
	}

	if (!cfg.createdirs || !s->rundaemon) {
		return;
	}

	/* possible pid/lock and log directory */
	preparevnstatdir(cfg.pidfile, s->user, s->group);
	if (cfg.uselogging == 1) {
		preparevnstatdir(cfg.logfile, s->user, s->group);
	}
}

void datacache_status(datacache **dc)
{
	char buffer[1024], bwtemp[16];
	unsigned int b = 0, count = 0;
	uint32_t bwlimit = 0;
	datacache *iterator = *dc;

	timeused_debug(__func__, 1);

	snprintf(buffer, 1024, "Monitoring (%d): ", datacache_count(dc));
	b = (unsigned int)strlen(buffer) + 1;

	while (iterator != NULL) {
		if ((b + strlen(iterator->interface) + 16) < 1020) {
			if (!ibwget(iterator->interface, &bwlimit) || bwlimit == 0) {
				snprintf(bwtemp, 16, " (no limit) ");
			} else {
				snprintf(bwtemp, 16, " (%" PRIu32 " Mbit) ", bwlimit);
			}
			strcat(buffer, iterator->interface);
			strcat(buffer, bwtemp);
			b += strlen(iterator->interface) + strlen(bwtemp);
		} else {
			strcat(buffer, "...");
			break;
		}
		count++;
		iterator = iterator->next;
	}

	if (count) {
		strncpy_nt(errorstring, buffer, 1024);
	} else {
		snprintf(errorstring, 1024, "Nothing to monitor");
	}
	printe(PT_Info);
	timeused_debug(__func__, 0);
}

void interfacechangecheck(DSTATE *s)
{
	char *ifacelist, interface[32];
	datacache *iterator = s->dcache;
	uint32_t newhash;
	int offset, found;

	timeused_debug(__func__, 1);

	/* get list of currently visible interfaces */
	if (getifliststring(&ifacelist, 0) == 0) {
		free(ifacelist);
		s->iflisthash = 0;
		return;
	}

	newhash = simplehash(ifacelist, (int)strlen(ifacelist));

	if (s->iflisthash == newhash) {
		free(ifacelist);
		return;
	}

	/* search for changes if hash doesn't match */
	if (debug) {
		printf("ifacelist changed: '%s'    %u <> %u\n", ifacelist, s->iflisthash, newhash);
	}

	while (iterator != NULL) {

		if (!iterator->filled) {
			iterator = iterator->next;
			continue;
		}

		found = offset = 0;

		while (offset <= (int)strlen(ifacelist)) {
			sscanf(ifacelist + offset, "%31s", interface);
			if (strcmp(iterator->interface, interface) == 0) {
				found = 1;
				break;
			}
			offset += (int)strlen(interface) + 1;
		}

		if (iterator->active == 1 && found == 0) {
			iterator->active = 0;
			iterator->currx = 0;
			iterator->curtx = 0;
			if (cfg.savestatus) {
				s->forcesave = 1;
			}
			snprintf(errorstring, 1024, "Interface \"%s\" disabled.", iterator->interface);
			printe(PT_Info);
		} else if (iterator->active == 0 && found == 1) {
			iterator->active = 1;
			iterator->currx = 0;
			iterator->curtx = 0;
			if (cfg.savestatus) {
				s->forcesave = 1;
			}
			snprintf(errorstring, 1024, "Interface \"%s\" enabled.", iterator->interface);
			printe(PT_Info);
		}

		iterator = iterator->next;
	}
	free(ifacelist);

	s->iflisthash = newhash;
	timeused_debug(__func__, 0);
}

uint32_t simplehash(const char *data, int len)
{
	uint32_t hash;

	if (len <= 0 || data == NULL) {
		return 0;
	}

	hash = (uint32_t)len;

	for (len--; len >= 0; len--) {
		if (len > 0) {
			hash += (uint32_t)data[len] * (uint32_t)len;
		} else {
			hash += (uint32_t)data[len];
		}
	}

	return hash;
}

__attribute__((noreturn)) void errorexitdaemon(DSTATE *s, const int fataldberror)
{
	if (!fataldberror) {
		flushcachetodisk(s);
	}
	db_close();

	datacache_clear(&s->dcache);
	ibwflush();

	if (s->rundaemon && !debug) {
		close(pidfile);
		unlink(cfg.pidfile);
	}

	exit(EXIT_FAILURE);
}

short getcurrenthour(void)
{
	int ret = 0;
	time_t current;
	struct tm *stm;
	char buffer[4];

	current = time(NULL);
	stm = localtime(&current);
	if (stm == NULL) {
		return 0;
	}

	if (!strftime(buffer, sizeof(buffer), "%H", stm)) {
		return 0;
	}

	ret = atoi(buffer);
	if (ret > 23 || ret < 0) {
		ret = 0;
	}

	return (short)ret;
}

int waittimesync(DSTATE *s)
{
	datacache *iterator = s->dcache;
	char timestamp[22], timestamp2[22];

	if (cfg.timesyncwait == 0) {
		return 0;
	}

	if (s->prevdbupdate == 0 && s->prevdbsave == 0) {
		while (iterator != NULL) {
			if (debug) {
				printf("w: processing %s...\n", iterator->interface);
			}

			if (!iterator->filled) {
				if (!initcachevalues(s, &iterator)) {
					iterator = iterator->next;
					continue;
				}
				s->iflisthash = 0;
			}

			if (debug) {
				strftime(timestamp, 22, DATETIMEFORMAT, localtime(&iterator->updated));
				printf("w: has %s\n", timestamp);
			}
			if (iterator->updated > s->prevdbsave) {
				s->prevdbsave = iterator->updated;
			}
			iterator = iterator->next;
		}
		if (s->prevdbsave == 0) {
			snprintf(errorstring, 1024, "Couldn't define when database was last updated. Continuing, some errors may follow.");
			printe(PT_Info);
			return 0;
		}
	}

	s->current = time(NULL);

	if (debug) {
		strftime(timestamp, 22, DATETIMEFORMAT, localtime(&s->current));
		printf("current time:     %s\n", timestamp);
		strftime(timestamp2, 22, DATETIMEFORMAT, localtime(&s->prevdbsave));
		printf("latest db update: %s\n", timestamp2);
	}

	if (s->current < s->prevdbsave) {
		if (s->prevdbupdate == 0) {
			s->prevdbupdate = s->current;
			strftime(timestamp, 22, DATETIMEFORMAT, localtime(&s->current));
			strftime(timestamp2, 22, DATETIMEFORMAT, localtime(&s->prevdbsave));
			snprintf(errorstring, 1024, "Latest database update is in the future (db: %s > now: %s). Giving the system clock up to %d minutes to sync before continuing.", timestamp2, timestamp, cfg.timesyncwait);
			printe(PT_Info);
		}
		if (s->current - s->prevdbupdate >= cfg.timesyncwait * 60) {
			strftime(timestamp, 22, DATETIMEFORMAT, localtime(&s->current));
			strftime(timestamp2, 22, DATETIMEFORMAT, localtime(&s->prevdbsave));
			snprintf(errorstring, 1024, "Latest database update is still in the future (db: %s > now: %s), continuing. Some errors may follow.", timestamp2, timestamp);
			printe(PT_Info);
			return 0;
		}
	} else {
		if (s->prevdbupdate != 0) {
			strftime(timestamp, 22, DATETIMEFORMAT, localtime(&s->current));
			strftime(timestamp2, 22, DATETIMEFORMAT, localtime(&s->prevdbsave));
			snprintf(errorstring, 1024, "Latest database update is no longer in the future (db: %s <= now: %s), continuing.", timestamp2, timestamp);
			printe(PT_Info);
		}
		s->prevdbsave = s->current;
		s->prevdbupdate = 0;
		if (debug) {
			printf("time sync ok\n\n");
		}
		return 0;
	}

	return 1;
}
