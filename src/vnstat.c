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
#include "dbxml.h"
#include "dbjson.h"
#include "dbshow.h"
#include "dbaccess.h"
#include "dbmerge.h"
#include "misc.h"
#include "cfg.h"
#include "ibw.h"
#include "vnstat.h"

int main(int argc, char *argv[]) {

	int i, currentarg;
	DIR *dir = NULL;
	struct dirent *di = NULL;
	PARAMS p;

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
	strncpy_nt(p.nick, "none", 32);

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
				printf("Error: Interface for -i missing.\n");
				return 1;
			}
		} else if (strcmp(argv[currentarg],"--config")==0) {
			/* config has already been parsed earlier so nothing to do here */
			currentarg++;
			continue;
		} else if ((strcmp(argv[currentarg],"--nick"))==0) {
			if (currentarg+1<argc) {
				strncpy_nt(p.nick, argv[currentarg+1], 32);
				if (debug)
					printf("Used nick: %s\n", p.nick);
				currentarg++;
				continue;
			} else {
				printf("Error: Nick for --nick missing.\n");
				return 1;
			}
		} else if ((strcmp(argv[currentarg],"--style"))==0) {
			if (currentarg+1<argc && isdigit(argv[currentarg+1][0])) {
				if (cfg.ostyle > 4 || cfg.ostyle < 0) {
					printf("Error: Invalid style parameter \"%d\" for --style.\n", cfg.ostyle);
					printf(" Valid parameters:\n");
					printf("    0 - a more narrow output\n");
					printf("    1 - enable bar column if available\n");
					printf("    2 - average traffic rate in summary and weekly outputs\n");
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
				printf("    2 - average traffic rate in summary and weekly outputs\n");
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
				printf("Error: Directory for --dbdir missing.\n");
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
				printf("Error: Locale for --locale missing.\n");
				return 1;
			}
		} else if (strcmp(argv[currentarg],"--create")==0) {
			p.create=1;
			p.query=0;
			if (debug)
				printf("Creating database...\n");
		} else if ((strcmp(argv[currentarg],"-u")==0) || (strcmp(argv[currentarg],"--update")==0)) {
			p.update=1;
			p.query=0;
			if (debug)
				printf("Updating database...\n");
		} else if (strcmp(argv[currentarg],"--importdb")==0) {
			if (currentarg+1<argc) {
				p.import=1;
				strncpy_nt(p.filename, argv[currentarg+1], 512);
				if (debug)
					printf("Used import file: %s\n", p.filename);
				currentarg++;
				continue;
			} else {
				printf("Error: File parameter for --importdb missing.\n");
				return 1;
			}
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
		} else if ((strcmp(argv[currentarg],"-w")==0) || (strcmp(argv[currentarg],"--weeks")==0)) {
			cfg.qmode=6;
		} else if ((strcmp(argv[currentarg],"-h")==0) || (strcmp(argv[currentarg],"--hours")==0)) {
			cfg.qmode=7;
		} else if ((strcmp(argv[currentarg],"--exportdb")==0) || (strcmp(argv[currentarg],"--dumpdb")==0)) {
			cfg.qmode=4;
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
		} else if (strcmp(argv[currentarg],"--savemerged")==0) {
			p.savemerged=1;
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
		} else if (strcmp(argv[currentarg],"--enable")==0) {
			p.active=1;
			p.query=0;
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
		} else if (strcmp(argv[currentarg],"--cleartop")==0) {
			p.cleartop=1;
		} else if (strcmp(argv[currentarg],"--rebuildtotal")==0) {
			p.rebuildtotal=1;
		} else if (strcmp(argv[currentarg],"--disable")==0) {
			p.active=0;
			p.query=0;
		} else if (strcmp(argv[currentarg],"--testkernel")==0) {
			i=kerneltest();
			return i;
		} else if (strcmp(argv[currentarg],"--showconfig")==0) {
			printcfgfile();
			return 0;
		} else if (strcmp(argv[currentarg],"--delete")==0) {
			p.del=1;
			p.query=0;
		} else if (strcmp(argv[currentarg],"--iflist")==0) {
			getiflist(&p.ifacelist, 1);
			printf("Available interfaces: %s\n", p.ifacelist);
			free(p.ifacelist);
			return 0;
		} else if ((strcmp(argv[currentarg],"-v")==0) || (strcmp(argv[currentarg],"--version")==0)) {
			printf("vnStat %s by Teemu Toivola <tst at iki dot fi>\n", getversion());
			return 0;
		} else if ((strcmp(argv[currentarg],"-r")==0) || (strcmp(argv[currentarg],"--reset")==0)) {
			p.reset=1;
			p.query=0;
		} else if (strcmp(argv[currentarg],"--sync")==0) {
			p.sync=1;
			p.query=0;
		} else {
			printf("Unknown parameter \"%s\". Use --help for help.\n",argv[currentarg]);
			return 1;
		}

	}

	/* check if the database dir exists and if it contains files */
	if (!p.traffic && !p.livetraffic) {
		if ((dir=opendir(p.dirname))!=NULL) {
			if (debug)
				printf("Dir OK\n");
			while ((di=readdir(dir))) {
				if ((di->d_name[0]=='.') || (strcmp(di->d_name, DATABASEFILE)==0)) {
					continue;
				}
				strncpy_nt(p.definterface, di->d_name, 32);
				p.files++;
			}
			if (debug)
				printf("%d file(s) found\n", p.files);
			if (p.files>1) {
				strncpy_nt(p.definterface, cfg.iface, 32);
			}
			closedir(dir);
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
	}
	p.interface[31]='\0';

	/* parameter handlers */
	handledbmerge(&p);
	handlecounterreset(&p);
	handleimport(&p);
	handlecountersync(&p);
	handledelete(&p);
	handlecleartop10(&p);
	handlerebuildtotal(&p);
	handleenabledisable(&p);
	handlecreate(&p);
	handleupdate(&p);
	handleshowdatabases(&p);
	handletrafficmeters(&p);

	/* show something if nothing was shown previously */
	if (!p.query && !p.update && !p.create && !p.reset && !p.sync && p.active==-1 && !p.cleartop && !p.rebuildtotal && !p.traffic && !p.livetraffic) {

		/* give more help if there's no database */
		if (p.files==0) {
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

	return 0;
}

void initparams(PARAMS *p)
{
	noexit = 0; /* allow functions to exit in case of error */
	debug = 0; /* debug disabled by default */
	disableprints = 0; /* let prints be visible */

	p->create = 0;
	p->update = 0;
	p->query = 1;
	p->newdb = 0;
	p->reset = 0;
	p->sync = 0;
	p->merged = 0;
	p->savemerged = 0;
	p->import = 0;
	p->active = -1;
	p->files = 0;
	p->force = 0;
	p->cleartop = 0;
	p->rebuildtotal = 0;
	p->traffic = 0;
	p->livetraffic = 0;
	p->defaultiface = 1;
	p->del=0;
	p->livemode = 0;
	p->ifacelist = NULL;
	p->cfgfile[0] = '\0';
	p->jsonmode = 'a';
	p->xmlmode = 'a';
}

int synccounters(const char *iface, const char *dirname)
{
	readdb(iface, dirname, 0);
	if (!getifinfo(iface)) {
		printf("Error: Unable to sync unavailable interface \"%s\".", iface);
		return 0;
	}

	/* set counters to current without counting traffic */
	data.currx = ifinfo.rx;
	data.curtx = ifinfo.tx;

	writedb(iface, dirname, 0);
	return 1;
}

void showhelp(PARAMS *p)
{
	printf(" vnStat %s by Teemu Toivola <tst at iki dot fi>\n\n", getversion());

	printf("         -q,  --query          query database\n");
	printf("         -h,  --hours          show hours\n");
	printf("         -d,  --days           show days\n");
	printf("         -m,  --months         show months\n");
	printf("         -w,  --weeks          show weeks\n");
	printf("         -t,  --top10          show top 10 days\n");
	printf("         -s,  --short          use short output\n");
	printf("         -u,  --update         update database\n");
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
	printf(" vnStat %s by Teemu Toivola <tst at iki dot fi>\n\n", getversion());

	printf("   Query:\n");
	printf("         -q, --query           query database\n");
	printf("         -h, --hours           show hours\n");
	printf("         -d, --days            show days\n");
	printf("         -m, --months          show months\n");
	printf("         -w, --weeks           show weeks\n");
	printf("         -t, --top10           show top 10 days\n");
	printf("         -s, --short           use short output\n");
	printf("         -ru, --rateunit       swap configured rate unit\n");
	printf("         --oneline             show simple parseable format\n");
	printf("         --exportdb            dump database in text format\n");
	printf("         --importdb            import previously exported database\n");
	printf("         --json                show database in json format\n");
	printf("         --xml                 show database in xml format\n");

	printf("   Modify:\n");
	printf("         --create              create database\n");
	printf("         --delete              delete database\n");
	printf("         -u, --update          update database\n");
	printf("         -r, --reset           reset interface counters\n");
	printf("         --sync                sync interface counters\n");
	printf("         --enable              enable interface\n");
	printf("         --disable             disable interface\n");
	printf("         --nick                set a nickname for interface\n");
	printf("         --cleartop            clear the top 10\n");
	printf("         --rebuildtotal        rebuild total transfers from months\n");

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
	printf("         --savemerged          save merged database to current directory\n");
	printf("         --showconfig          dump config file with current settings\n");
	printf("         --testkernel          check if the kernel is broken\n");
	printf("         --longhelp            display this help\n\n");

	printf("See also \"man vnstat\".\n");
}

void handledbmerge(PARAMS *p)
{
	/* db merge */
	if (p->query && strstr(p->interface, "+")) {
		if (mergedb(p->interface, p->dirname)) {
			p->files = p->merged = 1;
		} else {
			exit(EXIT_FAILURE);
		}
	}

	/* save merged database */
	if (p->merged && p->savemerged) {
		data.lastupdated = 0;
		if (writedb("mergeddb", ".", 2)) {
			printf("Database saved as \"mergeddb\" in the current directory.\n");
		}
		exit(EXIT_SUCCESS);
	}
}

void handlecounterreset(PARAMS *p)
{
	if (!p->reset) {
		return;
	}

	if (!p->force && !spacecheck(p->dirname)) {
		printf("Error: Not enough free diskspace available.\n");
		exit(EXIT_FAILURE);
	}
	readdb(p->interface, p->dirname, 0);
	data.currx=0;
	data.curtx=0;
	writedb(p->interface, p->dirname, 0);
	if (debug)
		printf("Counters reset for \"%s\"\n", data.interface);
}

void handleimport(PARAMS *p)
{
	if (!p->import) {
		return;
	}

	if (p->defaultiface) {
		printf("Error: Specify interface to be imported using the -i parameter.\n");
		exit(EXIT_FAILURE);
	}
	if (!p->force && !spacecheck(p->dirname)) {
		printf("Error: Not enough free diskspace available.\n");
		exit(EXIT_FAILURE);
	}
	if (!p->force && checkdb(p->interface, p->dirname)) {
		printf("Error: Database file for interface \"%s\" already exists.\n", p->interface);
		printf("Add --force parameter to overwrite it.\n");
		exit(EXIT_FAILURE);
	}
	initdb();
	if (!importdb(p->filename)) {
		exit(EXIT_FAILURE);
	}
	if (!validatedb()) {
		printf("Error: validation of imported database failed.\n");
		exit(EXIT_FAILURE);
	}
	/* set boot time to one in order to force counter sync */
	data.btime = 1;
	strncpy_nt(data.interface, p->interface, 32);
	if (writedb(p->interface, p->dirname, 1)) {
		printf("Database import for \"%s\" completed.\n", data.interface);
	}
	exit(EXIT_SUCCESS);
}

void handlecountersync(PARAMS *p)
{
	if (!p->sync) {
		return;
	}

	if (!p->force && !spacecheck(p->dirname)) {
		printf("Error: Not enough free diskspace available.\n");
		exit(EXIT_FAILURE);
	}
	if (!synccounters(p->interface, p->dirname)) {
		exit(EXIT_FAILURE);
	}
	if (debug)
		printf("Counters synced for \"%s\"\n", data.interface);
}

void handledelete(PARAMS *p)
{
	if (!p->del) {
		return;
	}

	if (!p->force) {
		printf("Warning:\nThe current option would delete the database for \"%s\".\n", p->interface);
		printf("Use --force in order to really do that.\n");
		exit(EXIT_FAILURE);
	}

	if (checkdb(p->interface, p->dirname)) {
		if (removedb(p->interface, p->dirname)) {
			printf("Database for interface \"%s\" deleted.\n", p->interface);
			printf("The interface will no longer be monitored. Use --create\n");
			printf("if monitoring the interface is again needed.\n");
			exit(EXIT_SUCCESS);
		} else {
			printf("Error: Deleting database for interface \"%s\" failed.\n", p->interface);
			exit(EXIT_FAILURE);
		}
	} else {
			printf("Error: No database found for interface \"%s\".\n", p->interface);
			exit(EXIT_FAILURE);
	}
}

void handlecleartop10(PARAMS *p)
{
	if (!p->cleartop) {
		return;
	}

	if (!p->force && !spacecheck(p->dirname)) {
		printf("Error: Not enough free diskspace available.\n");
		exit(EXIT_FAILURE);
	}
	if (p->force) {
		cleartop10(p->interface, p->dirname);
		p->query=0;
	} else {
		printf("Warning:\nThe current option would clear the top 10 for \"%s\".\n", p->interface);
		printf("Use --force in order to really do that.\n");
		exit(EXIT_FAILURE);
	}
}

void handlerebuildtotal(PARAMS *p)
{
	if (!p->rebuildtotal) {
		return;
	}

	if (!spacecheck(p->dirname)) {
		printf("Error: Not enough free diskspace available.\n");
		exit(EXIT_FAILURE);
	}
	if (p->force) {
		rebuilddbtotal(p->interface, p->dirname);
		p->query=0;
	} else {
		printf("Warning:\nThe current option would rebuild total transfers for \"%s\".\n", p->interface);
		printf("Use --force in order to really do that.\n");
		exit(EXIT_FAILURE);
	}
}

void handleenabledisable(PARAMS *p)
{
	/* enable & disable */
	if (p->active==1) {
		if (!p->force && !spacecheck(p->dirname)) {
			printf("Error: Not enough free diskspace available.\n");
			exit(EXIT_FAILURE);
		}
		p->newdb=readdb(p->interface, p->dirname, 0);
		if (!data.active && !p->newdb) {
			data.active=1;
			writedb(p->interface, p->dirname, 0);
			if (debug)
				printf("Interface \"%s\" enabled.\n", data.interface);
		} else if (!p->newdb) {
			printf("Interface \"%s\" is already enabled.\n", data.interface);
		}
	} else if (p->active==0) {
		if (!p->force && !spacecheck(p->dirname)) {
			printf("Error: Not enough free diskspace available.\n");
			exit(EXIT_FAILURE);
		}
		p->newdb=readdb(p->interface, p->dirname, 0);
		if (data.active && !p->newdb) {
			data.active=0;
			writedb(p->interface, p->dirname, 0);
			if (debug)
				printf("Interface \"%s\" disabled.\n", data.interface);
		} else if (!p->newdb) {
			printf("Interface \"%s\" is already disabled.\n", data.interface);
		}
	}
}

void handlecreate(PARAMS *p)
{
	if (!p->create) {
		return;
	}

	if (p->defaultiface) {
		printf("Error: Use -i parameter to specify an interface.\n");
		exit(EXIT_FAILURE);
	}

	if (!p->force && !getifinfo(p->interface)) {
		getiflist(&p->ifacelist, 1);
		printf("Only available interfaces can be added for monitoring.\n\n");
		printf("The following interfaces are currently available:\n    %s\n", p->ifacelist);
		free(p->ifacelist);
		exit(EXIT_FAILURE);
	}

	if (checkdb(p->interface, p->dirname)) {
		printf("Error: Database for interface \"%s\" already exists.\n", p->interface);
		exit(EXIT_FAILURE);
	}

	if (!p->force && !spacecheck(p->dirname)) {
		printf("Error: Not enough free diskspace available.\n");
		exit(EXIT_FAILURE);
	}

	printf("Creating database for interface \"%s\"...\n", p->interface);
	initdb();
	strncpy_nt(data.interface, p->interface, 32);
	strncpy_nt(data.nick, p->interface, 32);
	if (writedb(p->interface, p->dirname, 1)) {
		printf("\nRestart the vnStat daemon if it is currently running in order to start monitoring \"%s\".\n", p->interface);
	}
}

void handleupdate(PARAMS *p)
{
	DIR *dir = NULL;
	struct dirent *di = NULL;
	time_t current;

	if (!p->update) {
		return;
	}

	current = time(NULL);

	/* check that there's some free diskspace left */
	if (!p->force && !spacecheck(p->dirname)) {
		printf("Error: Not enough free diskspace available.\n");
		exit(EXIT_FAILURE);
	}

	/* update every file if -i isn't specified */
	if (p->defaultiface) {

		if ((dir=opendir(p->dirname))==NULL) {
			return;
		}

		p->files=0;
		while ((di=readdir(dir))) {

			/* ignore backup files, '.' and '..' dirs */
			if ((di->d_name[0]=='.') || (strcmp(di->d_name, DATABASEFILE)==0)) {
				continue;
			}

			p->files++;
			strncpy_nt(p->interface, di->d_name, 32);
			if (debug)
				printf("\nProcessing file \"%s/%s\"...\n", p->dirname, p->interface);
			p->newdb=readdb(p->interface, p->dirname, 0);

			if (!data.active) {
				if (debug)
					printf("Disabled interface \"%s\" not updated.\n", data.interface);
				continue;
			}

			/* skip interface if not available */
			if (!getifinfo(data.interface)) {
				if (debug)
					printf("Interface \"%s\" not available, skipping.\n", data.interface);
				continue;
			}
			parseifinfo(p->newdb);

			/* check that the time is correct */
			if ((current>=data.lastupdated) || p->force) {
				writedb(p->interface, p->dirname, p->newdb);
			} else {
				/* print error if previous update is more than 6 hours in the future */
				/* otherwise do nothing */
				if (data.lastupdated>=(current+21600)) {
					printf("Error: The previous update was after the current date.\n\n");
					printf("Previous update: %s", (char*)asctime(localtime(&data.lastupdated)));
					printf("   Current time: %s\n", (char*)asctime(localtime(&current)));
					printf("Use --force to override this message.\n");
					exit(EXIT_FAILURE);
				} else {
					if (debug)
						printf("\"%s\" not updated, %s > %s.\n", data.interface, (char*)asctime(localtime(&data.lastupdated)), (char*)asctime(localtime(&current)));
				}
			}
		}

		closedir(dir);
		if (p->files==0) {
			/* no database found */
			p->update=0;
		}

	/* update only selected file */
	} else {
		p->newdb=readdb(p->interface, p->dirname, 0);

		if (!data.active) {
			if (debug)
				printf("Disabled interface \"%s\" not updated.\n", data.interface);
			return;
		}

		if (!p->force && !getifinfo(data.interface)) {
			getiflist(&p->ifacelist, 1);
			printf("Only available interfaces can be added for monitoring.\n\n");
			printf("The following interfaces are currently available:\n    %s\n", p->ifacelist);
			free(p->ifacelist);
			exit(EXIT_FAILURE);
		}
		parseifinfo(p->newdb);
		if ((current>=data.lastupdated) || p->force) {
			if (strcmp(p->nick, "none")!=0)
				strncpy_nt(data.nick, p->nick, 32);
			writedb(p->interface, p->dirname, p->newdb);
		} else {
			/* print error if previous update is more than 6 hours in the future */
			/* otherwise do nothing */
			if (data.lastupdated>=(current+21600)) {
				printf("Error: The previous update was after the current date.\n\n");
				printf("Previous update: %s", (char*)asctime(localtime(&data.lastupdated)));
				printf("   Current time: %s\n", (char*)asctime(localtime(&current)));
				printf("Use --force to override this message.\n");
				exit(EXIT_FAILURE);
			} else {
				if (debug)
					printf("\"%s\" not updated, %s > %s.\n", data.interface, (char*)asctime(localtime(&data.lastupdated)), (char*)asctime(localtime(&current)));
			}
		}
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
	if (p->files==0) {
		/* printf("No database found.\n"); */
		p->query=0;
	} else if ((cfg.qmode==0 || cfg.qmode==8 || cfg.qmode==10) && (p->files>1)) {

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
			p->newdb=readdb(p->interface, p->dirname, p->force);
			if (p->newdb) {
				continue;
			}
			if (cfg.qmode==0) {
				showdb(5);
			} else if (cfg.qmode==8) {
				showxml(p->xmlmode);
			} else if (cfg.qmode==10) {
				showjson(dbcount, p->jsonmode);
			}
			dbcount++;
		}
		closedir(dir);
		if (cfg.qmode==8) {
			xmlfooter();
		} else if (cfg.qmode==10) {
			jsonfooter();
		}

	/* show in qmode if there's only one file or qmode!=0 */
	} else {
		showoneinterface(p, p->definterface);
	}
}

void showoneinterface(PARAMS *p, const char *interface)
{
	if (!p->merged) {
		p->newdb=readdb(interface, p->dirname, p->force);
	}
	if (p->newdb) {
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
		showdb(cfg.qmode);
	} else if (cfg.qmode==8) {
		xmlheader();
		showxml(p->xmlmode);
		xmlfooter();
	} else if (cfg.qmode==10) {
		jsonheader();
		showjson(0, p->jsonmode);
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
		trafficmeter(p->interface, cfg.sampletime);
	}

	/* live traffic */
	if (p->livetraffic) {
		livetrafficmeter(p->interface, p->livemode);
	}
}
