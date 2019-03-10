/*
vnStat - Copyright (c) 2002-2019 Teemu Toivola <tst@iki.fi>

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
#include "ifinfo.h"
#include "traffic.h"
#include "dbsql.h"
#include "dbxml.h"
#include "dbjson.h"
#include "dbshow.h"
#include "misc.h"
#include "cfg.h"
#include "cfgoutput.h"
#include "ibw.h"
#include "fs.h"
#include "vnstat.h"

int main(int argc, char *argv[]) {

	int currentarg;
	DIR *dir = NULL;
	PARAMS p;
	dbiflist *dbifl = NULL;

	initparams(&p);

	/* early check for debug and config parameter */
	if (argc > 1) {
		for (currentarg=1; currentarg<argc; currentarg++) {
			if ((strcmp(argv[currentarg],"-D")==0) || (strcmp(argv[currentarg],"--debug")==0)) {
				debug = 1;
				printf("Debug enabled, vnstat %s\n", VERSION);
			} else if (strcmp(argv[currentarg],"--config")==0) {
				if (currentarg+1<argc) {
					strncpy_nt(p.cfgfile, argv[currentarg+1], 512);
					if (debug)
						printf("Used config file: %s\n", p.cfgfile);
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
	if (!loadcfg(p.cfgfile)) {
		return 1;
	}
	if (!ibwloadcfg(p.cfgfile)) {
		return 1;
	}

	configlocale();
	strncpy_nt(p.interface, "default", 32);
	strncpy_nt(p.definterface, cfg.iface, 32);
	strncpy_nt(p.alias, "none", 32);

	/* init dirname */
	strncpy_nt(p.dirname, cfg.dbdir, 512);

	/* parse parameters, maybe not the best way but... */
	for (currentarg=1; currentarg<argc; currentarg++) {
		if (debug)
			printf("arg %d: \"%s\"\n",currentarg,argv[currentarg]);
		if (strcmp(argv[currentarg],"--longhelp")==0) {
			showlonghelp(&p);
			return 0;
		} else if ((strcmp(argv[currentarg],"-?")==0) || (strcmp(argv[currentarg],"--help")==0)) {
			showhelp(&p);
			return 0;
		} else if ((strcmp(argv[currentarg],"-i")==0) || (strcmp(argv[currentarg],"--iface")==0)) {
			if (currentarg+1<argc) {
				if (strlen(argv[currentarg+1]) > 31) {
					printf("Error: Interface name is limited to 31 characters.\n");
					return 1;
				}
				strncpy_nt(p.interface, argv[currentarg+1], 32);
				p.defaultiface = 0;
				if (debug)
					printf("Used interface: %s\n", p.interface);
				currentarg++;
				continue;
			} else {
				printf("Error: Interface for %s missing.\n", argv[currentarg]);
				return 1;
			}
		} else if (strcmp(argv[currentarg],"--config")==0) {
			/* config has already been parsed earlier so nothing to do here */
			currentarg++;
			continue;
		} else if (strcmp(argv[currentarg],"--setalias")==0 || strcmp(argv[currentarg],"--nick")==0) {
			if (currentarg+1<argc) {
				strncpy_nt(p.alias, argv[currentarg+1], 32);
				if (debug)
					printf("Used alias: %s\n", p.alias);
				p.setalias = 1;
				currentarg++;
				continue;
			} else {
				printf("Error: Alias for %s missing.\n", argv[currentarg]);
				return 1;
			}
		} else if ((strcmp(argv[currentarg],"--style"))==0) {
			if (currentarg+1<argc && isdigit(argv[currentarg+1][0])) {
				if (cfg.ostyle > 4 || cfg.ostyle < 0) {
					printf("Error: Invalid style parameter \"%d\" for --style.\n", cfg.ostyle);
					printf(" Valid parameters:\n");
					printf("    0 - a more narrow output\n");
					printf("    1 - enable bar column if available\n");
					printf("    2 - average traffic rate in summary output\n");
					printf("    3 - average traffic rate in all outputs if available\n");
					printf("    4 - disable terminal control characters in -l / --live\n");
					printf("        and show raw values in --oneline\n");
					return 1;
				}
				cfg.ostyle = atoi(argv[currentarg+1]);
				if (debug)
					printf("Style changed: %d\n", cfg.ostyle);
				currentarg++;
				continue;
			} else {
				printf("Error: Style parameter for --style missing.\n");
				printf(" Valid parameters:\n");
				printf("    0 - a more narrow output\n");
				printf("    1 - enable bar column if available\n");
				printf("    2 - average traffic rate in summary output\n");
				printf("    3 - average traffic rate in all outputs if available\n");
				printf("    4 - disable terminal control characters in -l / --live\n");
				printf("        and show raw values in --oneline\n");
				return 1;
			}
		} else if ((strcmp(argv[currentarg],"--dbdir"))==0) {
			if (currentarg+1<argc) {
				strncpy_nt(p.dirname, argv[currentarg+1], 512);
				if (debug)
					printf("DatabaseDir: \"%s\"\n", p.dirname);
				currentarg++;
				continue;
			} else {
				printf("Error: Directory for %s missing.\n", argv[currentarg]);
				return 1;
			}
		} else if ((strcmp(argv[currentarg],"--locale"))==0) {
			if (currentarg+1<argc) {
				setlocale(LC_ALL, argv[currentarg+1]);
				if (debug)
					printf("Locale: \"%s\"\n", argv[currentarg+1]);
				currentarg++;
				continue;
			} else {
				printf("Error: Locale for %s missing.\n", argv[currentarg]);
				return 1;
			}
		} else if (strcmp(argv[currentarg],"--add")==0) {
			p.addiface=1;
			p.query=0;
		} else if ((strcmp(argv[currentarg],"-u")==0) || (strcmp(argv[currentarg],"--update")==0)) {
			printf("Error: The \"%s\" parameter is not supported in this version.\n", argv[currentarg]);
			return 1;
		} else if ((strcmp(argv[currentarg],"-q")==0) || (strcmp(argv[currentarg],"--query")==0)) {
			p.query=1;
		} else if ((strcmp(argv[currentarg],"-D")==0) || (strcmp(argv[currentarg],"--debug")==0)) {
			debug=1;
		} else if ((strcmp(argv[currentarg],"-d")==0) || (strcmp(argv[currentarg],"--days")==0)) {
			cfg.qmode=1;
			if (currentarg+1<argc && isdigit(argv[currentarg+1][0])) {
				cfg.listdays = atoi(argv[currentarg+1]);
				if (cfg.listdays < 0) {
					printf("Error: Invalid limit parameter \"%s\" for %s. Only a zero and positive numbers are allowed.\n", argv[currentarg+1], argv[currentarg]);
					return 1;
				}
				currentarg++;
			}
		} else if ((strcmp(argv[currentarg],"-m")==0) || (strcmp(argv[currentarg],"--months")==0)) {
			cfg.qmode=2;
			if (currentarg+1<argc && isdigit(argv[currentarg+1][0])) {
				cfg.listmonths = atoi(argv[currentarg+1]);
				if (cfg.listmonths < 0) {
					printf("Error: Invalid limit parameter \"%s\" for %s. Only a zero and positive numbers are allowed.\n", argv[currentarg+1], argv[currentarg]);
					return 1;
				}
				currentarg++;
			}
		} else if ((strcmp(argv[currentarg],"-t")==0) || (strcmp(argv[currentarg],"--top")==0)) {
			cfg.qmode=3;
			if (currentarg+1<argc && isdigit(argv[currentarg+1][0])) {
				cfg.listtop = atoi(argv[currentarg+1]);
				if (cfg.listtop < 0) {
					printf("Error: Invalid limit parameter \"%s\" for %s. Only a zero and positive numbers are allowed.\n", argv[currentarg+1], argv[currentarg]);
					return 1;
				}
				currentarg++;
			}
		} else if ((strcmp(argv[currentarg],"-s")==0) || (strcmp(argv[currentarg],"--short")==0)) {
			cfg.qmode=5;
		} else if ((strcmp(argv[currentarg],"-y")==0) || (strcmp(argv[currentarg],"--years")==0)) {
			cfg.qmode=6;
			if (currentarg+1<argc && isdigit(argv[currentarg+1][0])) {
				cfg.listyears = atoi(argv[currentarg+1]);
				if (cfg.listyears < 0) {
					printf("Error: Invalid limit parameter \"%s\" for %s. Only a zero and positive numbers are allowed.\n", argv[currentarg+1], argv[currentarg]);
					return 1;
				}
				currentarg++;
			}
		} else if ((strcmp(argv[currentarg],"-hg")==0) || (strcmp(argv[currentarg],"--hoursgraph")==0)) {
			cfg.qmode=7;
		} else if ((strcmp(argv[currentarg],"-h")==0) || (strcmp(argv[currentarg],"--hours")==0)) {
			cfg.qmode=11;
			if (currentarg+1<argc && isdigit(argv[currentarg+1][0])) {
				cfg.listhours = atoi(argv[currentarg+1]);
				if (cfg.listhours < 0) {
					printf("Error: Invalid limit parameter \"%s\" for %s. Only a zero and positive numbers are allowed.\n", argv[currentarg+1], argv[currentarg]);
					return 1;
				}
				currentarg++;
			}
		} else if ((strcmp(argv[currentarg],"-5")==0) || (strcmp(argv[currentarg],"--fiveminutes")==0)) {
			cfg.qmode=12;
			if (currentarg+1<argc && isdigit(argv[currentarg+1][0])) {
				cfg.listfivemins = atoi(argv[currentarg+1]);
				if (cfg.listfivemins < 0) {
					printf("Error: Invalid limit parameter \"%s\" for %s. Only a zero and positive numbers are allowed.\n", argv[currentarg+1], argv[currentarg]);
					return 1;
				}
				currentarg++;
			}
		} else if (strcmp(argv[currentarg],"--oneline")==0) {
			cfg.qmode=9;
			if (currentarg+1<argc && argv[currentarg+1][0]!='-') {
				if (argv[currentarg+1][0]=='b') {
					cfg.ostyle = 4;
					currentarg++;
				} else {
					printf("Error: Invalid mode parameter \"%s\" for --oneline.\n", argv[currentarg+1]);
					printf(" Valid parameters:\n");
					printf("    (none) - automatically scaled units visible\n");
					printf("    b      - all values in bytes\n");
					return 1;
				}
			}
		} else if (strcmp(argv[currentarg],"--xml")==0) {
			if (currentarg+1<argc && argv[currentarg+1][0]!='-' && !isdigit(argv[currentarg+1][0])) {
				p.xmlmode = argv[currentarg+1][0];
				if (strlen(argv[currentarg+1])!=1 || strchr("afhdmyt", p.xmlmode)==NULL) {
					printf("Error: Invalid mode parameter \"%s\" for --xml.\n", argv[currentarg+1]);
					printf(" Valid parameters:\n");
					printf("    a - all (default)\n");
					printf("    f - only 5 minutes\n");
					printf("    h - only hours\n");
					printf("    d - only days\n");
					printf("    m - only months\n");
					printf("    y - only years\n");
					printf("    t - only top\n");
					return 1;
				}
				currentarg++;
			}
			if (currentarg+1<argc && isdigit(argv[currentarg+1][0])) {
				cfg.listjsonxml = atoi(argv[currentarg+1]);
				if (cfg.listjsonxml < 0) {
					printf("Error: Invalid limit parameter \"%s\" for %s. Only a zero and positive numbers are allowed.\n", argv[currentarg+1], argv[currentarg]);
					return 1;
				}
				currentarg++;
			}
			cfg.qmode=8;
		} else if (strcmp(argv[currentarg],"--json")==0) {
			if (currentarg+1<argc && argv[currentarg+1][0]!='-' && !isdigit(argv[currentarg+1][0])) {
				p.jsonmode = argv[currentarg+1][0];
				if (strlen(argv[currentarg+1])!=1 || strchr("afhdmyt", p.jsonmode)==NULL) {
					printf("Error: Invalid mode parameter \"%s\" for --json.\n", argv[currentarg+1]);
					printf(" Valid parameters:\n");
					printf("    a - all (default)\n");
					printf("    f - only 5 minutes\n");
					printf("    h - only hours\n");
					printf("    d - only days\n");
					printf("    m - only months\n");
					printf("    y - only years\n");
					printf("    t - only top\n");
					return 1;
				}
				currentarg++;
			}
			if (currentarg+1<argc && isdigit(argv[currentarg+1][0])) {
				cfg.listjsonxml = atoi(argv[currentarg+1]);
				if (cfg.listjsonxml < 0) {
					printf("Error: Invalid limit parameter \"%s\" for %s. Only a zero and positive numbers are allowed.\n", argv[currentarg+1], argv[currentarg]);
					return 1;
				}
				currentarg++;
			}
			cfg.qmode=10;
		} else if ((strcmp(argv[currentarg],"-ru")==0) || (strcmp(argv[currentarg],"--rateunit"))==0) {
			if (currentarg+1<argc && isdigit(argv[currentarg+1][0])) {
				if (cfg.rateunit > 1 || cfg.rateunit < 0) {
					printf("Error: Invalid parameter \"%d\" for --rateunit.\n", cfg.rateunit);
					printf(" Valid parameters:\n");
					printf("    0 - bytes\n");
					printf("    1 - bits\n");
					return 1;
				}
				cfg.rateunit = atoi(argv[currentarg+1]);
				if (debug)
					printf("Rateunit changed: %d\n", cfg.rateunit);
				currentarg++;
				continue;
			} else {
				cfg.rateunit = !cfg.rateunit;
				if (debug)
					printf("Rateunit changed: %d\n", cfg.rateunit);
			}
		} else if ((strcmp(argv[currentarg],"-tr")==0) || (strcmp(argv[currentarg],"--traffic")==0)) {
			if (currentarg+1<argc && isdigit(argv[currentarg+1][0])) {
				cfg.sampletime=atoi(argv[currentarg+1]);
				currentarg++;
				p.traffic=1;
				p.query=0;
				continue;
			}
			p.traffic=1;
			p.query=0;
		} else if ((strcmp(argv[currentarg],"-l")==0) || (strcmp(argv[currentarg],"--live")==0)) {
			if (currentarg+1<argc && argv[currentarg+1][0]!='-') {
				if (!isdigit(argv[currentarg+1][0]) || p.livemode > 1 || p.livemode < 0) {
					printf("Error: Invalid mode parameter \"%s\" for -l / --live.\n", argv[currentarg+1]);
					printf(" Valid parameters:\n");
					printf("    0 - show packets per second (default)\n");
					printf("    1 - show transfer counters\n");
					return 1;
				}
				p.livemode = atoi(argv[currentarg+1]);
				currentarg++;
			}
			p.livetraffic=1;
			p.query=0;
		} else if (strcmp(argv[currentarg],"--force")==0) {
			p.force=1;
		} else if (strcmp(argv[currentarg],"--showconfig")==0) {
			printcfgfile();
			return 0;
		} else if (strcmp(argv[currentarg],"--remove")==0) {
			p.removeiface=1;
			p.query=0;
		} else if ((strcmp(argv[currentarg],"-b")==0) || (strcmp(argv[currentarg],"--begin")==0)) {
			if (currentarg+1<argc) {
				if (!validatedatetime(argv[currentarg+1])) {
					printf("Error: Invalid date format, expected YYYY-MM-DD HH:MM or YYYY-MM-DD.\n");
					return 1;
				}
				strncpy_nt(p.databegin, argv[currentarg+1], 18);
				currentarg++;
			} else {
				printf("Error: Date of format YYYY-MM-DD HH:MM or YYYY-MM-DD for %s missing.\n", argv[currentarg]);
				return 1;
			}
		} else if ((strcmp(argv[currentarg],"-e")==0) || (strcmp(argv[currentarg],"--end")==0)) {
			if (currentarg+1<argc) {
				if (!validatedatetime(argv[currentarg+1])) {
					printf("Error: Invalid date format, expected YYYY-MM-DD HH:MM or YYYY-MM-DD.\n");
					return 1;
				}
				strncpy_nt(p.dataend, argv[currentarg+1], 18);
				currentarg++;
			} else {
				printf("Error: Date of format YYYY-MM-DD HH:MM or YYYY-MM-DD for %s missing.\n", argv[currentarg]);
				return 1;
			}
		} else if (strcmp(argv[currentarg],"--iflist")==0) {
			getiflist(&p.ifacelist, 1);
			printf("Available interfaces: %s\n", p.ifacelist);
			free(p.ifacelist);
			return 0;
		} else if ((strcmp(argv[currentarg],"-v")==0) || (strcmp(argv[currentarg],"--version")==0)) {
			printf("vnStat %s by Teemu Toivola <tst at iki dot fi>\n", getversion());
			return 0;
		} else {
			printf("Unknown parameter \"%s\". Use --help for help.\n",argv[currentarg]);
			return 1;
		}

	}

	/* open database and see if it contains any interfaces */
	if (!p.traffic && !p.livetraffic) {
		if ((dir=opendir(p.dirname))!=NULL) {
			if (debug)
				printf("Dir OK\n");
			closedir(dir);
			strncpy_nt(cfg.dbdir, p.dirname, 512);
			if (!db_open_ro()) {
				printf("Error: Unable to open database \"%s/%s\": %s\n", p.dirname, DATABASEFILE, strerror(errno));
				if (errno == ENOENT) {
					printf("The vnStat daemon should have created the database when started.\n");
					printf("Check that it is configured and running. See also \"man vnstatd\".\n");
				}
				return 1;
			}
			p.ifcount = db_getinterfacecount();
			if (debug)
				printf("%"PRIu64" interface(s) found\n", p.ifcount);

			if (p.ifcount > 1) {
				strncpy_nt(p.definterface, cfg.iface, 32);
			}
		} else {
			printf("Error: Unable to open database directory \"%s\": %s\n", p.dirname, strerror(errno));
			if (errno==ENOENT) {
				printf("The vnStat daemon should have created this directory when started.\n");
				printf("Check that it is configured and running. See also \"man vnstatd\".\n");
			} else {
				printf("Make sure it is at least read enabled for current user.\n");
				printf("Use --help for help.\n");
			}
			return 1;
		}
	}

	/* set used interface if none specified and make sure it's null terminated */
	if (p.defaultiface) {
		strncpy_nt(p.interface, p.definterface, 32);

		/* ensure that some usable interface is default */
		if (p.ifcount > 0 && !db_getinterfacecountbyname(p.interface)) {
			if (db_getiflist(&dbifl) > 0) {
				strncpy_nt(p.interface, dbifl->interface, 32);
				if (debug) {
					printf("Using \"%s\" as interface, default \"%s\" not found in database.\n", p.interface, p.definterface);
				}
				dbiflistfree(&dbifl);
			}
		}
	}
	p.interface[31]='\0';

	/* parameter handlers */
	handleremoveinterface(&p);
	handleaddinterface(&p);
	handlesetalias(&p);
	handleshowdatabases(&p);
	handletrafficmeters(&p);

	/* show something if nothing was shown previously */
	if (!p.query && !p.traffic && !p.livetraffic) {

		/* give more help if there's no database */
		if (p.ifcount == 0) {
			getiflist(&p.ifacelist, 1);
			printf("No interfaces found in the database, nothing to do. Use --help for help.\n\n");
			printf("Interfaces can be added to the database with the following command:\n");
			printf("    %s --add -i eth0\n\n", argv[0]);
			printf("Replace 'eth0' with the interface that should be monitored.\n\n");
			if (strlen(cfg.cfgfile)) {
				printf("The default interface can be changed by updating the \"Interface\" keyword\n");
				printf("value in the configuration file \"%s\".\n\n", cfg.cfgfile);
			}
			printf("The following interfaces are currently available:\n    %s\n", p.ifacelist);
			free(p.ifacelist);
		} else {
			printf("Nothing to do. Use --help for help.\n");
		}
	}

	/* cleanup */
	ibwflush();
	db_close();

	return 0;
}

void initparams(PARAMS *p)
{
	noexit = 0; /* allow functions to exit in case of error */
	debug = 0; /* debug disabled by default */
	disableprints = 0; /* let prints be visible */

	p->addiface = 0;
	p->query = 1;
	p->setalias = 0;
	p->ifcount = 0;
	p->force = 0;
	p->traffic = 0;
	p->livetraffic = 0;
	p->defaultiface = 1;
	p->removeiface = 0;
	p->livemode = 0;
	p->ifacelist = NULL;
	p->cfgfile[0] = '\0';
	p->jsonmode = 'a';
	p->xmlmode = 'a';
	p->databegin[0] = '\0';
	p->dataend[0] = '\0';
}

void showhelp(PARAMS *p)
{
	printf("vnStat %s by Teemu Toivola <tst at iki dot fi>\n\n", getversion());

	printf("      -5,  --fiveminutes [count]   show 5 minutes\n");
	printf("      -h,  --hours [count]         show hours\n");
	printf("      -hg, --hoursgraph            show hours graph\n");
	printf("      -d,  --days [count]          show days\n");
	printf("      -m,  --months [count]        show months\n");
	printf("      -y,  --years [count]         show years\n");
	printf("      -t,  --top [count]           show top days\n\n");

	printf("      -b, --begin <date>           set list begin date\n");
	printf("      -e, --end <date>             set list end date\n\n");

	printf("      --oneline [mode]             show simple parsable format\n");
	printf("      --json [mode] [limit]        show database in json format\n");
	printf("      --xml [mode] [limit]         show database in xml format\n\n");

	printf("      -tr, --traffic [time]        calculate traffic\n");
	printf("      -l,  --live [mode]           show transfer rate in real time\n");
	printf("      -i,  --iface <interface>     select interface (default: %s)\n\n", p->definterface);

	printf("Use \"--longhelp\" or \"man vnstat\" for complete list of options.\n");
}

void showlonghelp(PARAMS *p)
{
	printf("vnStat %s by Teemu Toivola <tst at iki dot fi>\n\n", getversion());

	printf("Query:\n");

	printf("      -q,  --query                 query database\n");
	printf("      -s,  --short                 use short output\n");
	printf("      -5,  --fiveminutes [count]   show 5 minutes\n");
	printf("      -h,  --hours [count]         show hours\n");
	printf("      -hg, --hoursgraph            show hours graph\n");
	printf("      -d,  --days [count]          show days\n");
	printf("      -m,  --months [count]        show months\n");
	printf("      -y,  --years [count]         show years\n");
	printf("      -t,  --top [count]           show top days\n");
	printf("      -b,  --begin <date>          set list begin date\n");
	printf("      -e,  --end <date>            set list end date\n");
	printf("      --oneline [mode]             show simple parsable format\n");
	printf("      --json [mode] [limit]        show database in json format\n");
	printf("      --xml [mode] [limit]         show database in xml format\n\n");

	printf("Modify:\n");

	printf("      --add                        add interface to database\n");
	printf("      --remove                     remove interface from database\n");
	printf("      --setalias <alias>           set alias for interface\n\n");

	printf("Misc:\n");

	printf("      -i,  --iface <interface>     select interface (default: %s)\n", p->definterface);
	printf("      -?,  --help                  show short help\n");
	printf("      -D,  --debug                 show some additional debug information\n");
	printf("      -v,  --version               show version\n");
	printf("      -tr, --traffic [time]        calculate traffic\n");
	printf("      -l,  --live [mode]           show transfer rate in real time\n");
	printf("      -ru, --rateunit [mode]       swap configured rate unit\n");
	printf("      --style <mode>               select output style (0-4)\n");
	printf("      --iflist                     show list of available interfaces\n");
	printf("      --dbdir <directory>          select database directory\n");
	printf("      --locale <locale>            set locale\n");
	printf("      --config <config file>       select config file\n");
	printf("      --showconfig                 dump config file with current settings\n");
	printf("      --longhelp                   show this help\n\n");

	printf("See also \"man vnstat\" for longer descriptions of each option.\n");
}

void handleremoveinterface(PARAMS *p)
{
	if (!p->removeiface) {
		return;
	}

	if (!db_getinterfacecountbyname(p->interface)) {
		printf("Error: Interface \"%s\" not found in database.\n", p->interface);
		exit(EXIT_FAILURE);
	}

	if (!p->force) {
		printf("Warning:\nThe current option would remove all data\nabout interface \"%s\" from the database.\n", p->interface);
		printf("Use --force in order to really do that.\n");
		exit(EXIT_FAILURE);
	}

	if (!db_close() || !db_open_rw(0)) {
		printf("Error: Handling database \"%s/%s\" failing: %s\n", p->dirname, DATABASEFILE, strerror(errno));
		exit(EXIT_FAILURE);
	}

	if (db_removeinterface(p->interface)) {
		printf("Interface \"%s\" removed from database.\n", p->interface);
		printf("The interface will no longer be monitored. Use --add\n");
		printf("if monitoring the interface is again needed.\n");
		exit(EXIT_SUCCESS);
	} else {
		printf("Error: Removing interface \"%s\" from database failed.\n", p->interface);
		exit(EXIT_FAILURE);
	}
}

void handleaddinterface(PARAMS *p)
{
	if (!p->addiface) {
		return;
	}

	if (p->defaultiface) {
		printf("Error: Use -i parameter to specify an interface.\n");
		exit(EXIT_FAILURE);
	}

	db_errcode = 0;
	if (db_getinterfacecountbyname(p->interface)) {
		printf("Error: Interface \"%s\" already exists in the database.\n", p->interface);
		exit(EXIT_FAILURE);
	}
	if (db_errcode) {
		exit(EXIT_FAILURE);
	}

	if (!p->force && !getifinfo(p->interface)) {
		getiflist(&p->ifacelist, 1);
		printf("Only available interfaces can be added for monitoring.\n\n");
		printf("The following interfaces are currently available:\n    %s\n", p->ifacelist);
		free(p->ifacelist);
		exit(EXIT_FAILURE);
	}

	if (!p->force && !spacecheck(p->dirname)) {
		printf("Error: Not enough free diskspace available.\n");
		exit(EXIT_FAILURE);
	}

	if (!db_close() || !db_open_rw(0)) {
		printf("Error: Handling database \"%s/%s\" failing: %s\n", p->dirname, DATABASEFILE, strerror(errno));
		exit(EXIT_FAILURE);
	}

	printf("Adding interface \"%s\" for monitoring to database...\n", p->interface);
	if (db_addinterface(p->interface)) {
		printf("\nRestart the vnStat daemon if it is currently running in order to start monitoring \"%s\".\n", p->interface);
		exit(EXIT_SUCCESS);
	} else {
		printf("Error: Adding interface \"%s\" to database failed: %s\n", p->interface, strerror(errno));
		exit(EXIT_FAILURE);
	}
}

void handlesetalias(PARAMS *p)
{
	if (!p->setalias) {
		return;
	}

	if (p->defaultiface) {
		printf("Error: Use -i parameter to specify an interface.\n");
		exit(EXIT_FAILURE);
	}

	if (!db_getinterfacecountbyname(p->interface)) {
		printf("Error: Interface \"%s\" not found in database.\n", p->interface);
		exit(EXIT_FAILURE);
	}

	if (!db_close() || !db_open_rw(0)) {
		printf("Error: Handling database \"%s/%s\" failing: %s\n", p->dirname, DATABASEFILE, strerror(errno));
		exit(EXIT_FAILURE);
	}

	if (db_setalias(p->interface, p->alias)) {
		printf("Alias of interface \"%s\" set to \"%s\".\n", p->interface, p->alias);
		exit(EXIT_SUCCESS);
	} else {
		printf("Error: Changing interface \"%s\" alias failed: %s\n", p->interface, strerror(errno));
		exit(EXIT_FAILURE);
	}
}

void handleshowdatabases(PARAMS *p)
{
	int ifcount = 0;
	dbiflist *dbifl = NULL, *dbifl_i = NULL;

	if (!p->query) {
		return;
	}

	/* show only specified file */
	if (!p->defaultiface) {
		showoneinterface(p, p->interface);
		return;
	}

	/* show all interfaces if -i isn't specified */
	if (p->ifcount==0) {
		/* printf("No database found.\n"); */
		p->query=0;
	} else if ((cfg.qmode==0 || cfg.qmode==8 || cfg.qmode==10) && (p->ifcount>1)) {

		if (cfg.qmode==0) {
			if (cfg.ostyle!=0) {
				printf("\n                      rx      /      tx      /     total    /   estimated\n");
			} else {
				printf("\n                      rx      /      tx      /     total\n");
			}
		} else if (cfg.qmode==8) {
			xmlheader();
		} else if (cfg.qmode==10) {
			jsonheader();
		}

		if (db_getiflist(&dbifl) <= 0) {
			return;
		}

		dbifl_i = dbifl;

		while (dbifl_i != NULL) {
			strncpy_nt(p->interface, dbifl_i->interface, 32);
			if (debug)
				printf("\nProcessing interface \"%s\"...\n", p->interface);
			if (cfg.qmode==0) {
				showdb(p->interface, 5, "", "");
			} else if (cfg.qmode==8) {
				showxml(p->interface, p->xmlmode, p->databegin, p->dataend);
			} else if (cfg.qmode==10) {
				showjson(p->interface, ifcount, p->jsonmode, p->databegin, p->dataend);
			}
			ifcount++;
			dbifl_i = dbifl_i->next;
		}
		dbiflistfree(&dbifl);

		if (cfg.qmode==8) {
			xmlfooter();
		} else if (cfg.qmode==10) {
			jsonfooter();
		}

	/* show in qmode if there's only one file or qmode!=0 */
	} else {
		showoneinterface(p, p->interface);
	}
}

void showoneinterface(PARAMS *p, const char *interface)
{
	if (!db_getinterfacecountbyname(p->interface)) {
		if (strchr(p->interface, '+') == NULL)  {
			printf("Error: Interface \"%s\" not found in database.\n", p->interface);
		} else {
			printf("Error: Not all requested interfaces found in database.\n");
		}
		exit(EXIT_FAILURE);
	}

	if (cfg.qmode==5) {
		if (cfg.ostyle!=0) {
			printf("\n                      rx      /      tx      /     total    /   estimated\n");
		} else {
			printf("\n                      rx      /      tx      /     total\n");
		}
	}
	if (cfg.qmode!=8 && cfg.qmode!=10) {
		showdb(interface, cfg.qmode, p->databegin, p->dataend);
	} else if (cfg.qmode==8) {
		xmlheader();
		showxml(p->interface, p->xmlmode, p->databegin, p->dataend);
		xmlfooter();
	} else if (cfg.qmode==10) {
		jsonheader();
		showjson(p->interface, 0, p->jsonmode, p->databegin, p->dataend);
		jsonfooter();
	}
}

void handletrafficmeters(PARAMS *p)
{
	if (!p->traffic && !p->livetraffic) {
		return;
	}

	if (!isifavailable(p->interface)) {
		getiflist(&p->ifacelist, 0);
		if (p->defaultiface) {
			printf("Error: Configured default interface \"%s\" isn't available.\n\n", p->interface);
			if (strlen(cfg.cfgfile)) {
				printf("Update \"Interface\" keyword value in \"%s\" to change\n", cfg.cfgfile);
				printf("the default interface or give an alternative interface\nusing the -i parameter.\n\n");
			} else {
				printf("An alternative interface can be given using the -i parameter.\n\n");
			}
			printf("The following interfaces are currently available:\n    %s\n", p->ifacelist);

		} else {
			printf("Error: Unable to get interface \"%s\" statistics.\n\n", p->interface);
			printf("The following interfaces are currently available:\n    %s\n", p->ifacelist);
		}
		free(p->ifacelist);
		exit(EXIT_FAILURE);
	}

	/* calculate traffic */
	if (p->traffic) {
		trafficmeter(p->interface, (unsigned int)cfg.sampletime);
	}

	/* live traffic */
	if (p->livetraffic) {
		livetrafficmeter(p->interface, p->livemode);
	}
}
