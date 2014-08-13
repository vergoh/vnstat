#include "common.h"
#include "ifinfo.h"
#include "dbaccess.h"
#include "dbcache.h"
#include "misc.h"
#include "cfg.h"
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

	if (cfg.uselogging) {
		snprintf(errorstring, 512, "vnStat daemon %s started. (uid:%d gid:%d)", VNSTATVERSION, (int)getuid(), (int)getgid());
		if (!printe(PT_Info)) {
			printf("Error: Unable to use logfile. Exiting.\n");
			exit(EXIT_FAILURE);
		}
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

	if (cfg.uselogging==1) {
		snprintf(errorstring, 512, "Daemon running with pid %d.", (int)getpid());
		printe(PT_Info);
	}
}

int addinterfaces(const char *dirname)
{
	char *ifacelist, interface[32];
	int index = 0, count = 0, bwlimit = 0;

	/* get list of currently visible interfaces */
	if (getiflist(&ifacelist)==0) {
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

		/* create database for interface */
		initdb();
		strncpy_nt(data.interface, interface, 32);
		strncpy_nt(data.nick, data.interface, 32);
		if (!getifinfo(interface)) {
			if (debug)
				printf("getifinfo failed, skip\n");
			continue;
		}
		parseifinfo(1);
		if (!writedb(interface, dirname, 1)) {
			continue;
		}
		count++;
		bwlimit = ibwget(interface);
		if (bwlimit > 0) {
			printf("\"%s\" added with %d Mbit bandwidth limit.\n", interface, bwlimit);
		} else {
			printf("\"%s\" added. Warning: no bandwidth limit has been set.\n", interface);
		}
	}

	if (count==1) {
		printf("-> %d interface added.", count);
	} else {
		printf("-> %d interfaces added.", count);
	}

	if (count) {
		printf("\nLimits can be modified using the configuration file. See \"man vnstat.conf\".");
	}

	printf("\n");

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

uid_t getuser(const char *user)
{
	struct passwd *pw;
	uid_t uid;

	if (!strlen(user)) {
		return getuid();
	}

	if (isnumeric(user)) {
		uid = atoi(user);
		pw = getpwuid(uid);
	} else {
		pw = getpwnam(user);
	}

	if (pw == NULL) {
		printf("Error: No such user: \"%s\".\n", user);
		exit(EXIT_FAILURE);
	}

	uid = pw->pw_uid;

	if (debug)
		printf("getuser(%s / %d): %s (%d)\n", user, atoi(user), pw->pw_name, (int)uid);

	return uid;
}

gid_t getgroup(const char *group)
{
	struct group *gr;
	gid_t gid;

	if (!strlen(group)) {
		return getgid();
	}

	if (isnumeric(group)) {
		gid = atoi(group);
		gr = getgrgid(gid);
	} else {
		gr = getgrnam(group);
	}

	if (gr == NULL) {
		printf("Error: No such group: \"%s\".\n", group);
		exit(EXIT_FAILURE);
	}

	gid = gr->gr_gid;

	if (debug)
		printf("getgroup(%s / %d): %s (%d)\n", group, atoi(group), gr->gr_name, (int)gid);

	return gid;
}

void setuser(const char *user)
{
	uid_t uid;

	if (!strlen(user)) {
		return;
	}

	if (getuid() != 0 && geteuid() != 0) {
		printf("Error: User can only be set as root.\n");
		exit(EXIT_FAILURE);
	}

	if (isnumeric(user) && atoi(user) == 0) {
		return;
	}

	uid = getuser(user);

	if (debug)
		printf("switching to user id %d.\n", uid);

	if (setuid(uid) != 0) {
		perror("Error: setuid");
		exit(EXIT_FAILURE);
	}
}

void setgroup(const char *group)
{
	gid_t gid;

	if (!strlen(group)) {
		return;
	}

	if (getuid() != 0 && geteuid() != 0) {
		printf("Error: Group can only be set as root.\n");
		exit(EXIT_FAILURE);
	}

	if (isnumeric(group) && atoi(group) == 0) {
		return;
	}

	gid = getgroup(group);

	if (debug)
		printf("switching to group id %d.\n", gid);

	if (setgid(gid) != 0) {
		perror("Error: setgid");
		exit(EXIT_FAILURE);
	}
}

void initdstate(DSTATE *s)
{
	noexit = 1;        /* disable exits in functions */
	debug = 0;         /* debug disabled by default */
	s->rundaemon = 0;  /* daemon disabled by default */

	s->running = 1;
	s->dbsaved = 1;
	s->showhelp = 1;
	s->sync = 0;
	s->forcesave = 0;
	s->noadd = 0;
	s->dbhash = 0;
	s->cfgfile[0] = '\0';
	s->dirname[0] = '\0';
	s->user[0] = '\0';
	s->group[0] = '\0';
	s->prevdbupdate = 0;
	s->prevdbsave = 0;
	s->dbcount = 0;
	s->dodbsave = 0;
	s->datalist = NULL;
}

void preparedatabases(DSTATE *s)
{
	DIR *dir;
	struct dirent *di;

	/* check that directory is ok */
	if ((dir=opendir(s->dirname))==NULL) {
		printf("Error: Unable to open database directory \"%s\": %s\n", s->dirname, strerror(errno));
		printf("Make sure it exists and is at least read enabled for current user.\n");
		printf("Exiting...\n");
		exit(EXIT_FAILURE);
	}

	/* check if there's something to work with */
	s->dbcount = 0;
	while ((di=readdir(dir))) {
		if (di->d_name[0]!='.') {
			s->dbcount++;
		}
	}
	closedir(dir);

	if (s->dbcount > 0) {
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
	printf("Zero database found, adding available interfaces...\n");
	if (!addinterfaces(s->dirname)) {
		printf("Nothing to do, exiting.\n");
		exit(EXIT_FAILURE);
	}

	/* set counter back to zero so that dbs will be cached later */
	s->dbcount = 0;
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
	DIR *dir;
	struct dirent *di;

	if ((dir=opendir(s->dirname))==NULL) {
		snprintf(errorstring, 512, "Unable to access database directory \"%s\" (%s), exiting.", s->dirname, strerror(errno));
		printe(PT_Error);

		/* clean daemon stuff before exit */
		if (s->rundaemon && !debug) {
			close(pidfile);
			unlink(cfg.pidfile);
		}
		ibwflush();
		exit(EXIT_FAILURE);
	}

	while ((di=readdir(dir))) {
		if (di->d_name[0]=='.') {
			continue;
		}

		if (debug) {
			printf("\nProcessing file \"%s/%s\"...\n", s->dirname, di->d_name);
		}

		if (!cacheadd(di->d_name, s->sync)) {
			snprintf(errorstring, 512, "Cache memory allocation failed, exiting.");
			printe(PT_Error);

			/* clean daemon stuff before exit */
			if (s->rundaemon && !debug) {
				close(pidfile);
				unlink(cfg.pidfile);
			}
			ibwflush();
			exit(EXIT_FAILURE);
		}
		s->dbcount++;
	}

	closedir(dir);
	s->sync = 0;

	/* disable update interval check for one loop if database list was refreshed */
	/* otherwise increase default update interval since there's nothing else to do */
	if (s->dbcount) {
		s->updateinterval = 0;
		intsignal = 42;
		s->prevdbsave = s->current;
		/* list monitored interfaces to log */
		cachestatus();
	} else {
		s->updateinterval = 120;
	}
}

void adjustsaveinterval(DSTATE *s)
{
	/* modify active save interval if all interfaces are unavailable */
	if (cacheactivecount() > 0) {
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

void processdatalist(DSTATE *s)
{
	while (s->datalist!=NULL) {

		if (debug) {
			printf("d: processing %s (%d)...\n", s->datalist->data.interface, s->dodbsave);
		}

		/* get data from cache if available */
		if (!datalist_cacheget(s)) {
			s->datalist = s->datalist->next;
			continue;
		}

		/* get info if interface has been marked as active */
		datalist_getifinfo(s);

		/* check that the time is correct */
		if (!datalist_timevalidation(s)) {
			s->datalist = s->datalist->next;
			continue;
		}

		/* write data to file if now is the time for it */
		if (!datalist_writedb(s)) {
			/* remove interface from update list since the database file doesn't exist anymore */
			snprintf(errorstring, 512, "Database for interface \"%s\" no longer exists, removing from update list.", s->datalist->data.interface);
			printe(PT_Info);
			s->datalist = cacheremove(s->datalist->data.interface);
			s->dbcount--;
			cachestatus();
			continue;
		}

		s->datalist = s->datalist->next;
	}
}

int datalist_cacheget(DSTATE *s)
{
	if (cacheget(s->datalist)==0) {

		/* try to read data from file if not cached */
		if (readdb(s->datalist->data.interface, s->dirname)==0) {
			/* mark cache as filled on read success and force interface status update */
			s->datalist->filled = 1;
			s->dbhash = 0;
		} else {
			return 0;
		}
	}
	return 1;
}

void datalist_getifinfo(DSTATE *s)
{
	if (!data.active) {
		if (debug)
			printf("d: interface is disabled\n");
		return;
	}

	if (!getifinfo(data.interface)) {
		/* disable interface since we can't access its data */
		data.active = 0;
		snprintf(errorstring, 512, "Interface \"%s\" not available, disabling.", data.interface);
		printe(PT_Info);
		return;
	}

	if (s->datalist->sync) { /* if --sync was used during startup */
		data.currx = ifinfo.rx;
		data.curtx = ifinfo.tx;
		s->datalist->sync = 0;
	} else {
		parseifinfo(0);
	}
}

int datalist_timevalidation(DSTATE *s)
{
	if (s->current >= data.lastupdated) {
		data.lastupdated = s->current;
		cacheupdate();
		return 1;
	}

	/* skip update if previous update is less than a day in the future */
	/* otherwise exit with error message since the clock is problably messed */
	if (data.lastupdated > (s->current+86400)) {
		snprintf(errorstring, 512, "Interface \"%s\" has previous update date too much in the future, exiting. (%d / %d)", data.interface, (unsigned int)data.lastupdated, (unsigned int)s->current);
		printe(PT_Error);

		/* clean daemon stuff before exit */
		if (s->rundaemon && !debug) {
			close(pidfile);
			unlink(cfg.pidfile);
		}
		ibwflush();
		exit(EXIT_FAILURE);
	}

	return 0;
}

int datalist_writedb(DSTATE *s)
{
	if (!s->dodbsave) {
		return 1;
	}

	if (!checkdb(s->datalist->data.interface, s->dirname)) {
		return 0;
	}

	if (spacecheck(s->dirname)) {
		if (writedb(s->datalist->data.interface, s->dirname, 0)) {
			if (!s->dbsaved) {
				snprintf(errorstring, 512, "Database write possible again.");
				printe(PT_Info);
				s->dbsaved = 1;
			}
		} else {
			if (s->dbsaved) {
				snprintf(errorstring, 512, "Unable to write database, continuing with cached data.");
				printe(PT_Error);
				s->dbsaved = 0;
			}
		}
	} else {
		/* show freespace error only once */
		if (s->dbsaved) {
			snprintf(errorstring, 512, "Free diskspace check failed, unable to write database, continuing with cached data.");
			printe(PT_Error);
			s->dbsaved = 0;
		}
	}

	return 1;
}

void handleintsignals(DSTATE *s)
{
	switch (intsignal) {

		case SIGHUP:
			snprintf(errorstring, 512, "SIGHUP received, flushing data to disk and reloading config.");
			printe(PT_Info);
			cacheflush(s->dirname);
			s->dbcount = 0;
			ibwflush();
			if (loadcfg(s->cfgfile)) {
				strncpy_nt(s->dirname, cfg.dbdir, 512);
			}
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
