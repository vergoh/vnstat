/*
vnStat - Copyright (c) 2002-2016 Teemu Toivola <tst@iki.fi>

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
//#include "dbxml.h"
//#include "dbjson.h"
#include "dbshow.h"
#include "misc.h"
#include "cfg.h"
#include "ibw.h"
#include "fs.h"
#include "vnstat.h"

int main(int argc, char *argv[]) {

	int i, currentarg;
	DIR *dir = NULL;
	PARAMS p;
	dbiflist *dbifl = NULL;

	initparams(&p);

	/* early check for debug and config parameter */
	if (argc > 1) {
		for (currentarg=1; currentarg<argc; currentarg++) {
			if ((strcmp(argv[currentarg],"-D")==0) || (strcmp(argv[currentarg],"--debug")==0)) {
				debug = 1;
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
		} else if (strcmp(argv[currentarg],"--create")==0) {
			p.create=1;
			p.query=0;
		} else if ((strcmp(argv[currentarg],"-u")==0) || (strcmp(argv[currentarg],"--update")==0)) {
			printf("Error: The \"%s\" parameter is not supported in this version.\n", argv[currentarg]);
			exit(EXIT_FAILURE);
		} else if ((strcmp(argv[currentarg],"-q")==0) || (strcmp(argv[currentarg],"--query")==0)) {
			p.query=1;
		} else if ((strcmp(argv[currentarg],"-D")==0) || (strcmp(argv[currentarg],"--debug")==0)) {
			debug=1;
		} else if ((strcmp(argv[currentarg],"-d")==0) || (strcmp(argv[currentarg],"--days")==0)) {
			cfg.qmode=1;
		} else if ((strcmp(argv[currentarg],"-m")==0) || (strcmp(argv[currentarg],"--months")==0)) {
			cfg.qmode=2;
		} else if ((strcmp(argv[currentarg],"-t")==0) || (strcmp(argv[currentarg],"--top10")==0)) {
			cfg.qmode=3;
		} else if ((strcmp(argv[currentarg],"-s")==0) || (strcmp(argv[currentarg],"--short")==0)) {
			cfg.qmode=5;
		} else if ((strcmp(argv[currentarg],"-h")==0) || (strcmp(argv[currentarg],"--hours")==0)) {
			cfg.qmode=7;
/*		} else if ((strcmp(argv[currentarg],"--exportdb")==0) || (strcmp(argv[currentarg],"--dumpdb")==0)) {
			cfg.qmode=4; */
		} else if (strcmp(argv[currentarg],"--oneline")==0) {
			cfg.qmode=9;
/*		} else if (strcmp(argv[currentarg],"--xml")==0) {
			if (currentarg+1<argc && argv[currentarg+1][0]!='-') {
				p.xmlmode = argv[currentarg+1][0];
				if (strlen(argv[currentarg+1])!=1 || strchr("ahdmt", p.xmlmode)==NULL) {
					printf("Error: Invalid mode parameter \"%s\" for --xml.\n", argv[currentarg+1]);
					printf(" Valid parameters:\n");
					printf("    a - all (default)\n");
					printf("    h - only hours\n");
					printf("    d - only days\n");
					printf("    m - only months\n");
					printf("    t - only top 10\n");
					return 1;
				}
				currentarg++;
			}
			cfg.qmode=8;
		} else if (strcmp(argv[currentarg],"--json")==0) {
			if (currentarg+1<argc && argv[currentarg+1][0]!='-') {
				p.jsonmode = argv[currentarg+1][0];
				if (strlen(argv[currentarg+1])!=1 || strchr("ahdmt", p.jsonmode)==NULL) {
					printf("Error: Invalid mode parameter \"%s\" for --json.\n", argv[currentarg+1]);
					printf(" Valid parameters:\n");
					printf("    a - all (default)\n");
					printf("    h - only hours\n");
					printf("    d - only days\n");
					printf("    m - only months\n");
					printf("    t - only top 10\n");
					return 1;
				}
				currentarg++;
			}
			cfg.qmode=10;
*/		} else if ((strcmp(argv[currentarg],"-ru")==0) || (strcmp(argv[currentarg],"--rateunit"))==0) {
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
		} else if (strcmp(argv[currentarg],"--delete")==0) {
			p.delete=1;
			p.query=0;
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
			if (!db_open(0)) {
				if (errno != ENOENT) {
					printf("Error: Unable to open database \"%s/%s\": %s\n", p.dirname, DATABASEFILE, strerror(errno));
					return 1;
				} else {
					p.query = 0;
				}
			}
			p.ifcount = db_getinterfacecount();
			if (debug)
				printf("%d interface(s) found\n", p.ifcount);

			if (p.ifcount > 1) {
				strncpy_nt(p.definterface, cfg.iface, 32);
			}
		} else {
			printf("Error: Unable to open database directory \"%s\": %s\n", p.dirname, strerror(errno));
			if (errno==ENOENT) {
				printf("The vnStat daemon should have created this directory when started.\n");
				printf("Check that it is is configured and running. See also \"man vnstatd\".\n");
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
			if (db_getiflist(&dbifl)) {
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
	handledelete(&p);
	handlecreate(&p);
	handlesetalias(&p);
	handleshowdatabases(&p);
	handletrafficmeters(&p);

	/* show something if nothing was shown previously */
	if (!p.query && !p.traffic && !p.livetraffic) {

		/* give more help if there's no database */
		if (p.ifcount == 0) {
			getiflist(&p.ifacelist, 1);
			printf("No database found, nothing to do. Use --help for help.\n\n");
			printf("A new database can be created with the following command:\n");
			printf("    %s --create -i eth0\n\n", argv[0]);
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

	p->create = 0;
	p->query = 1;
	p->setalias = 0;
	p->ifcount = 0;
	p->force = 0;
	p->traffic = 0;
	p->livetraffic = 0;
	p->defaultiface = 1;
	p->delete=0;
	p->livemode = 0;
	p->ifacelist = NULL;
	p->cfgfile[0] = '\0';
	p->jsonmode = 'a';
	p->xmlmode = 'a';
}

void showhelp(PARAMS *p)
{
	printf(" vnStat %s by Teemu Toivola <tst at iki dot fi>\n\n", getversion());

	printf("         -q,  --query          query database\n");
	printf("         -h,  --hours          show hours\n");
	printf("         -d,  --days           show days\n");
	printf("         -m,  --months         show months\n");
	printf("         -t,  --top10          show top 10 days\n");
	printf("         -s,  --short          use short output\n");
	printf("         -i,  --iface          select interface (default: %s)\n", p->definterface);
	printf("         -?,  --help           short help\n");
	printf("         -v,  --version        show version\n");
	printf("         -tr, --traffic        calculate traffic\n");
	printf("         -ru, --rateunit       swap configured rate unit\n");
	printf("         -l,  --live           show transfer rate in real time\n\n");

	printf("See also \"--longhelp\" for complete options list and \"man vnstat\".\n");
}

void showlonghelp(PARAMS *p)
{
	/* TODO: update */
	/* --create could be replaced with --add as it adds the interface to the database */
	/* --delete could be similarly replaced with --remove */

	printf(" vnStat %s by Teemu Toivola <tst at iki dot fi>\n\n", getversion());

	printf("   Query:\n");
	printf("         -q, --query           query database\n");
	printf("         -h, --hours           show hours\n");
	printf("         -d, --days            show days\n");
	printf("         -m, --months          show months\n");
	printf("         -t, --top10           show top 10 days\n");
	printf("         -s, --short           use short output\n");
	printf("         -ru, --rateunit       swap configured rate unit\n");
	printf("         --oneline             show simple parseable format\n");
	//printf("         --exportdb            dump database in text format\n");
	//printf("         --json                show database in json format\n");
	//printf("         --xml                 show database in xml format\n");

	printf("   Modify:\n");
	printf("         --create              create database\n");
	printf("         --delete              delete database\n");
	printf("         --setalias            set alias for interface\n");

	printf("   Misc:\n");
	printf("         -i,  --iface          select interface (default: %s)\n", p->definterface);
	printf("         -?,  --help           short help\n");
	printf("         -D,  --debug          show some additional debug information\n");
	printf("         -v,  --version        show version\n");
	printf("         -tr, --traffic        calculate traffic\n");
	printf("         -l,  --live           show transfer rate in real time\n");
	printf("         --style               select output style (0-4)\n");
	printf("         --iflist              show list of available interfaces\n");
	printf("         --dbdir               select database directory\n");
	printf("         --locale              set locale\n");
	printf("         --config              select config file\n");
	printf("         --showconfig          dump config file with current settings\n");
	printf("         --longhelp            display this help\n\n");

	printf("See also \"man vnstat\".\n");
}

void handledelete(PARAMS *p)
{
	if (!p->delete) {
		return;
	}

	if (!db_getinterfacecountbyname(p->interface)) {
		printf("Error: Interface \"%s\" not found in database.\n", p->interface);
		exit(EXIT_FAILURE);
	}

	if (!p->force) {
		printf("Warning:\nThe current option would delete all data\nabout interface \"%s\" from the database.\n", p->interface);
		printf("Use --force in order to really do that.\n");
		exit(EXIT_FAILURE);
	}

	if (db_removeinterface(p->interface)) {
		printf("Interface \"%s\" deleted from database.\n", p->interface);
		printf("The interface will no longer be monitored. Use --create\n");
		printf("if monitoring the interface is again needed.\n");
		exit(EXIT_SUCCESS);
	} else {
		printf("Error: Deleting interface \"%s\" from database failed.\n", p->interface);
		exit(EXIT_FAILURE);
	}
}

void handlecreate(PARAMS *p)
{
	char dbfile[512];

	if (!p->create) {
		return;
	}

	if (p->defaultiface) {
		printf("Error: Use -i parameter to specify an interface.\n");
		exit(EXIT_FAILURE);
	}

	if (db_getinterfacecountbyname(p->interface)) {
		printf("Error: Interface \"%s\" already exists in the database.\n", p->interface);
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

	snprintf(dbfile, 512, "%s/%s", p->dirname, DATABASEFILE);
	if (!fileexists(dbfile)) {
		/* database file doesn't exist so it can't be open either, try to create it */
		printf("Database doesn't exist, creating...\n");
		if (!db_open(1)) {
			if (errno == ENOENT) {
				printf("Error: Unable to create database, verify existence and write access of \"%s\".\n", p->dirname);
			} else {
				printf("Error: Unable to create database \"%s/%s\": %s\n", p->dirname, DATABASEFILE, strerror(errno));
			}
			exit(EXIT_FAILURE);
		}
		/* do file ownwership fixing if possible and needed */
		matchdbownerwithdirowner(p->dirname);
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
	DIR *dir = NULL;
	struct dirent *di = NULL;
	int dbcount = 0;

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
/*		} else if (cfg.qmode==8) {
			xmlheader();
		} else if (cfg.qmode==10) {
			jsonheader();
*/		}
		/* TODO: fetch the interface list from database */
		if ((dir=opendir(p->dirname))==NULL) {
			return;
		}
		while ((di=readdir(dir))) {
			if ((di->d_name[0]=='.') || (strcmp(di->d_name, DATABASEFILE)==0)) {
				continue;
			}
			strncpy_nt(p->interface, di->d_name, 32);
			if (debug)
				printf("\nProcessing file \"%s/%s\"...\n", p->dirname, p->interface);
/*			p->newdb=readdb(p->interface, p->dirname, p->force);
			if (p->newdb) {
				continue;
			}*/
			if (cfg.qmode==0) {
				/* TODO: most outputs missing */
				printf(" == query not implemented ==\n");
				//showdb(5);
/*			} else if (cfg.qmode==8) {
				showxml(p->xmlmode);
			} else if (cfg.qmode==10) {
				showjson(dbcount, p->jsonmode);
*/			}
			dbcount++;
		}
		closedir(dir);
/*		if (cfg.qmode==8) {
			xmlfooter();
		} else if (cfg.qmode==10) {
			jsonfooter();
		}
*/
	/* show in qmode if there's only one file or qmode!=0 */
	} else {
		showoneinterface(p, p->interface);
	}
}

void showoneinterface(PARAMS *p, const char *interface)
{
	if (!db_getinterfacecountbyname(p->interface)) {
		p->query = 0;
		return;
	}

	if (cfg.qmode==5) {
		if (cfg.ostyle!=0) {
			printf("\n                      rx      /      tx      /     total    /   estimated\n");
		} else {
			printf("\n                      rx      /      tx      /     total\n");
		}
	}
	if (cfg.qmode!=8 && cfg.qmode!=10) {
		/* TODO: xml ja json missing */
		showdb(interface, cfg.qmode);
/*	} else if (cfg.qmode==8) {
		xmlheader();
		showxml(p->xmlmode);
		xmlfooter();
	} else if (cfg.qmode==10) {
		jsonheader();
		showjson(0, p->jsonmode);
		jsonfooter();
*/	}
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
		trafficmeter(p->interface, cfg.sampletime);
	}

	/* live traffic */
	if (p->livetraffic) {
		livetrafficmeter(p->interface, p->livemode);
	}
}
