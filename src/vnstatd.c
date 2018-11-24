/*
vnStat daemon - Copyright (c) 2008-2018 Teemu Toivola <tst@iki.fi>

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; version 2 dated June, 1991.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License along
   with this program; if not, write to the Free Software Foundation, Inc.,
   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#include "common.h"
#include "datacache.h"
#include "dbsql.h"
#include "cfg.h"
#include "ibw.h"
#include "id.h"
#include "daemon.h"
#include "vnstatd.h"

int main(int argc, char *argv[])
{
	int currentarg;
	uint32_t previflisthash;
	uint64_t temp;
	DSTATE s;

	initdstate(&s);

	/* early check for debug and config parameter */
	if (argc > 1) {
		for (currentarg=1; currentarg<argc; currentarg++) {
			if ((strcmp(argv[currentarg],"-D")==0) || (strcmp(argv[currentarg],"--debug")==0)) {
				debug = 1;
				printf("Debug enabled, vnstatd %s\n", VERSION);
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
			debug = 1;
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
		} else if (strcmp(argv[currentarg],"--alwaysadd")==0) {
			s.alwaysadd = 1;
		} else if ((strcmp(argv[currentarg],"-v")==0) || (strcmp(argv[currentarg],"--version")==0)) {
			printf("vnStat daemon %s by Teemu Toivola <tst at iki dot fi>\n", getversion());
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

	if (s.noadd && s.alwaysadd) {
		printf("Error: --noadd and --alwaysadd can't both be used at the same time.\n");
		return 1;
	}

	if (s.rundaemon && debug) {
		printf("Error: --daemon and --debug can't both be used at the same time.\n");
		return 1;
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

	if (!db_open_rw(1)) {
		printf("Error: Unable to open database \"%s/%s\": %s\n", s.dirname, DATABASEFILE, strerror(errno));
		printf("Exiting...\n");
		exit(EXIT_FAILURE);
	}

	detectboot(&s);
	preparedatabases(&s);

	if (!db_removeoldentries()) {
		printf("Error: Database \"%s/%s\" cleanup failed: %s\n", s.dirname, DATABASEFILE, strerror(errno));
		printf("Exiting...\n");
		exit(EXIT_FAILURE);
	}

	setsignaltraps();

	/* start as daemon if requested, debug can't be enabled at the same time */
	if (s.rundaemon && !debug) {
		if (!db_close()) {
			printf("Error: Failed to close database \"%s/%s\" before starting daemon: %s\n", s.dirname, DATABASEFILE, strerror(errno));
			printf("Exiting...\n");
			exit(EXIT_FAILURE);
		}
		noexit++;
		daemonize();
		if (!db_open_rw(0)) {
			snprintf(errorstring, 1024, "Failed to reopen database \"%s/%s\": %s", s.dirname, DATABASEFILE, strerror(errno));
			printe(PT_Error);
			exit(EXIT_FAILURE);
		}
	}

	s.running = 1;
	snprintf(errorstring, 1024, "vnStat daemon %s started. (pid:%d uid:%d gid:%d)", getversion(), (int)getpid(), (int)getuid(), (int)getgid());
	printe(PT_Info);

	/* warmup */
	if (s.dbcount == 0) {
		filldatabaselist(&s);
		s.prevdbsave = 0;
	}
	while (s.running && s.dbcount && waittimesync(&s)) {
		if (intsignal) {
			handleintsignals(&s);
		} else {
			sleep(5);
		}
	}

	/* main loop */
	while (s.running) {

		s.current = time(NULL);

		/* track interface status only if at least one database exists */
		if (s.dbcount != 0) {
			previflisthash = s.iflisthash;
			interfacechangecheck(&s);
			if (s.alwaysadd && s.iflisthash != previflisthash && previflisthash != 0) {
				temp = s.dbcount;
				s.dbcount += addinterfaces(&s);
				if (temp != s.dbcount) {
					datacache_status(&s.dcache);
				}
			}
		}

		/* do update only if enough time has passed since the previous update */
		if ((s.current - s.prevdbupdate) >= s.updateinterval) {

			s.updateinterval = cfg.updateinterval;

			if (debug) {
				debugtimestamp();
				datacache_debug(&s.dcache);
				ibwlist();
			}

			/* fill database list if cache is empty */
			if (s.dbcount == 0) {
				filldatabaselist(&s);

			/* update data cache */
			} else {
				s.prevdbupdate = s.current - (s.current % s.updateinterval);

				adjustsaveinterval(&s);
				checkdbsaveneed(&s);

				processdatacache(&s);

				if (debug) {
					printf("\n");
				}
			}
		}

		if (s.running && intsignal == 0) {
			sleep((unsigned int)(cfg.pollinterval - (time(NULL) % cfg.pollinterval)));
		}

		if (intsignal) {
			handleintsignals(&s);
		}
	}

	flushcachetodisk(&s);
	db_close();

	datacache_clear(&s.dcache);
	ibwflush();

	if (s.rundaemon && !debug) {
		close(pidfile);
		unlink(cfg.pidfile);
	}

	return 0;
}

void showhelp(void)
{
	printf("vnStat daemon %s by Teemu Toivola <tst at iki dot fi>\n\n", getversion());

	printf("      -d, --daemon             fork process to background\n");
	printf("      -n, --nodaemon           stay in foreground attached to the terminal\n\n");

	printf("      -s, --sync               sync interface counters on first update\n");
	printf("      -D, --debug              show additional debug and disable daemon\n");
	printf("      -?, --help               show this help\n");
	printf("      -v, --version            show version\n");
	printf("      -p, --pidfile <file>     select used pid file\n");
	printf("      -u, --user <user>        set daemon process user\n");
	printf("      -g, --group <group>      set daemon process group\n");
	printf("      --config <config file>   select used config file\n");
	printf("      --noadd                  don't add found interfaces if no dbs are found\n");
	printf("      --alwaysadd              always add new interfaces even when some dbs exist\n\n");

	printf("See also \"man vnstatd\".\n");
}
