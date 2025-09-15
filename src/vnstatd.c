/*
vnStat daemon - Copyright (C) 2008-2025 Teemu Toivola <tst@iki.fi>

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
#include "misc.h"
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
		for (currentarg = 1; currentarg < argc; currentarg++) {
			if ((strcmp(argv[currentarg], "-D") == 0) || (strcmp(argv[currentarg], "--debug") == 0)) {
				if (!debug) {
					printf("Debug enabled, vnstatd %s, SQLite %s\n", VERSION, sqlite3_libversion());
				}
				if (debug < 2) {
					debug += 1;
				}
			} else if (strcmp(argv[currentarg], "--config") == 0) {
				if (currentarg + 1 < argc) {
					strncpy_nt(s.cfgfile, argv[currentarg + 1], 512);
					if (debug)
						printf("Used config file: %s\n", s.cfgfile);
					currentarg++;
				} else {
					printf("Error: File for --config missing.\n");
					return 1;
				}
			}
		}
	}

	timeused_debug("daemon_startup", 1);

	/* load config if available */
	if (!loadcfg(s.cfgfile, CT_Daemon)) {
		return 1;
	}
	if (!ibwloadcfg(s.cfgfile)) {
		return 1;
	}

	/* init config settings */
	strncpy_nt(s.user, cfg.daemonuser, 33);
	strncpy_nt(s.group, cfg.daemongroup, 33);
	s.updateinterval = cfg.updateinterval;
	s.saveinterval = cfg.saveinterval * 60;

	parseargs(&s, argc, argv);

	preparedirs(&s);

	/* set user and/or group if requested */
	setgroup(s.group);
	setuser(s.user);

	if (!db_open_rw(1)) {
		printf("Error: Failed to open database \"%s/%s\" in read/write mode.\n", cfg.dbdir, DATABASEFILE);
		printf("Exiting...\n");
		exit(EXIT_FAILURE);
	}

	detectboot(&s);
	preparedatabase(&s);

	if (s.initdb) {
		db_close();
		if (debug) {
			printf("--initdb complete, exiting...\n");
		}
		exit(EXIT_SUCCESS);
	}

	if (cfg.fiveminutehours == 0 && cfg.hourlydays == 0 && cfg.dailydays == 0 && cfg.monthlymonths == 0 && cfg.yearlyyears == 0 && cfg.topdayentries == 0) {
		printf("Error: All data resolutions have been disabled in data retention configuration:");
		printf("  5MinuteHours   %d\n", cfg.fiveminutehours);
		printf("  HourlyDays     %d\n", cfg.hourlydays);
		printf("  DailyDays      %d\n", cfg.dailydays);
		printf("  MonthlyMonths  %d\n", cfg.monthlymonths);
		printf("  YearlyYears    %d\n", cfg.yearlyyears);
		printf("  TopDayEntries  %d\n", cfg.topdayentries);
		printf("Exiting...\n");
		exit(EXIT_FAILURE);
	}

	if (s.dbifcount > 0) {
		if (!db_removeoldentries()) {
			printf("Error: Database \"%s/%s\" old entry cleanup failed: %s\n", cfg.dbdir, DATABASEFILE, strerror(errno));
			printf("Exiting...\n");
			exit(EXIT_FAILURE);
		}

		if (!db_removedisabledresolutionentries()) {
			printf("Error: Database \"%s/%s\" disabled resolution entry cleanup failed: %s\n", cfg.dbdir, DATABASEFILE, strerror(errno));
			printf("Exiting...\n");
			exit(EXIT_FAILURE);
		}

		if (cfg.vacuumonstartup) {
			if (!db_vacuum()) {
				printf("Error: Database \"%s/%s\" vacuum failed: %s\n", cfg.dbdir, DATABASEFILE, strerror(errno));
				printf("Exiting...\n");
				exit(EXIT_FAILURE);
			}
		}
	}

	setsignaltraps();

	/* start as daemon if requested, debug can't be enabled at the same time */
	if (s.rundaemon == 1 && !debug) {
		if (!db_close()) {
			printf("Error: Failed to close database \"%s/%s\" before starting daemon: %s\n", cfg.dbdir, DATABASEFILE, strerror(errno));
			printf("Exiting...\n");
			exit(EXIT_FAILURE);
		}
		noexit++;
		daemonize();
		if (!db_open_rw(0)) {
			snprintf(errorstring, 1024, "Failed to reopen database \"%s/%s\": %s", cfg.dbdir, DATABASEFILE, strerror(errno));
			printe(PT_Error);
			exit(EXIT_FAILURE);
		}
	}

	timeused_debug("daemon_startup", 0);
	s.running = 1;

	printstartupdetails();

	/* warmup */
	s.dbifcount = 0;
	filldatabaselist(&s);
	s.prevdbsave = 0;

	while (s.running && s.dbifcount && waittimesync(&s)) {
		if (intsignal) {
			handleintsignals(&s);
		} else {
			sleep(5);
		}
	}

	/* main loop */
	while (s.running) {

		s.current = time(NULL);

		/* track interface status only if at least one database entry exists */
		if (s.dbifcount != 0) {
			previflisthash = s.iflisthash;
			interfacechangecheck(&s);
			if (cfg.alwaysadd && s.iflisthash != previflisthash && previflisthash != 0) {
				temp = s.dbifcount;
				s.dbifcount += addinterfaces(&s);
				if (temp != s.dbifcount) {
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
			if (s.dbifcount == 0) {
				filldatabaselist(&s);

				/* update data cache */
			} else {
				s.prevdbupdate = s.current - (s.current % s.updateinterval);

				adjustsaveinterval(&s);
				checkdbsaveneed(&s);

				processdatacache(&s);

#if HAVE_DECL_SQLITE_CHECKPOINT_RESTART
				if (cfg.waldb && (s.current - s.prevwaldbcheckpoint) >= WALDBCHECKPOINTINTERVALMINS * 60) {
					db_walcheckpoint();
					s.prevwaldbcheckpoint = s.current;
				}
#endif

				if (debug) {
					printf("\n");
				}
			}
		}

		if (s.running && intsignal == 0) {
			if (s.dbifcount == 0) {
				sleep((unsigned int)(s.updateinterval - (time(NULL) % s.updateinterval)));
			} else {
				sleep((unsigned int)(cfg.pollinterval - (time(NULL) % cfg.pollinterval)));
			}
		}

		if (intsignal) {
			handleintsignals(&s);
		}
	}

	flushcachetodisk(&s);
	db_close();

	datacache_clear(&s.dcache);
	ibwflush();

	if (s.rundaemon == 1 && !debug) {
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
	printf("      -t, --timestamp          timestamp prints when running in foreground\n");
	printf("      --config <config file>   select used config file\n");
	printf("      --initdb                 create database if it doesn't exist and exit\n");
	printf("      --alwaysadd [mode]       automatically start monitoring all new interfaces\n");
	printf("      --noadd                  disable discovery of interfaces when database\n");
	printf("                               contains none, disable legacy data import when used\n");
	printf("                               in combination with --initdb\n");
	printf("      --noremove               disable removal of never seen interfaces\n");
	printf("      --startempty             start even when database is empty\n\n");

	printf("See also \"man vnstatd\".\n");
}

void parseargs(DSTATE *s, int argc, char **argv)
{
	int currentarg, pidfiledefined = 0;

	/* parse parameters, maybe not the best way but... */
	for (currentarg = 1; currentarg < argc; currentarg++) {
		if (debug)
			printf("arg %d: \"%s\"\n", currentarg, argv[currentarg]);
		if ((strcmp(argv[currentarg], "-?") == 0) || (strcmp(argv[currentarg], "--help") == 0)) {
			break;
		} else if (strcmp(argv[currentarg], "--config") == 0) {
			/* config has already been parsed earlier so nothing to do here */
			currentarg++;
		} else if ((strcmp(argv[currentarg], "-D") == 0) || (strcmp(argv[currentarg], "--debug") == 0)) {
			;
		} else if ((strcmp(argv[currentarg], "-d") == 0) || (strcmp(argv[currentarg], "--daemon") == 0)) {
			s->rundaemon = 1;
			s->showhelp = 0;
		} else if ((strcmp(argv[currentarg], "-n") == 0) || (strcmp(argv[currentarg], "--nodaemon") == 0)) {
			s->rundaemon = 0;
			s->showhelp = 0;
		} else if ((strcmp(argv[currentarg], "-s") == 0) || (strcmp(argv[currentarg], "--sync") == 0)) {
			s->sync = 1;
		} else if ((strcmp(argv[currentarg], "-t") == 0) || (strcmp(argv[currentarg], "--timestamp") == 0)) {
			cfg.timestampprints = 1;
		} else if ((strcmp(argv[currentarg], "-u") == 0) || (strcmp(argv[currentarg], "--user") == 0)) {
			if (currentarg + 1 < argc) {
				strncpy_nt(s->user, argv[currentarg + 1], 33);
				if (debug)
					printf("Requested user: \"%s\"\n", s->user);
				currentarg++;
			} else {
				printf("Error: User for --user missing.\n");
				exit(EXIT_FAILURE);
			}
		} else if ((strcmp(argv[currentarg], "-g") == 0) || (strcmp(argv[currentarg], "--group") == 0)) {
			if (currentarg + 1 < argc) {
				strncpy_nt(s->group, argv[currentarg + 1], 33);
				if (debug)
					printf("Requested group: \"%s\"\n", s->group);
				currentarg++;
			} else {
				printf("Error: Group for --group missing.\n");
				exit(EXIT_FAILURE);
			}
		} else if (strcmp(argv[currentarg], "--noadd") == 0) {
			s->noadd = 1;
		} else if (strcmp(argv[currentarg], "--noremove") == 0) {
			s->noremove = 1;
		} else if (strcmp(argv[currentarg], "--alwaysadd") == 0) {
			if (currentarg + 1 < argc && (strlen(argv[currentarg + 1]) == 1 || ishelprequest(argv[currentarg + 1]))) {
				if (!isdigit(argv[currentarg + 1][0]) || atoi(argv[currentarg + 1]) > 1 || atoi(argv[currentarg + 1]) < 0) {
					if (!ishelprequest(argv[currentarg + 1]))
						printf("Error: Invalid mode parameter \"%s\".\n", argv[currentarg + 1]);
					printf(" Valid parameters for %s:\n", argv[currentarg]);
					printf("    0 - disabled");
					if (!cfg.alwaysadd) {
						printf(" (default)");
					}
					printf("\n    1 - enabled");
					if (cfg.alwaysadd) {
						printf(" (default)");
					}
					printf("\n No mode parameter results in feature being enabled.\n");
					exit(EXIT_FAILURE);
				}
				cfg.alwaysadd = atoi(argv[currentarg + 1]);
				currentarg++;
			} else {
				cfg.alwaysadd = 1;
			}
		} else if (strcmp(argv[currentarg], "--initdb") == 0) {
			s->initdb = 1;
			s->showhelp = 0;
		} else if (strcmp(argv[currentarg], "--startempty") == 0) {
			s->startempty = 1;
		} else if ((strcmp(argv[currentarg], "-v") == 0) || (strcmp(argv[currentarg], "--version") == 0)) {
			printf("vnStat daemon %s by Teemu Toivola <tst at iki dot fi> (SQLite %s)\n", getversion(), sqlite3_libversion());
			exit(EXIT_SUCCESS);
		} else if ((strcmp(argv[currentarg], "-p") == 0) || (strcmp(argv[currentarg], "--pidfile") == 0)) {
			if (currentarg + 1 < argc) {
				strncpy_nt(cfg.pidfile, argv[currentarg + 1], 512);
				cfg.pidfile[511] = '\0';
				if (debug)
					printf("Used pid file: %s\n", cfg.pidfile);
				currentarg++;
				pidfiledefined = 1;
			} else {
				printf("Error: File for --pidfile missing.\n");
				exit(EXIT_FAILURE);
			}
		} else {
			printf("Unknown arg \"%s\". Use --help for help.\n", argv[currentarg]);
			exit(EXIT_FAILURE);
		}
	}

	if (s->noadd && cfg.alwaysadd) {
		printf("Warning: --noadd and --alwaysadd can't both be enabled at the same time. --alwaysadd has been ignored.\n");
		cfg.alwaysadd = 0;
	}

	if (s->rundaemon == 1 && debug) {
		printf("Error: --daemon and --debug can't both be used at the same time.\n");
		exit(EXIT_FAILURE);
	}

	if (s->rundaemon == 1 && s->initdb) {
		printf("Error: --daemon and --initdb can't both be used at the same time.\n");
		exit(EXIT_FAILURE);
	}

	if (s->rundaemon == 0 && s->initdb) {
		printf("Error: --nodaemon and --initdb can't both be used at the same time.\n");
		exit(EXIT_FAILURE);
	}

	if (s->startempty && s->initdb) {
		printf("Error: --startempty and --initdb can't both be used at the same time.\n");
		exit(EXIT_FAILURE);
	}

	/* show help if nothing else was asked to be done */
	if (s->showhelp) {
		showhelp();
		exit(EXIT_SUCCESS);
	}

	if (s->rundaemon != 1 && pidfiledefined) {
		printf("Error: --pidfile can only be used together with --daemon\n");
		exit(EXIT_FAILURE);
	}
}
