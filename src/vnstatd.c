/*
vnStat daemon - Copyright (c) 2008-2015 Teemu Toivola <tst@iki.fi>

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
#include "dbcache.h"
#include "dbsql.h"
#include "cfg.h"
#include "ibw.h"
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
	if (!ibwloadcfg(s.cfgfile)) {
		return 1;
	}

	/* init dirname and other config settings */
	strncpy_nt(s.dirname, cfg.dbdir, 512);
	strncpy_nt(s.user, cfg.daemonuser, 33);
	strncpy_nt(s.group, cfg.daemongroup, 33);
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

	preparedirs(&s);

	/* set user and/or group if requested */
	setgroup(s.group);
	setuser(s.user);

	if (!db_open(1)) {
		printf("Error: Unable to open database \"%s/%s\": %s\n", s.dirname, DATABASEFILE, strerror(errno));
		printf("Exiting...\n");
		exit(EXIT_FAILURE);
	}

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
				ibwlist();
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
