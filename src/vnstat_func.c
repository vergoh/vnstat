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
	disableprinte = 0; /* let printe() output be visible */
	stderrprinte = 0;  /* use stdout for printe() output */

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
	p->limit = -1;
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

	p->alert = 0;
	p->alertoutput = 0;
	p->alertexit = 0;
	p->alerttype = 0;
	p->alertcondition = 0;
	p->alertlimit = 0;
	p->alertrateunit = -1;
	p->alertrateunitmode = -1;

	/* load default config */
	defaultcfg();
}

void showhelp(const PARAMS *p)
{
	printf("vnStat %s by Teemu Toivola <tst at iki dot fi>\n\n", getversion());

	printf("      -5,  --fiveminutes [limit]   show 5 minutes\n");
	printf("      -h,  --hours [limit]         show hours\n");
	printf("      -hg, --hoursgraph            show hours graph\n");
	printf("      -d,  --days [limit]          show days\n");
	printf("      -m,  --months [limit]        show months\n");
	printf("      -y,  --years [limit]         show years\n");
	printf("      -t,  --top [limit]           show top days\n\n");

	printf("      -b, --begin <date>           set list begin date\n");
	printf("      -e, --end <date>             set list end date\n\n");

	printf("      --95%%                        show 95th percentile\n"); // TODO: documentation
	printf("      --oneline [mode]             show simple parsable format\n");
	printf("      --json [mode] [limit]        show database in json format\n");
	printf("      --xml [mode] [limit]         show database in xml format\n");
	printf("      --alert <output> <exit> <type> <condition> <limit> <unit>\n");
	printf("                                   alert if limit is exceeded\n\n");

	printf("      -tr, --traffic [time]        calculate traffic\n");
	printf("      -l,  --live [mode]           show transfer rate in real time\n");
	printf("      -i,  --iface <interface>     select interface");
	if (strlen(p->definterface)) {
		printf(" (default: %s)", p->definterface);
	}
	printf("\n\n");

	printf("Use \"--longhelp\" or \"man vnstat\" for complete list of options.\n");
}

void showlonghelp(const PARAMS *p)
{
	printf("vnStat %s by Teemu Toivola <tst at iki dot fi>\n\n", getversion());

	printf("Query:\n");

	printf("      -q,  --query [mode]          query database\n");
	printf("      -s,  --short                 use short output\n");
	printf("      -5,  --fiveminutes [limit]   show 5 minutes\n");
	printf("      -h,  --hours [limit]         show hours\n");
	printf("      -hg, --hoursgraph            show hours graph\n");
	printf("      -d,  --days [limit]          show days\n");
	printf("      -m,  --months [limit]        show months\n");
	printf("      -y,  --years [limit]         show years\n");
	printf("      -t,  --top [limit]           show top days\n");
	printf("      -b,  --begin <date>          set list begin date\n");
	printf("      -e,  --end <date>            set list end date\n");
	printf("      --oneline [mode]             show simple parsable format\n");
	printf("      --json [mode] [limit]        show database in json format\n");
	printf("      --xml [mode] [limit]         show database in xml format\n");
	printf("      --alert <output> <exit> <type> <condition> <limit> <unit>\n");
	printf("                                   alert if limit is exceeded\n\n");

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
	printf("      --limit <limit>              set output entry limit\n");
	printf("      --style <mode>               select output style (0-4)\n");
	printf("      --iflist [mode]              show list of available interfaces\n");
	printf("      --dbiflist [mode]            show list of interfaces in database\n");
	printf("      --dbdir <directory>          select database directory\n");
	printf("      --locale <locale>            set locale\n");
	printf("      --config <config file>       select config file\n");
	printf("      --showconfig                 dump config file with current settings\n");
	printf("      --longhelp                   show this help\n\n");

	printf("See also \"man vnstat\" for longer descriptions of each option.\n");
}

void parseargs(PARAMS *p, const int argc, char **argv)
{
	int currentarg;

	/* parse parameters, maybe not the best way but... */
	for (currentarg = 1; currentarg < argc; currentarg++) {
		if (debug)
			printf("arg %d: \"%s\"\n", currentarg, argv[currentarg]);
		if (strcmp(argv[currentarg], "--longhelp") == 0) {
			showlonghelp(p);
			exit(EXIT_SUCCESS);
		} else if ((strcmp(argv[currentarg], "-?") == 0) || (strcmp(argv[currentarg], "--help") == 0)) {
			showhelp(p);
			exit(EXIT_SUCCESS);
		} else if ((strcmp(argv[currentarg], "-i") == 0) || (strcmp(argv[currentarg], "--iface") == 0) || (strcmp(argv[currentarg], "--interface") == 0)) {
			if (currentarg + 1 < argc) {
				if (strchr(argv[currentarg + 1], '+') != NULL) {
					if (strlen(argv[currentarg + 1]) > MAXIFPARAMLEN - 1) {
						printf("Error: Interface merge is limited to %d characters.\n", MAXIFPARAMLEN - 1);
						exit(EXIT_FAILURE);
					}
				} else {
					if (strlen(argv[currentarg + 1]) > MAXIFLEN - 1) {
						printf("Error: Interface name is limited to %d characters.\n", MAXIFLEN - 1);
						exit(EXIT_FAILURE);
					}
				}
				strncpy_nt(p->interface, argv[currentarg + 1], MAXIFPARAMLEN);
				if (strlen(p->interface)) {
					p->defaultiface = 0;
				} else {
					strncpy_nt(p->definterface, p->interface, MAXIFPARAMLEN);
				}
				if (debug)
					printf("Used interface: \"%s\"\n", p->interface);
				currentarg++;
			} else {
				printf("Error: Interface for %s missing.\n", argv[currentarg]);
				exit(EXIT_FAILURE);
			}
		} else if (strcmp(argv[currentarg], "--config") == 0) {
			/* config has already been parsed earlier so nothing to do here */
			currentarg++;
		} else if (strcmp(argv[currentarg], "--setalias") == 0 || strcmp(argv[currentarg], "--nick") == 0) {
			if (strcmp(argv[currentarg], "--nick") == 0) {
				printf("Warning: --nick is deprecated and will be removed in a future release. Use --setalias instead.\n");
			}
			if (currentarg + 1 < argc) {
				strncpy_nt(p->alias, argv[currentarg + 1], 32);
				if (debug)
					printf("Used alias: \"%s\"\n", p->alias);
				p->setalias = 1;
				currentarg++;
			} else {
				printf("Error: Alias for %s missing.\n", argv[currentarg]);
				exit(EXIT_FAILURE);
			}
		} else if ((strcmp(argv[currentarg], "--style")) == 0) {
			if (currentarg + 1 < argc && isdigit(argv[currentarg + 1][0])) {
				cfg.ostyle = atoi(argv[currentarg + 1]);
				if (cfg.ostyle > 4 || cfg.ostyle < 0) {
					printf("Error: Invalid style parameter \"%d\"\n", cfg.ostyle);
					showstylehelp();
					exit(EXIT_FAILURE);
				}
				if (debug)
					printf("Style changed: %d\n", cfg.ostyle);
				currentarg++;
			} else {
				printf("Error: Style parameter for --style missing.\n");
				showstylehelp();
				exit(EXIT_FAILURE);
			}
		} else if ((strcmp(argv[currentarg], "--dbdir")) == 0) {
			if (currentarg + 1 < argc) {
				strncpy_nt(cfg.dbdir, argv[currentarg + 1], 512);
				if (debug)
					printf("DatabaseDir: \"%s\"\n", cfg.dbdir);
				currentarg++;
			} else {
				printf("Error: Directory for %s missing.\n", argv[currentarg]);
				exit(EXIT_FAILURE);
			}
		} else if ((strcmp(argv[currentarg], "--locale")) == 0) {
			if (currentarg + 1 < argc) {
				setlocale(LC_ALL, argv[currentarg + 1]);
				if (debug)
					printf("Locale: \"%s\"\n", argv[currentarg + 1]);
				currentarg++;
			} else {
				printf("Error: Locale for %s missing.\n", argv[currentarg]);
				exit(EXIT_FAILURE);
			}
		} else if (strcmp(argv[currentarg], "--add") == 0) {
			p->addiface = 1;
			p->query = 0;
		} else if ((strcmp(argv[currentarg], "-u") == 0) || (strcmp(argv[currentarg], "--update") == 0)) {
			printf("Error: The \"%s\" parameter is not supported in this version.\n", argv[currentarg]);
			exit(EXIT_FAILURE);
		} else if ((strcmp(argv[currentarg], "-q") == 0) || (strcmp(argv[currentarg], "--query") == 0)) {
			p->query = 1;
			if (currentarg + 1 < argc && (strlen(argv[currentarg + 1]) == 1 || ishelprequest(argv[currentarg + 1]))) {
				if (argv[currentarg + 1][0] == 'a') {
					cfg.qmode = 0;
				} else if (argv[currentarg + 1][0] == 's') {
					cfg.qmode = 4;
				} else {
					if (!ishelprequest(argv[currentarg + 1]))
						printf("Error: Invalid mode parameter \"%s\".\n", argv[currentarg + 1]);
					printf(" Valid parameters for %s:\n", argv[currentarg]);
					printf("    (none) - use QueryMode configuration\n");
					printf("    a      - summary for all interfaces\n");
					printf("    s      - summary for single interface\n");
					exit(EXIT_FAILURE);
				}
				currentarg++;
			}
		} else if ((strcmp(argv[currentarg], "-D") == 0) || (strcmp(argv[currentarg], "--debug") == 0)) {
			debug = 1;
		} else if ((strcmp(argv[currentarg], "-d") == 0) || (strcmp(argv[currentarg], "--days") == 0)) {
			cfg.qmode = 1;
			if (currentarg + 1 < argc && isdigit(argv[currentarg + 1][0])) {
				cfg.listdays = atoi(argv[currentarg + 1]);
				if (cfg.listdays < 0) {
					printf("Error: Invalid limit parameter \"%s\" for %s. Only a zero and positive numbers are allowed.\n", argv[currentarg + 1], argv[currentarg]);
					exit(EXIT_FAILURE);
				}
				currentarg++;
			}
		} else if ((strcmp(argv[currentarg], "-m") == 0) || (strcmp(argv[currentarg], "--months") == 0)) {
			cfg.qmode = 2;
			if (currentarg + 1 < argc && isdigit(argv[currentarg + 1][0])) {
				cfg.listmonths = atoi(argv[currentarg + 1]);
				if (cfg.listmonths < 0) {
					printf("Error: Invalid limit parameter \"%s\" for %s. Only a zero and positive numbers are allowed.\n", argv[currentarg + 1], argv[currentarg]);
					exit(EXIT_FAILURE);
				}
				currentarg++;
			}
		} else if ((strcmp(argv[currentarg], "-t") == 0) || (strcmp(argv[currentarg], "--top") == 0)) {
			cfg.qmode = 3;
			if (currentarg + 1 < argc && isdigit(argv[currentarg + 1][0])) {
				cfg.listtop = atoi(argv[currentarg + 1]);
				if (cfg.listtop < 0) {
					printf("Error: Invalid limit parameter \"%s\" for %s. Only a zero and positive numbers are allowed.\n", argv[currentarg + 1], argv[currentarg]);
					exit(EXIT_FAILURE);
				}
				currentarg++;
			}
		} else if ((strcmp(argv[currentarg], "-s") == 0) || (strcmp(argv[currentarg], "--short") == 0)) {
			cfg.qmode = 5;
		} else if ((strcmp(argv[currentarg], "-y") == 0) || (strcmp(argv[currentarg], "--years") == 0)) {
			cfg.qmode = 6;
			if (currentarg + 1 < argc && isdigit(argv[currentarg + 1][0])) {
				cfg.listyears = atoi(argv[currentarg + 1]);
				if (cfg.listyears < 0) {
					printf("Error: Invalid limit parameter \"%s\" for %s. Only a zero and positive numbers are allowed.\n", argv[currentarg + 1], argv[currentarg]);
					exit(EXIT_FAILURE);
				}
				currentarg++;
			}
		} else if ((strcmp(argv[currentarg], "-hg") == 0) || (strcmp(argv[currentarg], "--hoursgraph") == 0)) {
			cfg.qmode = 7;
		} else if ((strcmp(argv[currentarg], "-h") == 0) || (strcmp(argv[currentarg], "--hours") == 0)) {
			cfg.qmode = 11;
			if (currentarg + 1 < argc && isdigit(argv[currentarg + 1][0])) {
				cfg.listhours = atoi(argv[currentarg + 1]);
				if (cfg.listhours < 0) {
					printf("Error: Invalid limit parameter \"%s\" for %s. Only a zero and positive numbers are allowed.\n", argv[currentarg + 1], argv[currentarg]);
					exit(EXIT_FAILURE);
				}
				currentarg++;
			}
		} else if ((strcmp(argv[currentarg], "-5") == 0) || (strcmp(argv[currentarg], "--fiveminutes") == 0)) {
			cfg.qmode = 12;
			if (currentarg + 1 < argc && isdigit(argv[currentarg + 1][0])) {
				cfg.listfivemins = atoi(argv[currentarg + 1]);
				if (cfg.listfivemins < 0) {
					printf("Error: Invalid limit parameter \"%s\" for %s. Only a zero and positive numbers are allowed.\n", argv[currentarg + 1], argv[currentarg]);
					exit(EXIT_FAILURE);
				}
				currentarg++;
			}
		} else if ((strcmp(argv[currentarg], "--95") == 0) || (strcmp(argv[currentarg], "--95%") == 0) || (strcmp(argv[currentarg], "--95th") == 0)) {
			cfg.qmode = 13;
		} else if (strcmp(argv[currentarg], "--oneline") == 0) {
			cfg.qmode = 9;
			if (currentarg + 1 < argc && (strlen(argv[currentarg + 1]) == 1 || ishelprequest(argv[currentarg + 1]))) {
				if (argv[currentarg + 1][0] == 'b') {
					cfg.ostyle = 4;
					currentarg++;
				} else {
					if (!ishelprequest(argv[currentarg + 1]))
						printf("Error: Invalid mode parameter \"%s\".\n", argv[currentarg + 1]);
					printf(" Valid parameters for --oneline:\n");
					printf("    (none) - automatically scaled units visible\n");
					printf("    b      - all values in bytes\n");
					exit(EXIT_FAILURE);
				}
			}
		} else if (strcmp(argv[currentarg], "--xml") == 0) {
			cfg.qmode = 8;
			if (currentarg + 1 < argc && (strlen(argv[currentarg + 1]) == 1 || ishelprequest(argv[currentarg + 1]))) {
				p->xmlmode = argv[currentarg + 1][0];
				if (strlen(argv[currentarg + 1]) != 1 || strchr("asfhdmytp", p->xmlmode) == NULL) {
					if (!ishelprequest(argv[currentarg + 1]))
						printf("Error: Invalid mode parameter \"%s\".\n", argv[currentarg + 1]);
					printf(" Valid parameters for --xml:\n");
					printf("    a - all except 95th percentile (default)\n");
					printf("    s - summary\n");
					printf("    f - only 5 minutes\n");
					printf("    h - only hours\n");
					printf("    d - only days\n");
					printf("    m - only months\n");
					printf("    y - only years\n");
					printf("    t - only top\n");
					printf("    p - 95th percentile\n"); // TODO: documentation
					exit(EXIT_FAILURE);
				}
				if (p->xmlmode == 's') {
					cfg.listjsonxml = 2;
				}
				currentarg++;
			}
			if (currentarg + 1 < argc && isdigit(argv[currentarg + 1][0])) {
				cfg.listjsonxml = atoi(argv[currentarg + 1]);
				if (cfg.listjsonxml < 0) {
					printf("Error: Invalid limit parameter \"%s\" for %s. Only a zero and positive numbers are allowed.\n", argv[currentarg + 1], argv[currentarg]);
					exit(EXIT_FAILURE);
				}
				currentarg++;
			}
		} else if (strcmp(argv[currentarg], "--json") == 0) {
			cfg.qmode = 10;
			if (currentarg + 1 < argc && (strlen(argv[currentarg + 1]) == 1 || ishelprequest(argv[currentarg + 1]))) {
				p->jsonmode = argv[currentarg + 1][0];
				if (strlen(argv[currentarg + 1]) != 1 || strchr("asfhdmytp", p->jsonmode) == NULL) {
					if (!ishelprequest(argv[currentarg + 1]))
						printf("Error: Invalid mode parameter \"%s\".\n", argv[currentarg + 1]);
					printf(" Valid parameters for --json:\n");
					printf("    a - all except 95th percentile (default)\n");
					printf("    s - summary\n");
					printf("    f - only 5 minutes\n");
					printf("    h - only hours\n");
					printf("    d - only days\n");
					printf("    m - only months\n");
					printf("    y - only years\n");
					printf("    t - only top\n");
					printf("    p - 95th percentile\n"); // TODO: documentation
					exit(EXIT_FAILURE);
				}
				if (p->jsonmode == 's') {
					cfg.listjsonxml = 2;
				}
				currentarg++;
			}
			if (currentarg + 1 < argc && isdigit(argv[currentarg + 1][0])) {
				cfg.listjsonxml = atoi(argv[currentarg + 1]);
				if (cfg.listjsonxml < 0) {
					printf("Error: Invalid limit parameter \"%s\" for %s. Only a zero and positive numbers are allowed.\n", argv[currentarg + 1], argv[currentarg]);
					exit(EXIT_FAILURE);
				}
				currentarg++;
			}
		} else if ((strcmp(argv[currentarg], "-ru") == 0) || (strcmp(argv[currentarg], "--rateunit")) == 0) {
			if (currentarg + 1 < argc && (strlen(argv[currentarg + 1]) == 1 || ishelprequest(argv[currentarg + 1]))) {
				if (!isdigit(argv[currentarg + 1][0]) || atoi(argv[currentarg + 1]) > 1 || atoi(argv[currentarg + 1]) < 0) {
					if (!ishelprequest(argv[currentarg + 1]))
						printf("Error: Invalid parameter \"%s\".\n", argv[currentarg + 1]);
					printf(" Valid parameters for %s:\n", argv[currentarg]);
					printf("    0 - bytes\n");
					printf("    1 - bits\n");
					exit(EXIT_FAILURE);
				}
				cfg.rateunit = atoi(argv[currentarg + 1]);
				if (debug)
					printf("Rateunit changed: %d\n", cfg.rateunit);
				currentarg++;
			} else {
				cfg.rateunit = !cfg.rateunit;
				if (debug)
					printf("Rateunit changed: %d\n", cfg.rateunit);
			}
		} else if ((strcmp(argv[currentarg], "-tr") == 0) || (strcmp(argv[currentarg], "--traffic") == 0)) {
			if (currentarg + 1 < argc && isdigit(argv[currentarg + 1][0])) {
				cfg.sampletime = atoi(argv[currentarg + 1]);
				currentarg++;
			}
			p->traffic = 1;
			p->query = 0;
		} else if ((strcmp(argv[currentarg], "-l") == 0) || (strcmp(argv[currentarg], "--live") == 0)) {
			if (currentarg + 1 < argc && (strlen(argv[currentarg + 1]) == 1 || ishelprequest(argv[currentarg + 1]))) {
				if (!isdigit(argv[currentarg + 1][0]) || atoi(argv[currentarg + 1]) > 1 || atoi(argv[currentarg + 1]) < 0) {
					if (!ishelprequest(argv[currentarg + 1]))
						printf("Error: Invalid mode parameter \"%s\".\n", argv[currentarg + 1]);
					printf(" Valid parameters for %s:\n", argv[currentarg]);
					printf("    0 - show packets per second (default)\n");
					printf("    1 - show transfer counters\n");
					exit(EXIT_FAILURE);
				}
				p->livemode = atoi(argv[currentarg + 1]);
				currentarg++;
			}
			p->livetraffic = 1;
			p->query = 0;
		} else if (strcmp(argv[currentarg], "--force") == 0) {
			p->force = 1;
		} else if (strcmp(argv[currentarg], "--showconfig") == 0) {
			printcfgfile();
			exit(EXIT_SUCCESS);
		} else if (strcmp(argv[currentarg], "--remove") == 0) {
			p->removeiface = 1;
			p->query = 0;
		} else if (strcmp(argv[currentarg], "--rename") == 0) {
			if (currentarg + 1 < argc) {
				if (strlen(argv[currentarg + 1]) > MAXIFLEN - 1) {
					printf("Error: Interface name is limited to %d characters.\n", MAXIFLEN - 1);
					exit(EXIT_FAILURE);
				}
				strncpy_nt(p->newifname, argv[currentarg + 1], MAXIFLEN);
				if (debug)
					printf("Given new interface name: \"%s\"\n", p->newifname);
				p->renameiface = 1;
				p->query = 0;
				currentarg++;
			} else {
				printf("Error: New interface name for %s missing.\n", argv[currentarg]);
				exit(EXIT_FAILURE);
			}
		} else if ((strcmp(argv[currentarg], "-b") == 0) || (strcmp(argv[currentarg], "--begin") == 0)) {
			if (currentarg + 1 < argc) {
				if (!validatedatetime(argv[currentarg + 1])) {
					printf("Error: Invalid date format, expected YYYY-MM-DD HH:MM, YYYY-MM-DD or \"today\".\n");
					exit(EXIT_FAILURE);
				}
				strncpy_nt(p->databegin, argv[currentarg + 1], 18);
				currentarg++;
			} else {
				printf("Error: Date of format YYYY-MM-DD HH:MM, YYYY-MM-DD or \"today\" for %s missing.\n", argv[currentarg]);
				exit(EXIT_FAILURE);
			}
		} else if ((strcmp(argv[currentarg], "-e") == 0) || (strcmp(argv[currentarg], "--end") == 0)) {
			if (currentarg + 1 < argc) {
				if (!validatedatetime(argv[currentarg + 1])) {
					printf("Error: Invalid date format, expected YYYY-MM-DD HH:MM or YYYY-MM-DD.\n");
					exit(EXIT_FAILURE);
				}
				strncpy_nt(p->dataend, argv[currentarg + 1], 18);
				currentarg++;
			} else {
				printf("Error: Date of format YYYY-MM-DD HH:MM or YYYY-MM-DD for %s missing.\n", argv[currentarg]);
				exit(EXIT_FAILURE);
			}
		} else if (strcmp(argv[currentarg], "--iflist") == 0) {
			p->query = 0;
			if (currentarg + 1 < argc && (strlen(argv[currentarg + 1]) == 1 || ishelprequest(argv[currentarg + 1]))) {
				if (!isdigit(argv[currentarg + 1][0]) || atoi(argv[currentarg + 1]) > 1 || atoi(argv[currentarg + 1]) < 0) {
					if (!ishelprequest(argv[currentarg + 1]))
						printf("Error: Invalid mode parameter \"%s\".\n", argv[currentarg + 1]);
					printf(" Valid parameters for --iflist:\n");
					printf("    0 - show verbose (default)\n");
					printf("    1 - show one interface per line\n");
					exit(EXIT_FAILURE);
				}
				p->query = atoi(argv[currentarg + 1]);
				currentarg++;
			}
			showiflist(p->query);
			exit(EXIT_SUCCESS);
		} else if (strcmp(argv[currentarg], "--dbiflist") == 0) {
			p->query = 0;
			if (currentarg + 1 < argc && (strlen(argv[currentarg + 1]) == 1 || ishelprequest(argv[currentarg + 1]))) {
				if (!isdigit(argv[currentarg + 1][0]) || atoi(argv[currentarg + 1]) > 1 || atoi(argv[currentarg + 1]) < 0) {
					if (!ishelprequest(argv[currentarg + 1]))
						printf("Error: Invalid mode parameter \"%s\".\n", argv[currentarg + 1]);
					printf(" Valid parameters for --dbiflist:\n");
					printf("    0 - show verbose (default)\n");
					printf("    1 - show one interface per line\n");
					exit(EXIT_FAILURE);
				}
				p->query = atoi(argv[currentarg + 1]);
				currentarg++;
			}
			showdbiflist(p->query);
			exit(EXIT_SUCCESS);
		} else if (strcmp(argv[currentarg], "--limit") == 0) {
			if (currentarg + 1 < argc && isdigit(argv[currentarg + 1][0])) {
				p->limit = atoi(argv[currentarg + 1]);
				if (p->limit < 0) {
					printf("Error: Invalid limit parameter \"%s\" for %s. Only a zero and positive numbers are allowed.\n", argv[currentarg + 1], argv[currentarg]);
					exit(EXIT_FAILURE);
				}
				currentarg++;
			} else {
				printf("Error: Invalid or missing parameter for %s.\n", argv[currentarg]);
				exit(EXIT_FAILURE);
			}
		} else if (strcmp(argv[currentarg], "--alert") == 0) {
			if (currentarg + 6 >= argc) {
				printf("Error: Invalid parameter count for %s.\n", argv[currentarg]);
				showalerthelp();
				exit(EXIT_FAILURE);
			}
			if (!parsealertargs(p, argv + currentarg)) {
				exit(EXIT_FAILURE);
			} else {
				currentarg += 6;
				p->alert = 1;
			}
		} else if ((strcmp(argv[currentarg], "-v") == 0) || (strcmp(argv[currentarg], "--version") == 0)) {
			printf("vnStat %s by Teemu Toivola <tst at iki dot fi> (SQLite %s)\n", getversion(), sqlite3_libversion());
			exit(EXIT_SUCCESS);
		} else {
			if (argv[currentarg][0] == '-' || strlen(argv[currentarg]) == 1) {
				printf("Unknown parameter \"%s\". Use --help for help.\n", argv[currentarg]);
				exit(EXIT_FAILURE);
			} else {
				if (strchr(argv[currentarg], '+') != NULL) {
					if (strlen(argv[currentarg]) > MAXIFPARAMLEN - 1) {
						printf("Error: Interface merge is limited to %d characters.\n", MAXIFPARAMLEN - 1);
						exit(EXIT_FAILURE);
					}
				} else {
					if (strlen(argv[currentarg]) > MAXIFLEN - 1) {
						printf("Error: Interface name is limited to %d characters.\n", MAXIFLEN - 1);
						exit(EXIT_FAILURE);
					}
				}
				strncpy_nt(p->interface, argv[currentarg], MAXIFPARAMLEN);
				if (strlen(p->interface)) {
					p->defaultiface = 0;
				} else {
					strncpy_nt(p->definterface, p->interface, MAXIFPARAMLEN);
				}
				if (debug)
					printf("Used interface: \"%s\"\n", p->interface);
			}
		}
	}

	if (p->limit != -1) {
		cfg.listfivemins = cfg.listhours = cfg.listdays = cfg.listmonths = cfg.listyears = cfg.listtop = cfg.listjsonxml = p->limit;
	}
}

int parsealertargs(PARAMS *p, char **argv)
{
	int i, u, found, currentarg = 0;
	uint64_t alertlimit = 0, unitmultiplier = 1;
	int32_t unitmode = cfg.unitmode;
	const char *alerttypes[] = {"h", "hour", "hourly", "d", "day", "daily", "m", "month", "monthly", "y", "year", "yearly", "p", "95", "95%"};
	const char *alertconditions[] = {"rx", "tx", "total", "rx_estimate", "tx_estimate", "total_estimate"}; // order must match that of AlertCondition in dbshow.h

	for (i = 1; i <= 6; i++) {
		if (argv[currentarg + i][0] == '-' || ishelprequest(argv[currentarg + i])) {
			showalerthelp();
			return 0;
		}
	}
	currentarg++;

	// output
	if (!isnumeric(argv[currentarg])) {
		printf("Error: Non-numeric output parameter \"%s\" for %s.\n", argv[currentarg], argv[0]);
		showalerthelp();
		return 0;
	}
	p->alertoutput = (unsigned int)atoi(argv[currentarg]);
	if (p->alertoutput > 3) {
		printf("Error: Output parameter out of range for %s.\n", argv[0]);
		showalerthelp();
		return 0;
	}
	if (debug) {
		printf("Alert output: %u\n", p->alertoutput);
	}
	currentarg++;

	// exit
	if (!isnumeric(argv[currentarg])) {
		printf("Error: Non-numeric exit parameter \"%s\" for %s.\n", argv[currentarg], argv[0]);
		showalerthelp();
		return 0;
	}
	p->alertexit = (unsigned int)atoi(argv[currentarg]);
	if (p->alertexit > 3) {
		printf("Error: Exit parameter out of range for %s.\n", argv[0]);
		showalerthelp();
		return 0;
	}
	if (debug) {
		printf("Alert exit: %u\n", p->alertexit);
	}

	if (p->alertoutput == AO_No_Output && (p->alertexit == AE_Always_Exit_0 || p->alertexit == AE_Always_Exit_1)) {
		printf("Error: Configuring %s for no output and always same exit status provides no real usability.\n", argv[0]);
		showalerthelp();
		return 0;
	}
	currentarg++;

	// type
	found = 0;
	for (i = 0; i < 15; i++) {
		if (strcmp(argv[currentarg], alerttypes[i]) == 0) {
			found = 1;
			break;
		}
	}
	if (!found) {
		printf("Error: Invalid type parameter \"%s\" for %s.\n", argv[currentarg], argv[0]);
		showalerthelp();
		return 0;
	}

	switch (argv[currentarg][0]) {
		case 'h':
			p->alerttype = AT_Hour;
			break;
		case 'd':
			p->alerttype = AT_Day;
			break;
		case 'm':
			p->alerttype = AT_Month;
			break;
		case 'y':
			p->alerttype = AT_Year;
			break;
		case 'p':
		case '9':
			p->alerttype = AT_Percentile;
			break;
		default:
			return 0;
	}
	if (debug) {
		printf("Alert type: %u\n", p->alerttype);
	}
	currentarg++;

	if (p->alerttype == AT_Percentile && (p->alertoutput == 2 || p->alertexit == 2)) {
		printf("Error: Estimate output and exit conditions cannot be used in combination with selected alert type.\n");
		showalerthelp();
		return 0;
	}

	// condition
	found = 0;
	for (i = 0; i < 3 + (cfg.estimatevisible * 3); i++) {
		if (strcmp(argv[currentarg], alertconditions[i]) == 0) {
			found = 1;
			p->alertcondition = (unsigned int)i + 1;
			break;
		}
	}
	if (!found) {
		printf("Error: Invalid condition parameter \"%s\" for %s.\n", argv[currentarg], argv[0]);
		showalerthelp();
		return 0;
	}
	if (debug) {
		printf("Alert condition: %u\n", p->alertcondition);
	}
	currentarg++;

	if ((p->alertcondition == AC_RX_Estimate || p->alertcondition == AC_TX_Estimate || p->alertcondition == AC_Total_Estimate) &&
		(p->alertoutput == AO_Output_On_Estimate || p->alertexit == AE_Exit_1_On_Estimate)) {
		if (p->alertoutput == AO_Output_On_Estimate) {
			printf("Error: Estimate conditions cannot be used in combination with output parameter \"2\".\n");
		}
		if (p->alertexit == AE_Exit_1_On_Estimate) {
			printf("Error: Estimate conditions cannot be used in combination with exit parameter \"2\".\n");
		}
		showalerthelp();
		return 0;
	}

	if (p->alerttype == AT_Percentile && (p->alertcondition == AC_RX_Estimate || p->alertcondition == AC_TX_Estimate || p->alertcondition == AC_Total_Estimate)) {
		printf("Error: Estimate conditions cannot be used in combination with selected alert type.\n");
		showalerthelp();
		return 0;
	}

	// limit
	if (!isnumeric(argv[currentarg])) {
		printf("Error: Limit parameter for %s must be a greater than zero integer without decimals.\n", argv[0]);
		showalerthelp();
		return 0;
	}
	alertlimit = strtoull(argv[currentarg], (char **)NULL, 0);
	if (alertlimit == 0) {
		printf("Error: Invalid limit parameter \"%s\" for %s.\n", argv[currentarg], argv[0]);
		showalerthelp();
		return 0;
	}
	if (debug) {
		printf("Alert limit: %" PRIu64 "\n", alertlimit);
	}
	currentarg++;

	// limit unit
	found = 0;
	if (p->alerttype != AT_Percentile) {
		for (u = 0; u < 3; u++) {
			cfg.unitmode = u;
			for (i = 1; i <= UNITPREFIXCOUNT; i++) {
				if (strcmp(argv[currentarg], getunitprefix(i)) == 0) {
					found = 1;
					break;
				}
			}
			if (found) {
				break;
			}
		}
		cfg.unitmode = unitmode;
	} else {
		for (u = 0; u < 5; u++) {
			if (u == 1) {
				continue;
			}
			for (i = 1; i <= UNITPREFIXCOUNT; i++) {
				if (strcmp(argv[currentarg], getrateunitprefix(u, i)) == 0) {
					found = 1;
					break;
				}
			}
			if (found) {
				break;
			}
		}
		if (found) {
			// override configuration to use same units as user gave as parameter
			if (u > 2) {
				cfg.rateunit = 1;
				p->alertrateunit = 1;
			} else {
				cfg.rateunit = 0;
				p->alertrateunit = 0;
			}
			if (u == 0 || u == 3) {
				cfg.rateunitmode = 0;
				p->alertrateunitmode = 0;
			} else {
				cfg.rateunitmode = 1;
				p->alertrateunitmode = 1;
			}
		}

	}
	if (!found) {
		printf("Error: Invalid limit unit parameter \"%s\" for %s.\n", argv[currentarg], argv[0]);
		showalerthelp();
		return 0;
	}

	unitmultiplier = getunitdivisor(u, i);

	if (alertlimit > (uint64_t)(MAX64 / unitmultiplier)) {
		printf("Error: %" PRIu64 " %s exceeds maximum supported limit of %" PRIu64 " %s.\n", alertlimit, argv[currentarg], (uint64_t)(MAX64 / unitmultiplier), argv[currentarg]);
		return 0;
	}

	p->alertlimit = alertlimit * unitmultiplier;

	if (p->alerttype == AT_Percentile && cfg.rateunit == 1) {
		if (p->alertlimit < 8) {
			printf("Error: Limit needs to be at least 8 bits per second.\n");
			return 0;
		}
		p->alertlimit /= 8;
	}

	if (debug) {
		printf("Alert unit %s is %d %d = %" PRIu64 "\n", argv[currentarg], u, i, unitmultiplier);
		printf("Alert real limit is %" PRIu64 " * %" PRIu64 " = %" PRIu64 "\n", alertlimit, unitmultiplier, p->alertlimit);
	}

	return 1;
}

void showalerthelp(void)
{
	printf("\n");
	printf("Valid parameters for\n--alert <output> <exit> <type> <condition> <limit> <unit>\n\n");

	printf(" <output>\n");
	printf("    0 - no output\n");
	printf("    1 - always show output\n");
	printf("    2 - show output only if usage estimate exceeds limit\n");
	printf("    3 - show output only if limit is exceeded\n\n");

	printf(" <exit>\n");
	printf("    0 - always use exit status 0\n");
	printf("    1 - always use exit status 1\n");
	printf("    2 - use exit status 1 if usage estimate exceeds limit\n");
	printf("    3 - use exit status 1 if limit is exceeded\n\n");

	printf(" <type>\n");
	printf("    h, hour, hourly        d, day, daily        p, 95, 95%%\n");
	printf("    m, month, monthly      y, year, yearly\n\n");

	printf(" <condition>\n");
	if (cfg.estimatevisible) {
		printf("    rx, tx, total, rx_estimate, tx_estimate, total_estimate\n\n");
	} else {
		printf("    rx, tx, total\n\n");
	}

	printf(" <limit>\n");
	printf("    greater than zero integer, no decimals\n\n");

	printf(" <unit> for <limit>\n");
	printf("    B, KiB, MiB, GiB, TiB, PiB, EiB\n");
	printf("    B, KB, MB, GB, TB, PB, EB\n\n");

	printf(" <unit> for <limit> when 95th percentile <type> is used\n");
	printf("    B/s, KiB/s, MiB/s, GiB/s, TiB/s, PiB/s, EiB/s                (IEC 1024^n)\n");
	printf("    B/s, kB/s, MB/s, GB/s, TB/s, PB/s, EB/s                      (SI  1000^n)\n");
	printf("    bit/s, Kibit/s, Mibit/s, Gibit/s, Tibit/s, Pibit/s, Eibit/s  (IEC 1024^n)\n");
	printf("    bit/s, kbit/s, Mbit/s, Gbit/s, Tbit/s, Pbit/s, Ebit/s        (SI  1000^n)\n");
}

void showstylehelp(void)
{
	printf(" Valid parameters for --style:\n");
	printf("    0 - a more narrow output\n");
	printf("    1 - enable bar column if available\n");
	printf("    2 - average traffic rate in summary output\n");
	printf("    3 - average traffic rate in all outputs if available\n");
	printf("    4 - disable terminal control characters in -l / --live\n");
	printf("        and -tr / --traffic, show raw values in --oneline\n");
}

void handleshowalert(PARAMS *p)
{
	int alert = 0;

	if (!p->alert) {
		return;
	}

	if (p->defaultiface) {
		printf("Error: An interface needs to be explicitly specified for --alert.\n");
		exit(EXIT_FAILURE);
	}

	validateinterface(p);

	if (p->alertrateunit > -1 && p->alertrateunitmode > -1) {
		cfg.rateunit = p->alertrateunit;
		cfg.rateunitmode = p->alertrateunitmode;
	}

	alert = showalert(p->interface, p->alertoutput, p->alertexit, p->alerttype, p->alertcondition, p->alertlimit);

	if ((alert || p->alertexit == AE_Always_Exit_1) && p->alertexit != AE_Always_Exit_0) {
		exit(EXIT_FAILURE);
	}

	exit(EXIT_SUCCESS);
}

void handleremoveinterface(const PARAMS *p)
{
	if (!p->removeiface) {
		return;
	}

	if (p->defaultiface) {
		printf("Error: An interface needs to be explicitly specified for --remove.\n");
		exit(EXIT_FAILURE);
	}

	if (!db_getinterfacecountbyname(p->interface)) {
		printf("Error: Interface \"%s\" not found in database.\n", p->interface);
		exit(EXIT_FAILURE);
	}

	if (!p->force) {
		printf("Warning:\nThe current option would remove all data about interface \"%s\" from the database. ", p->interface);
		printf("Add --force in order to really do that.\n");
		exit(EXIT_FAILURE);
	}

#ifndef CHECK_VNSTAT
	if (!db_close() || !db_open_rw(0)) {
		printf("Error: Handling database \"%s/%s\" failing: %s\n", cfg.dbdir, DATABASEFILE, strerror(errno));
		exit(EXIT_FAILURE);
	}
#endif

	if (db_removeinterface(p->interface)) {
		printf("Interface \"%s\" removed from database.\n", p->interface);
		printf("The interface will no longer be monitored. Use --add if monitoring the interface is again needed.\n");
#ifndef CHECK_VNSTAT
		db_close();
		exit(EXIT_SUCCESS);
#endif
	} else {
		printf("Error: Removing interface \"%s\" from database failed.\n", p->interface);
		exit(EXIT_FAILURE);
	}
}

void handlerenameinterface(const PARAMS *p)
{
	if (!p->renameiface) {
		return;
	}

	if (p->defaultiface) {
		printf("Error: An interface needs to be explicitly specified for --rename.\n");
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
		printf("Error: Interface \"%s\" already exists in database.\n", p->newifname);
		exit(EXIT_FAILURE);
	}

	if (!p->force) {
		printf("Warning:\nThe current option would rename interface \"%s\" -> \"%s\" in the database. ", p->interface, p->newifname);
		printf("Add --force in order to really do that.\n");
		exit(EXIT_FAILURE);
	}

#ifndef CHECK_VNSTAT
	if (!db_close() || !db_open_rw(0)) {
		printf("Error: Handling database \"%s/%s\" failing: %s\n", cfg.dbdir, DATABASEFILE, strerror(errno));
		exit(EXIT_FAILURE);
	}
#endif

	if (db_renameinterface(p->interface, p->newifname)) {
		printf("Interface \"%s\" has been renamed \"%s\".\n", p->interface, p->newifname);
#ifndef CHECK_VNSTAT
		db_close();
		exit(EXIT_SUCCESS);
#endif
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
		printf("Error: An interface needs to be explicitly specified for --add.\n");
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

#ifndef CHECK_VNSTAT
	if (!db_close() || !db_open_rw(0)) {
		printf("Error: Handling database \"%s/%s\" failing: %s\n", cfg.dbdir, DATABASEFILE, strerror(errno));
		exit(EXIT_FAILURE);
	}
#endif

	printf("Adding interface \"%s\" to database for monitoring.\n", p->interface);
	if (db_addinterface(p->interface)) {
		if (cfg.rescanonsave) {
			printf("vnStat daemon will automatically start monitoring \"%s\" within %d minutes if the daemon process is currently running.\n", p->interface, cfg.saveinterval);
		} else {
			printf("Restart vnStat daemon if it is currently running in order to start monitoring \"%s\".\n", p->interface);
		}
		handlesetalias(p);
#ifndef CHECK_VNSTAT
		db_close();
		exit(EXIT_SUCCESS);
#endif
	} else {
		printf("Error: Adding interface \"%s\" to database failed.\n", p->interface);
		exit(EXIT_FAILURE);
	}
}

void handlesetalias(const PARAMS *p)
{
	if (!p->setalias) {
		return;
	}

	if (p->defaultiface) {
		printf("Error: An interface needs to be explicitly specified for --setalias.\n");
		exit(EXIT_FAILURE);
	}

	if (!db_getinterfacecountbyname(p->interface)) {
		printf("Error: Interface \"%s\" not found in database.\n", p->interface);
		exit(EXIT_FAILURE);
	}

#ifndef CHECK_VNSTAT
	if (!db_close() || !db_open_rw(0)) {
		printf("Error: Handling database \"%s/%s\" failing: %s\n", cfg.dbdir, DATABASEFILE, strerror(errno));
		exit(EXIT_FAILURE);
	}
#endif

	if (db_setalias(p->interface, p->alias)) {
		printf("Alias of interface \"%s\" set to \"%s\".\n", p->interface, p->alias);
#ifndef CHECK_VNSTAT
		db_close();
		exit(EXIT_SUCCESS);
#endif
	} else {
		printf("Error: Setting interface \"%s\" alias failed.\n", p->interface);
		exit(EXIT_FAILURE);
	}
}

void handleshowdata(PARAMS *p)
{
	int ifcount = 0;
	iflist *dbifl = NULL, *dbifl_i = NULL;

	if (!p->query) {
		return;
	}

	/* show only specified file */
	if (!p->defaultiface) {
		showoneinterface(p);
		return;
	}

	/* show all interfaces if none is explicitly specified */
	if (p->dbifcount == 0) {
		p->query = 0;
	} else if ((cfg.qmode == 0 || cfg.qmode == 8 || cfg.qmode == 10) && (p->dbifcount > 1)) {

		if (cfg.qmode == 0) {
			if (cfg.ostyle != 0 && cfg.estimatevisible) {
				printf("\n                      rx      /      tx      /     total    /   %s\n", cfg.estimatetext);
			} else {
				printf("\n                      rx      /      tx      /     total\n");
			}
		} else if (cfg.qmode == 8) {
			xmlheader();
		} else if (cfg.qmode == 10) {
			jsonheader(JSONVERSION);
		}

		if (db_getiflist(&dbifl) <= 0) {
			return;
		}

		dbifl_i = dbifl;

		while (dbifl_i != NULL) {
			strncpy_nt(p->interface, dbifl_i->interface, MAXIFLEN);
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

		/* show in qmode if there's only one interface or qmode!=0 */
	} else {
		showoneinterface(p);
	}
}

void showoneinterface(PARAMS *p)
{
	validateinterface(p);

	if (cfg.qmode == 5) {
		if (cfg.ostyle != 0) {
			printf("\n                      rx      /      tx      /     total");
			if (cfg.estimatevisible) {
				printf("    /   %s", cfg.estimatetext);
			}
			printf("\n");
		} else {
			printf("\n                 rx      /      tx      /     total\n");
		}
	}
	if (cfg.qmode != 8 && cfg.qmode != 10) {
		showdb(p->interface, cfg.qmode, p->databegin, p->dataend);
	} else if (cfg.qmode == 8) {
		xmlheader();
		showxml(p->interface, p->xmlmode, p->databegin, p->dataend);
		xmlfooter();
	} else if (cfg.qmode == 10) {
		jsonheader(JSONVERSION);
		showjson(p->interface, 0, p->jsonmode, p->databegin, p->dataend);
		jsonfooter();
	}
}

void handletrafficmeters(PARAMS *p)
{
	int i;

	if (!p->traffic && !p->livetraffic) {
		return;
	}

	if (strchr(p->interface, '+') != NULL) {
		printf("This feature doesn't support interface merges (\"%s\"), ", p->interface);
		for (i = 0; i < (int)strlen(p->interface); i++) {
			if (p->interface[i] == '+') {
				p->interface[i] = '\0';
				break;
			}
		}
		p->defaultiface = 0;
		printf("using \"%s\" instead.\n", p->interface);
	}

	if (!isifavailable(p->interface)) {
		getifliststring(&p->ifacelist, 0);
		if (p->defaultiface) {
			printf("Error: Configured default interface \"%s\" isn't available.\n\n", p->interface);
			if (strlen(cfg.cfgfile)) {
				printf("Update \"Interface\" keyword value in configuration file \"%s\" to change ", cfg.cfgfile);
				printf("the default interface or give an alternative interface with or without the -i parameter.\n\n");
			} else {
				printf("An alternative interface can be given with or without the -i parameter.\n\n");
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

	if (!p->defaultiface) {
		return;
	}

	if (strlen(p->definterface)) {
		strncpy_nt(p->interface, p->definterface, MAXIFPARAMLEN);
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
					strncpy_nt(p->interface, dbifl_iterator->interface, MAXIFLEN);
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
				strncpy_nt(p->interface, ifl->interface, MAXIFLEN);
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
	} else if (p->query) {
		dbifcount = db_getiflist_sorted(&dbifl, 1);
		if (dbifcount <= 0) {
			if (dbifcount == 0) {
				printf("Error: Database is empty.\n");
			} else {
				printf("Error: Unable to discover suitable interface from database.\n");
			}
			iflistfree(&dbifl);
			exit(EXIT_FAILURE);
		}
		strncpy_nt(p->interface, dbifl->interface, MAXIFLEN);
		if (debug)
			printf("Automatically selected interface from db: \"%s\"\n", p->interface);
	}

	iflistfree(&dbifl);
}

void showiflist(const int parseable)
{
	char *ifacelist = NULL;
	iflist *ifl = NULL, *ifl_iterator = NULL;

	if (!parseable) {
		if (!getifliststring(&ifacelist, 1)) {
			exit(EXIT_FAILURE);
		}
		if (strlen(ifacelist)) {
			printf("Available interfaces: %s\n", ifacelist);
		} else {
			printf("No usable interfaces found.\n");
		}
		free(ifacelist);
	} else {
		if (!getiflist(&ifl, 0, 1)) {
			exit(EXIT_FAILURE);
		}
		ifl_iterator = ifl;
		while (ifl_iterator != NULL) {
			printf("%s\n", ifl_iterator->interface);
			ifl_iterator = ifl_iterator->next;
		}
		iflistfree(&ifl);
	}
}

void showdbiflist(const int parseable)
{
	int dbifcount;
	iflist *dbifl = NULL, *dbifl_i = NULL;

	if (db == NULL && !db_open_ro()) {
		printf("Error: Failed to open database \"%s/%s\" in read-only mode.\n", cfg.dbdir, DATABASEFILE);
		exit(EXIT_FAILURE);
	}

	dbifcount = db_getiflist(&dbifl);
	if (dbifcount < 0) {
		printf("Error: Failed to get interface list from database \"%s/%s\".\n", cfg.dbdir, DATABASEFILE);
		exit(EXIT_FAILURE);
	}

	if (dbifcount == 0 && !parseable) {
		printf("Database is empty.\n");
	} else {
		dbifl_i = dbifl;

		if (!parseable) {
			printf("Interfaces in database:");
			while (dbifl_i != NULL) {
				printf(" %s", dbifl_i->interface);
				dbifl_i = dbifl_i->next;
			}
			printf("\n");
		} else {
			while (dbifl_i != NULL) {
				printf("%s\n", dbifl_i->interface);
				dbifl_i = dbifl_i->next;
			}
		}
	}

	iflistfree(&dbifl);
	db_close();
}

void validateinterface(PARAMS *p)
{
	int i, found = 0;

	timeused_debug(__func__, 1);

	if (!db_getinterfacecountbyname(p->interface)) {
		if (strchr(p->interface, '+') == NULL) {
			for (i = 1; i <= cfg.ifacematchmethod; i++) {
				found = db_setinterfacebyalias(p->interface, p->interface, i);
				if (found) {
					if (debug) {
						printf("Found \"%s\" with method %d\n", p->interface, i);
					}
					break;
				}
			}
			if (!found) {
				printf("Error: No interface matching \"%s\" found in database.\n", p->interface);
				exit(EXIT_FAILURE);
			}
		} else {
			printf("Error: Not all requested interfaces found in database or given interfaces aren't unique.\n");
			exit(EXIT_FAILURE);
		}
	}

	timeused_debug(__func__, 0);
}
