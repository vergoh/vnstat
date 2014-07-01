/*
vnStat daemon - Copyright (c) 2008-2014 Teemu Toivola <tst@iki.fi>

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; version 2 dated June, 1991.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program;  if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave., Cambridge, MA 02139, USA.
*/

#include "common.h"
#include "ifinfo.h"
#include "dbaccess.h"
#include "dbcache.h"
#include "misc.h"
#include "cfg.h"
#include "daemon.h"
#include "vnstatd.h"

int main(int argc, char *argv[])
{
	int currentarg;
	DSTATE s;

	initdstate(&s);

	/* early check for debug and config parameter */
	if (argc > 1) {
		for (currentarg=1; currentarg<argc; currentarg++) {
			if ((strcmp(argv[currentarg],"-D")==0) || (strcmp(argv[currentarg],"--debug")==0)) {
				debug = 1;
			} else if (strcmp(argv[currentarg],"--config")==0) {
				if (currentarg+1<argc) {
					strncpy_nt(s.cfgfile, argv[currentarg+1], 512);
					if (debug)
						printf("Used config file: %s\n", s.cfgfile);
					currentarg++;
					continue;
				} else {
					printf("Error: File for --config missing.\n");
					return 1;
				}
			}
		}
	}

	/* load config if available */
	if (!loadcfg(s.cfgfile)) {
		return 1;
	}

	/* init dirname and other config settings */
	strncpy_nt(s.dirname, cfg.dbdir, 512);
	s.updateinterval = cfg.updateinterval;
	s.saveinterval = cfg.saveinterval*60;

	/* parse parameters, maybe not the best way but... */
	for (currentarg=1; currentarg<argc; currentarg++) {
		if (debug)
			printf("arg %d: \"%s\"\n",currentarg,argv[currentarg]);
		if ((strcmp(argv[currentarg],"-?")==0) || (strcmp(argv[currentarg],"--help")==0)) {
			break;
		} else if (strcmp(argv[currentarg],"--config")==0) {
			/* config has already been parsed earlier so nothing to do here */
			currentarg++;
			continue;
		} else if ((strcmp(argv[currentarg],"-D")==0) || (strcmp(argv[currentarg],"--debug")==0)) {
			debug=1;
		} else if ((strcmp(argv[currentarg],"-d")==0) || (strcmp(argv[currentarg],"--daemon")==0)) {
			s.rundaemon = 1;
			s.showhelp = 0;
		} else if ((strcmp(argv[currentarg],"-n")==0) || (strcmp(argv[currentarg],"--nodaemon")==0)) {
			s.showhelp = 0;
		} else if ((strcmp(argv[currentarg],"-s")==0) || (strcmp(argv[currentarg],"--sync")==0)) {
			s.sync = 1;
		} else if ((strcmp(argv[currentarg],"-u")==0) || (strcmp(argv[currentarg],"--user")==0)) {
			if (currentarg+1<argc) {
				strncpy_nt(s.user, argv[currentarg+1], 33);
				if (debug)
					printf("Requested user: \"%s\"\n", s.user);
				currentarg++;
				continue;
			} else {
				printf("Error: User for --user missing.\n");
				return 1;
			}
		} else if ((strcmp(argv[currentarg],"-g")==0) || (strcmp(argv[currentarg],"--group")==0)) {
			if (currentarg+1<argc) {
				strncpy_nt(s.group, argv[currentarg+1], 33);
				if (debug)
					printf("Requested group: \"%s\"\n", s.group);
				currentarg++;
				continue;
			} else {
				printf("Error: Group for --group missing.\n");
				return 1;
			}
		} else if (strcmp(argv[currentarg],"--noadd")==0) {
			s.noadd = 1;
		} else if ((strcmp(argv[currentarg],"-v")==0) || (strcmp(argv[currentarg],"--version")==0)) {
			printf("vnStat daemon %s by Teemu Toivola <tst at iki dot fi>\n", VNSTATVERSION);
			return 0;
		} else if ((strcmp(argv[currentarg],"-p")==0) || (strcmp(argv[currentarg],"--pidfile")==0)) {
			if (currentarg+1<argc) {
				strncpy_nt(cfg.pidfile, argv[currentarg+1], 512);
				cfg.pidfile[511] = '\0';
				if (debug)
					printf("Used pid file: %s\n", cfg.pidfile);
				currentarg++;
				continue;
			} else {
				printf("Error: File for --pidfile missing.\n");
				return 1;
			}
		} else {
			printf("Unknown arg \"%s\". Use --help for help.\n",argv[currentarg]);
			return 1;
		}
	}

	/* show help if nothing else was asked to be done */
	if (s.showhelp) {
		showhelp();
		return 0;
	}

	/* set user and/or group if requested */
	setgroup(s.group);
	setuser(s.user);

	preparedatabases(&s);
	setsignaltraps();

	/* start as daemon if needed and debug isn't enabled */
	if (s.rundaemon && !debug) {
		noexit++;
		daemonize();
	}

	/* main loop */
	while (s.running) {

		s.current = time(NULL);

		/* track interface status only if at least one database exists */
		if (s.dbcount!=0) {
			s.dbhash = dbcheck(s.dbhash, &s.forcesave);
		}

		/* do update only if enough time has passed since the previous update */
		if ((s.current - s.prevdbupdate) >= s.updateinterval) {

			s.updateinterval = cfg.updateinterval;

			if (debug) {
				debugtimestamp();
				cacheshow();
			}

			/* fill database list if cache is empty */
			if (s.dbcount == 0) {
				filldatabaselist(&s);

			/* update data cache */
			} else {
				s.prevdbupdate = s.current;
				s.datalist = dataptr;

				adjustsaveinterval(&s);
				checkdbsaveneed(&s);

				processdatalist(&s);

				if (debug) {
					printf("\n");
				}
			}
		}

		if (s.running && intsignal==0) {
			sleep(cfg.pollinterval);
		}

		if (intsignal) {
			handleintsignals(&s);
		}
	}

	cacheflush(s.dirname);
	ibwflush();

	if (s.rundaemon && !debug) {
		close(pidfile);
		unlink(cfg.pidfile);
	}

	return 0;
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
}

void showhelp(void)
{
	printf(" vnStat daemon %s by Teemu Toivola <tst at iki dot fi>\n\n", VNSTATVERSION);
	printf("         -d, --daemon         fork process to background\n");
	printf("         -n, --nodaemon       stay in foreground attached to the terminal\n\n");
	printf("         -s, --sync           sync interface counters on first update\n");
	printf("         -D, --debug          show additional debug and disable daemon\n");
	printf("         -?, --help           show this help\n");
	printf("         -v, --version        show version\n");
	printf("         -p, --pidfile        select used pid file\n");
	printf("         -u, --user           set daemon process user\n");
	printf("         -g, --group          set daemon process group\n");
	printf("         --config             select used config file\n");
	printf("         --noadd              don't add found interfaces if no dbs are found\n\n");
	printf("See also \"man vnstatd\".\n");
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
		if (cacheget(s->datalist)==0) {

			/* try to read data from file if not cached */
			if (readdb(s->datalist->data.interface, s->dirname)!=-1) {
				/* mark cache as filled on read success and force interface status update */
				s->datalist->filled = 1;
				s->dbhash = 0;
			} else {
				s->datalist = s->datalist->next;
				continue;
			}
		}

		/* get info if interface has been marked as active */
		if (data.active) {
			if (getifinfo(data.interface)) {
				if (s->datalist->sync) { /* if --sync was used during startup */
					data.currx = ifinfo.rx;
					data.curtx = ifinfo.tx;
					s->datalist->sync = 0;
				} else {
					parseifinfo(0);
				}
			} else {
				/* disable interface since we can't access its data */
				data.active = 0;
				snprintf(errorstring, 512, "Interface \"%s\" not available, disabling.", data.interface);
				printe(PT_Info);
			}
		} else if (debug) {
			printf("d: interface is disabled\n");
		}

		/* check that the time is correct */
		if (s->current >= data.lastupdated) {
			data.lastupdated = s->current;
			cacheupdate();
		} else {
			/* skip update if previous update is less than a day in the future */
			/* otherwise exit with error message since the clock is problably messed */
			if (data.lastupdated > (s->current+86400)) {
				snprintf(errorstring, 512, "Interface \"%s\" has previous update date too much in the future, exiting.", data.interface);
				printe(PT_Error);

				/* clean daemon stuff before exit */
				if (s->rundaemon && !debug) {
					close(pidfile);
					unlink(cfg.pidfile);
				}
				ibwflush();
				exit(EXIT_FAILURE);
			} else {
				s->datalist = s->datalist->next;
				continue;
			}
		}

		/* write data to file if now is the time for it */
		if (s->dodbsave) {
			if (checkdb(s->datalist->data.interface, s->dirname)) {
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
			} else {
				/* remove interface from update list since the database file doesn't exist anymore */
				snprintf(errorstring, 512, "Database for interface \"%s\" no longer exists, removing from update list.", s->datalist->data.interface);
				printe(PT_Info);
				s->datalist = cacheremove(s->datalist->data.interface);
				s->dbcount--;
				cachestatus();
				continue;
			}
		}

		s->datalist = s->datalist->next;
	}
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
