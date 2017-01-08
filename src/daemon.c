#include "common.h"
#include "ifinfo.h"
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

	if (getppid()==1) {
		return; /* already a daemon */
	}

	i = (int)fork();

	if (i<0) { /* fork error */
		perror("Error: fork");
		exit(EXIT_FAILURE);
	}
	if (i>0) { /* parent exits */
		exit(EXIT_SUCCESS);
	}
	/* child (daemon) continues */

	setsid(); /* obtain a new process group */

	if (!verifylogaccess()) {
		printf("Error: Unable to use logfile. Exiting.\n");
		exit(EXIT_FAILURE);
	}

	/* lock / pid file */
	pidfile = open(cfg.pidfile, O_RDWR|O_CREAT, 0644);
	if (pidfile<0) {
		perror("Error: pidfile");
		snprintf(errorstring, 512, "opening pidfile \"%s\" failed (%s), exiting.", cfg.pidfile, strerror(errno));
		printe(PT_Error);
		exit(EXIT_FAILURE); /* can't open */
	}
	if (lockf(pidfile,F_TLOCK,0)<0) {
		perror("Error: pidfile lock");
		snprintf(errorstring, 512, "pidfile \"%s\" lock failed (%s), exiting.", cfg.pidfile, strerror(errno));
		printe(PT_Error);
		exit(EXIT_FAILURE); /* can't lock */
	}

	/* close all descriptors except lock file */
	for (i=getdtablesize();i>=0;--i) {
		if (i!=pidfile) {
			close(i);
		}
	}

	/* redirect standard i/o to null */
	i=open("/dev/null",O_RDWR); /* stdin */

	if (i < 0) {
		perror("Error: open() /dev/null");
		snprintf(errorstring, 512, "open() /dev/null failed, exiting.");
		printe(PT_Error);
		exit(EXIT_FAILURE);
	}

	/* stdout */
	if (dup(i) < 0) {
		perror("Error: dup(stdout)");
		snprintf(errorstring, 512, "dup(stdout) failed, exiting.");
		printe(PT_Error);
		exit(EXIT_FAILURE);
	}
	/* stderr */
	if (dup(i) < 0) {
		perror("Error: dup(stderr)");
		snprintf(errorstring, 512, "dup(stderr) failed, exiting.");
		printe(PT_Error);
		exit(EXIT_FAILURE);
	}

	umask(027); /* set newly created file permissions */

	/* change running directory */
	if (chdir("/") < 0) {
		perror("Error: chdir(/)");
		snprintf(errorstring, 512, "directory change to / failed, exiting.");
		printe(PT_Error);
		exit(EXIT_FAILURE);
	}

	/* first instance continues */
	snprintf(str, 10, "%d\n", (int)getpid());

	/* record pid to pidfile */
	if (write(pidfile,str,strlen(str)) < 0) {
		perror("Error: write(pidfile)");
		snprintf(errorstring, 512, "writing to pidfile %s failed, exiting.", cfg.pidfile);
		printe(PT_Error);
		exit(EXIT_FAILURE);
	}

	signal(SIGCHLD,SIG_IGN); /* ignore child */
	signal(SIGTSTP,SIG_IGN); /* ignore tty signals */
	signal(SIGTTOU,SIG_IGN);
	signal(SIGTTIN,SIG_IGN);
}

int addinterfaces(DSTATE *s)
{
	char *ifacelist, interface[32], buffer[32];
	int index = 0, count = 0, bwlimit = 0;

	/* get list of currently visible interfaces */
	if (getiflist(&ifacelist, 0)==0) {
		free(ifacelist);
		return 0;
	}

	if (strlen(ifacelist)<2) {
		free(ifacelist);
		return 0;
	}

	if (debug)
		printf("Interface list: \"%s\"\n", ifacelist);

	while (sscanf(ifacelist+index, "%31s", interface)!=EOF) {
		if (debug)
			printf("Processing: \"%s\"\n", interface);

		index += strlen(interface)+1;

		/* skip local interfaces */
		if ((strcmp(interface,"lo")==0) || (strcmp(interface,"lo0")==0) || (strcmp(interface,"sit0")==0)) {
			if (debug)
				printf("skip\n");
			continue;
		}

		/* skip already known interfaces */
		if (db_getinterfacecountbyname(interface)) {
			if (debug)
				printf("already known\n");
			continue;
		}

		/* create database for interface */
		if (!db_addinterface(interface)) {
			if (debug)
				printf("add failed, skip\n");
		}

		if (!getifinfo(interface)) {
			if (debug)
				printf("getifinfo failed, skip\n");
			continue;
		}

		/* TODO: this is most likely the wrong place to store btime */
		snprintf(buffer, 32, "%"PRIu64"", (uint64_t)MAX32);
		db_setinfo("btime", buffer, 1);

		db_setcounters(interface, ifinfo.rx, ifinfo.tx);

		count++;
		bwlimit = ibwget(interface);
		if (!s->running) {
			if (bwlimit > 0) {
				printf("\"%s\" added with %d Mbit bandwidth limit.\n", interface, bwlimit);
			} else {
				printf("\"%s\" added. Warning: no bandwidth limit has been set.\n", interface);
			}
		} else {
			if (debug)
				printf("\%s\" added with %d Mbit bandwidth limit to cache.\n", interface, bwlimit);
			datacache_add(&s->dcache, interface, 1);
		}
	}

	if (count && !s->running) {
		if (count==1) {
			printf("-> %d interface added.\n", count);
		} else {
			printf("-> %d interfaces added.\n", count);
		}

		printf("Limits can be modified using the configuration file. See \"man vnstat.conf\".\n");
		printf("Unwanted interfaces can be removed from monitoring with \"vnstat --delete\".\n");
	}

	free(ifacelist);
	return count;
}

void debugtimestamp(void)
{
	time_t now;
	char timestamp[22];

	now = time(NULL);
	strftime(timestamp, 22, "%Y-%m-%d %H:%M:%S", localtime(&now));
	printf("%s\n", timestamp);
}

void initdstate(DSTATE *s)
{
	noexit = 1;        /* disable exits in functions */
	debug = 0;         /* debug disabled by default */
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
	s->dirname[0] = '\0';
	s->user[0] = '\0';
	s->group[0] = '\0';
	s->prevdbupdate = 0;
	s->prevdbsave = 0;
	s->dbcount = 0;
	s->dodbsave = 0;
	s->dcache = NULL;
}

void preparedatabases(DSTATE *s)
{
	s->dbcount = db_getinterfacecount();

	if (s->dbcount > 0 && !s->alwaysadd) {
		s->dbcount = 0;
		return;
	}

	if (s->noadd) {
		printf("Zero database found, exiting.\n");
		exit(EXIT_FAILURE);
	}

	if (!spacecheck(s->dirname)) {
		printf("Error: Not enough free diskspace available, exiting.\n");
		exit(EXIT_FAILURE);
	}

	if (importlegacydbs(s) && !s->alwaysadd) {
		s->dbcount = 0;
		return;
	}

	if (s->dbcount == 0) {
		printf("Zero database found, adding available interfaces...\n");
	}

	if (!addinterfaces(s) && s->dbcount == 0) {
		printf("Nothing to do, exiting.\n");
		exit(EXIT_FAILURE);
	}

	/* set counter back to zero so that dbs will be cached later */
	s->dbcount = 0;
}

int importlegacydbs(DSTATE *s)
{
	DIR *dir;
	struct dirent *di;
	int importcount = 0;

	if ((dir=opendir(s->dirname))==NULL) {
		printf("Error: Unable to open database directory \"%s\": %s\n", s->dirname, strerror(errno));
		printf("Make sure it exists and is at least read enabled for current user.\n");
		printf("Exiting...\n");
		exit(EXIT_FAILURE);
	}

	s->dbcount = 0;
	while ((di=readdir(dir))) {
		if ((di->d_name[0]!='.') && (strcmp(di->d_name, DATABASEFILE)!=0)) {
			/* ignore already known interfaces */
			if (db_getinterfacecountbyname(di->d_name)) {
				continue;
			}
			if (importlegacydb(di->d_name, s->dirname)) {
				importcount++;
			}
		}
	}
	closedir(dir);

	s->dbcount += importcount;
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
	dbiflist *dbifl = NULL, *dbifl_iterator = NULL;

	if (db_getiflist(&dbifl) < 0) {
		snprintf(errorstring, 512, "Unable to access database(%s), exiting.", strerror(errno));
		printe(PT_Error);
		errorexitdaemon(s);
	}

	dbifl_iterator = dbifl;

	while (dbifl_iterator != NULL) {
		if (debug) {
			printf("\nProcessing interface \"%s\"...\n", dbifl_iterator->interface);
		}
		if (!datacache_add(&s->dcache, dbifl_iterator->interface, s->sync)) {
			snprintf(errorstring, 512, "Cache memory allocation failed, exiting.");
			printe(PT_Error);
			errorexitdaemon(s);
		}
		s->dbcount++;
		dbifl_iterator = dbifl_iterator->next;
	}

	dbiflistfree(&dbifl);
	s->sync = 0;

	/* disable update interval check for one loop if database list was refreshed */
	/* otherwise increase default update interval since there's nothing else to do */
	if (s->dbcount) {
		s->updateinterval = 0;
		intsignal = 42;
		s->prevdbsave = s->current;
		/* list monitored interfaces to log */
		datacache_status(&s->dcache);
	} else {
		s->updateinterval = 120;
	}
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
		s->prevdbsave = s->current;
	} else {
		s->dodbsave = 0;
	}
}

void processdatacache(DSTATE *s)
{
	datacache *iterator = s->dcache;

	while (iterator != NULL) {

		if (debug) {
			printf("dc: processing %s (%d)...\n", iterator->interface, s->dodbsave);
		}

		if (!iterator->filled) {
			if (!initcachevalues(&iterator)) {
				iterator = iterator->next;
				continue;
			}
			s->iflisthash = 0;
		}

		if (iterator->active) {
			if (!getifinfo(iterator->interface)) {
				/* disable interface since we can't access its data */
				iterator->active = 0;
				snprintf(errorstring, 512, "Interface \"%s\" not available, disabling.", iterator->interface);
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

	if (s->dodbsave) {
		flushcachetodisk(s);
		cleanremovedinterfaces(s);
	}
}

int initcachevalues(datacache **dc)
{
	uint64_t rx, tx;

	if (!db_getcounters((*dc)->interface, &rx, &tx)) {
		return 0;
	}

	(*dc)->currx = rx;
	(*dc)->curtx = tx;
	/* TODO: "updated" should come from database */
	(*dc)->updated = time(NULL);
	(*dc)->filled = 1;

	return 1;
}

int processifinfo(DSTATE *s, datacache **dc)
{
	uint64_t rxchange, txchange;
	uint64_t maxtransfer;
	time_t interval;
	int maxbw;

	if ((*dc)->syncneeded) { /* if --sync was used during startup */
		(*dc)->currx = ifinfo.rx;
		(*dc)->curtx = ifinfo.tx;
		(*dc)->syncneeded = 0;
		return 1;
	}

	if ((*dc)->updated > ifinfo.timestamp) {
		/* skip update if previous update is less than a day in the future */
		/* otherwise exit with error message since the clock is problably messed */
		if ((*dc)->updated > (ifinfo.timestamp+86400)) {
			snprintf(errorstring, 512, "Interface \"%s\" has previous update date too much in the future, exiting. (%u / %u)", (*dc)->interface, (unsigned int)(*dc)->updated, (unsigned int)ifinfo.timestamp);
			printe(PT_Error);
			errorexitdaemon(s);
		}
		return 0;
	}

	interval = ifinfo.timestamp -(*dc)->updated;
	if ( (interval >= 1) && (interval <= (60*MAXUPDATEINTERVAL)) ) {
		/* TODO: add btime handling here or somewhere else */
		/*
		if (data.btime < (btime-cfg.bvar)) {
			data.currx=0;
			data.curtx=0;
			if (debug)
				printf("System has been booted.\n");
		}
		*/

		rxchange = countercalc(&(*dc)->currx, &ifinfo.rx);
		txchange = countercalc(&(*dc)->curtx, &ifinfo.tx);

		/* get bandwidth limit for current interface */
		maxbw = ibwget((*dc)->interface);

		if (maxbw > 0) {

			/* calculate maximum possible transfer since last update based on set maximum rate */
			/* and add 10% in order to be on the safe side */
			maxtransfer = ceil((maxbw/(float)8)*interval*(float)1.1) * 1024 * 1024;

			if (debug)
				printf("interval: %"PRIu64"  maxbw: %d  maxrate: %"PRIu64"  rxc: %"PRIu64"  txc: %"PRIu64"\n", (uint64_t)interval, maxbw, maxtransfer, rxchange, txchange);

			/* sync counters if traffic is greater than set maximum */
			if ( (rxchange > maxtransfer) || (txchange > maxtransfer) ) {
				snprintf(errorstring, 512, "Traffic rate for \"%s\" higher than set maximum %d Mbit (%"PRIu64"->%"PRIu64", r%"PRIu64" t%"PRIu64"), syncing.", (*dc)->interface, maxbw, (uint64_t)interval, maxtransfer, rxchange, txchange);
				printe(PT_Info);
				rxchange = txchange = 0;
			}
		}

		if (rxchange || txchange) {
			xferlog_add(&(*dc)->log, ifinfo.timestamp - (ifinfo.timestamp % 300), rxchange, txchange);
		}
	}
	(*dc)->currx = ifinfo.rx;
	(*dc)->curtx = ifinfo.tx;
	(*dc)->updated = ifinfo.timestamp;

	return 1;
}

void flushcachetodisk(DSTATE *s)
{
	datacache *iterator = s->dcache;
	xferlog *logiterator;

	while (iterator != NULL) {
		/* TODO: error handling needed, if the disk is full then keep the cache up to some time limit */

		/* ignore interface no longer in database */
		if (!db_getinterfacecountbyname(iterator->interface)) {
			iterator = iterator->next;
			continue;
		}

		logiterator = iterator->log;
		while (logiterator != NULL) {
			db_addtraffic_dated(iterator->interface, logiterator->rx, logiterator->tx, (uint64_t)logiterator->timestamp);
			logiterator = logiterator->next;
		}
		xferlog_clear(&iterator->log);
		db_setcounters(iterator->interface, iterator->currx, iterator->curtx);
		if (!iterator->active) {
			db_setactive(iterator->interface, iterator->active);
		}

		iterator = iterator->next;
	}
}

void cleanremovedinterfaces(DSTATE *s)
{
	datacache *iterator = s->dcache;
	dbiflist *dbifl = NULL, *dbifl_iterator = NULL;

	while (iterator != NULL) {
		if (!db_getinterfacecountbyname(iterator->interface)) {
			dbiflistadd(&dbifl, iterator->interface);
		}
		iterator = iterator->next;
	}

	if (dbifl != NULL) {
		dbifl_iterator = dbifl;
		while (dbifl_iterator != NULL) {
			snprintf(errorstring, 512, "Removing interface \"%s\" from update list.", dbifl_iterator->interface);
			printe(PT_Info);
			datacache_remove(&s->dcache, dbifl_iterator->interface);
			s->dbcount--;
			dbifl_iterator = dbifl_iterator->next;
		}
		datacache_status(&s->dcache);
		dbiflistfree(&dbifl);
	}
}

void handleintsignals(DSTATE *s)
{
	switch (intsignal) {

		case SIGHUP:
			snprintf(errorstring, 512, "SIGHUP received, flushing data to disk and reloading config.");
			printe(PT_Info);
			/* TODO: cleanup: cacheflush(s->dirname); */
			s->dbcount = 0;
			ibwflush();
			db_close();
			if (loadcfg(s->cfgfile)) {
				strncpy_nt(s->dirname, cfg.dbdir, 512);
			}
			ibwloadcfg(s->cfgfile);
			db_open(1);
			break;

		case SIGINT:
			snprintf(errorstring, 512, "SIGINT received, exiting.");
			printe(PT_Info);
			s->running = 0;
			break;

		case SIGTERM:
			snprintf(errorstring, 512, "SIGTERM received, exiting.");
			printe(PT_Info);
			s->running = 0;
			break;

		case 42:
			break;

		case 0:
			break;

		default:
			snprintf(errorstring, 512, "Unkown signal %d received, ignoring.", intsignal);
			printe(PT_Info);
			break;
	}

	intsignal = 0;
}

void preparedirs(DSTATE *s)
{
	/* database directory */
	if (mkpath(s->dirname, 0775)) {
		updatedirowner(s->dirname, s->user, s->group);
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
	char buffer[512], bwtemp[16];
	int b = 13, count = 0, bwlimit = 0;
	datacache *iterator = *dc;

	snprintf(buffer, b, "Monitoring: ");

	while (iterator != NULL) {
		if ((b+strlen(iterator->interface)+16) < 508) {
			bwlimit = ibwget(iterator->interface);
			if (bwlimit < 0) {
				snprintf(bwtemp, 16, " (no limit) ");
			} else {
				snprintf(bwtemp, 16, " (%d Mbit) ", bwlimit);
			}
			strncat(buffer, iterator->interface, strlen(iterator->interface));
			strncat(buffer, bwtemp, strlen(bwtemp));
			b += strlen(iterator->interface) + strlen(bwtemp);
		} else {
			strcat(buffer, "...");
			break;
		}
		count++;
		iterator = iterator->next;
	}

	if (count) {
		strncpy_nt(errorstring, buffer, 512);
		errorstring[511] = '\0';
	} else {
		snprintf(errorstring, 512, "Nothing to monitor");
	}
	printe(PT_Info);
}

void interfacechangecheck(DSTATE *s)
{
	char *ifacelist, interface[32];
	datacache *iterator = s->dcache;
	uint32_t newhash;
	int offset, found;

	/* get list of currently visible interfaces */
	if (getiflist(&ifacelist, 0)==0) {
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
			sscanf(ifacelist+offset, "%31s", interface);
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
			snprintf(errorstring, 512, "Interface \"%s\" disabled.", iterator->interface);
			printe(PT_Info);
		} else if (iterator->active == 0 && found == 1) {
			iterator->active = 1;
			iterator->currx = 0;
			iterator->curtx = 0;
			if (cfg.savestatus) {
				s->forcesave = 1;
			}
			snprintf(errorstring, 512, "Interface \"%s\" enabled.", iterator->interface);
			printe(PT_Info);
		}

		iterator = iterator->next;
	}
	free(ifacelist);

	s->iflisthash = newhash;
}

uint32_t simplehash(const char *data, int len)
{
	uint32_t hash = len;

	if (len <= 0 || data == NULL) {
		return 0;
	}

	for (len--; len >= 0; len--) {
		if (len > 0) {
			hash += (int)data[len] * len;
		} else {
			hash += (int)data[len];
		}
	}

	return hash;
}

void errorexitdaemon(DSTATE *s)
{
	flushcachetodisk(s);
	db_close();

	datacache_clear(&s->dcache);
	ibwflush();

	if (s->rundaemon && !debug) {
		close(pidfile);
		unlink(cfg.pidfile);
	}

	exit(EXIT_FAILURE);
}
