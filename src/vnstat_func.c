#include "common.h"
#include "ifinfo.h"
#include "iflist.h"
#include "traffic.h"
#include "dbsql.h"
#include "dbxml.h"
#include "dbjson.h"
#include "dbshow.h"
#include "misc.h"
#include "cfg.h"
#include "cfgoutput.h"
#include "vnstat_func.h"

void initparams(PARAMS *p)
{
	db = NULL;
	noexit = 0;		   /* allow functions to exit in case of error */
	debug = 0;		   /* debug disabled by default */
	disableprints = 0; /* let prints be visible */

	p->addiface = 0;
	p->query = 1;
	p->setalias = 0;
	p->dbifcount = 0;
	p->force = 0;
	p->traffic = 0;
	p->livetraffic = 0;
	p->defaultiface = 1;
	p->removeiface = 0;
	p->renameiface = 0;
	p->livemode = 0;
	p->ifacelist = NULL;
	p->interface[0] = '\0';
	p->alias[0] = '\0';
	p->newifname[0] = '\0';
	p->filename[0] = '\0';
	p->definterface[0] = '\0';
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
	printf("      -i,  --iface <interface>     select interface");
	if (strlen(p->definterface)) {
		printf(" (default: %s)", p->definterface);
	}
	printf("\n\n");

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
	printf("      --rename <name>              rename interface in database\n");
	printf("      --setalias <alias>           set alias for interface\n\n");

	printf("Misc:\n");

	printf("      -i,  --iface <interface>     select interface");
	if (strlen(p->definterface)) {
		printf(" (default: %s)", p->definterface);
	}
	printf("\n");
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

	if (p->defaultiface) {
		printf("Error: Use -i parameter to specify an interface.\n");
		exit(EXIT_FAILURE);
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
		printf("Error: Handling database \"%s/%s\" failing: %s\n", cfg.dbdir, DATABASEFILE, strerror(errno));
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

void handlerenameinterface(PARAMS *p)
{
	if (!p->renameiface) {
		return;
	}

	if (p->defaultiface) {
		printf("Error: Use -i parameter to specify an interface.\n");
		exit(EXIT_FAILURE);
	}

	if (!strlen(p->newifname)) {
		printf("Error: New interface name must be at least one character long.\n");
		exit(EXIT_FAILURE);
	}

	if (!db_getinterfacecountbyname(p->interface)) {
		printf("Error: Interface \"%s\" not found in database.\n", p->interface);
		exit(EXIT_FAILURE);
	}

	if (db_getinterfacecountbyname(p->newifname)) {
		printf("Error: Interface \"%s\" already exists in database.\n", p->interface);
		exit(EXIT_FAILURE);
	}

	if (!p->force) {
		printf("Warning:\nThe current option would rename\ninterface \"%s\" -> \"%s\" in the database.\n", p->interface, p->newifname);
		printf("Use --force in order to really do that.\n");
		exit(EXIT_FAILURE);
	}

	if (!db_close() || !db_open_rw(0)) {
		printf("Error: Handling database \"%s/%s\" failing: %s\n", cfg.dbdir, DATABASEFILE, strerror(errno));
		exit(EXIT_FAILURE);
	}

	if (db_renameinterface(p->interface, p->newifname)) {
		printf("Interface \"%s\" has been renamed \"%s\".\n", p->interface, p->newifname);
		exit(EXIT_SUCCESS);
	} else {
		printf("Error: Renaming interface \"%s\" -> \"%s\" failed.\n", p->interface, p->newifname);
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
		getifliststring(&p->ifacelist, 1);
		printf("Only available interfaces can be added for monitoring.\n\n");
		printf("The following interfaces are currently available:\n    %s\n", p->ifacelist);
		free(p->ifacelist);
		exit(EXIT_FAILURE);
	}

	if (!p->force && !spacecheck(cfg.dbdir)) {
		printf("Error: Not enough free diskspace available.\n");
		exit(EXIT_FAILURE);
	}

	if (!db_close() || !db_open_rw(0)) {
		printf("Error: Handling database \"%s/%s\" failing: %s\n", cfg.dbdir, DATABASEFILE, strerror(errno));
		exit(EXIT_FAILURE);
	}

	printf("Adding interface \"%s\" for monitoring to database...\n", p->interface);
	if (db_addinterface(p->interface)) {
		printf("\nRestart the vnStat daemon if it is currently running in order to start monitoring \"%s\".\n", p->interface);
		exit(EXIT_SUCCESS);
	} else {
		printf("Error: Adding interface \"%s\" to database failed.\n", p->interface);
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
		printf("Error: Handling database \"%s/%s\" failing: %s\n", cfg.dbdir, DATABASEFILE, strerror(errno));
		exit(EXIT_FAILURE);
	}

	if (db_setalias(p->interface, p->alias)) {
		printf("Alias of interface \"%s\" set to \"%s\".\n", p->interface, p->alias);
		exit(EXIT_SUCCESS);
	} else {
		printf("Error: Setting interface \"%s\" alias failed.\n", p->interface);
		exit(EXIT_FAILURE);
	}
}

void handleshowdatabases(PARAMS *p)
{
	int ifcount = 0;
	iflist *dbifl = NULL, *dbifl_i = NULL;

	if (!p->query) {
		return;
	}

	/* show only specified file */
	if (!p->defaultiface) {
		showoneinterface(p, p->interface);
		return;
	}

	/* show all interfaces if -i isn't specified */
	if (p->dbifcount == 0) {
		p->query = 0;
	} else if ((cfg.qmode == 0 || cfg.qmode == 8 || cfg.qmode == 10) && (p->dbifcount > 1)) {

		if (cfg.qmode == 0) {
			if (cfg.ostyle != 0) {
				printf("\n                      rx      /      tx      /     total    /   estimated\n");
			} else {
				printf("\n                      rx      /      tx      /     total\n");
			}
		} else if (cfg.qmode == 8) {
			xmlheader();
		} else if (cfg.qmode == 10) {
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
			if (cfg.qmode == 0) {
				showdb(p->interface, 5, "", "");
			} else if (cfg.qmode == 8) {
				showxml(p->interface, p->xmlmode, p->databegin, p->dataend);
			} else if (cfg.qmode == 10) {
				showjson(p->interface, ifcount, p->jsonmode, p->databegin, p->dataend);
			}
			ifcount++;
			dbifl_i = dbifl_i->next;
		}
		iflistfree(&dbifl);

		if (cfg.qmode == 8) {
			xmlfooter();
		} else if (cfg.qmode == 10) {
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
		if (strchr(p->interface, '+') == NULL) {
			printf("Error: Interface \"%s\" not found in database.\n", p->interface);
		} else {
			printf("Error: Not all requested interfaces found in database or given interfaces aren't unique.\n");
		}
		exit(EXIT_FAILURE);
	}

	if (cfg.qmode == 5) {
		if (cfg.ostyle != 0) {
			printf("\n                      rx      /      tx      /     total    /   estimated\n");
		} else {
			printf("\n                      rx      /      tx      /     total\n");
		}
	}
	if (cfg.qmode != 8 && cfg.qmode != 10) {
		showdb(interface, cfg.qmode, p->databegin, p->dataend);
	} else if (cfg.qmode == 8) {
		xmlheader();
		showxml(p->interface, p->xmlmode, p->databegin, p->dataend);
		xmlfooter();
	} else if (cfg.qmode == 10) {
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
		getifliststring(&p->ifacelist, 0);
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

void handleifselection(PARAMS *p)
{
	int ifcount, dbifcount = 0, iffound = 0, dbopened = 0;
	iflist *ifl = NULL;
	iflist *dbifl = NULL, *dbifl_iterator = NULL;

	if (!p->defaultiface || !p->query) {
		return;
	}

	if (strlen(p->definterface)) {
		strncpy_nt(p->interface, p->definterface, 32);
		return;
	}

	if (p->traffic || p->livetraffic) {
		ifcount = getiflist(&ifl, 0, 1);

		/* try to open database for extra information */
		if (db == NULL) {
			if (!db_open_ro()) {
				db = NULL;
			} else {
				dbopened = 1;
			}
		}

		if (db != NULL) {
			dbifcount = db_getiflist_sorted(&dbifl, 1);
			if (dbopened) {
				db_close();
			}
		}

		if (dbifcount > 0 && ifcount > 0) {
			dbifl_iterator = dbifl;
			while (dbifl_iterator != NULL) {
				if (iflistsearch(&ifl, dbifl_iterator->interface)) {
					strncpy_nt(p->interface, dbifl_iterator->interface, 32);
					iffound = 1;
					if (debug)
						printf("Automatically selected interface with db: \"%s\"\n", p->interface);
					break;
				}
				dbifl_iterator = dbifl_iterator->next;
			}
		}

		if (!iffound) {
			if (ifcount > 0) {
				strncpy_nt(p->interface, ifl->interface, 32);
				if (debug)
					printf("Automatically selected interface without db: \"%s\"\n", p->interface);
			} else {
				printf("Error: Unable to find any suitable interface.\n");
				iflistfree(&ifl);
				iflistfree(&dbifl);
				exit(EXIT_FAILURE);
			}
		}

		iflistfree(&ifl);
	} else {
		if (db_getiflist_sorted(&dbifl, 1) <= 0) {
			printf("Error: Unable to discover suitable interface from database.\n");
			iflistfree(&dbifl);
			exit(EXIT_FAILURE);
		}
		strncpy_nt(p->interface, dbifl->interface, 32);
		if (debug)
			printf("Automatically selected interface from db: \"%s\"\n", p->interface);
	}

	iflistfree(&dbifl);
}
