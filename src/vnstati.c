/*
vnStat image output - Copyright (C) 2007-2020 Teemu Toivola <tst@iki.fi>

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
#include "iflist.h"
#include "dbsql.h"
#include "image.h"
#include "cfg.h"
#include "misc.h"
#include "vnstati.h"

int main(int argc, char *argv[])
{
	int currentarg;
	IPARAMS p;
	IMAGECONTENT ic;

	initiparams(&p);
	initimagecontent(&ic);

	/* early check for debug and config parameter */
	if (argc > 1) {
		for (currentarg = 1; currentarg < argc; currentarg++) {
			if ((strcmp(argv[currentarg], "-D") == 0) || (strcmp(argv[currentarg], "--debug") == 0)) {
				debug = 1;
				printf("Debug enabled, vnstati %s\n", VERSION);
			} else if (strcmp(argv[currentarg], "--config") == 0) {
				if (currentarg + 1 < argc) {
					strncpy_nt(p.cfgfile, argv[currentarg + 1], 512);
					if (debug)
						printf("Used config file: %s\n", p.cfgfile);
					currentarg++;
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
	cfg.qmode = 0;

	configlocale();
	strncpy_nt(p.interface, cfg.iface, 32);
	ic.current = time(NULL);

	parseargs(&p, &ic, argc, argv);
	validateinput(&p);
	handlecaching(&p, &ic);
	handledatabase(&p, &ic);
	openoutput(&p);

	if (debug)
		printf("Qmode: %d\n", cfg.qmode);

	drawimage(&ic);
	db_close();
	writeoutput(&p, &ic);

	if (debug)
		printf("all done\n");

	return 0;
}

void initiparams(IPARAMS *p)
{
	db = NULL;
	noexit = 0;		   /* allow functions to exit in case of error */
	debug = 0;		   /* debug disabled by default */
	disableprints = 0; /* let prints be visible */

	p->interface[0] = '\0';
	p->filename[0] = '\0';
	p->cfgfile[0] = '\0';
	p->cache = 0;
	p->help = 0;
	p->limit = -1;
}

void showihelp(IPARAMS *p)
{
	printf("vnStat image output %s by Teemu Toivola <tst at iki dot fi>\n\n", getversion());

	printf("      -5,  --fiveminutes [limit]   output 5 minutes\n");
	printf("      -h,  --hours [limit]         output hours\n");
	printf("      -hg, --hoursgraph            output hours graph\n");
	printf("      -d,  --days [limit]          output days\n");
	printf("      -m,  --months [limit]        output months\n");
	printf("      -y,  --years [limit]         output years\n");
	printf("      -t,  --top [limit]           output top days\n");
	printf("      -s,  --summary               output summary\n");
	printf("      -hs, --hsummary              output horizontal summary with hours\n");
	printf("      -vs, --vsummary              output vertical summary with hours\n");
	printf("      -nh, --noheader              remove header from output\n");
	printf("      -ne, --noedge                remove edge from output\n");
	printf("      -nl, --nolegend              remove legend from output\n");
	printf("      -ru, --rateunit [mode]       swap configured rate unit\n");
	printf("      -o,  --output <file>         select output filename\n");
	printf("      -c,  --cache <minutes>       update output only when too old\n");
	printf("      -i,  --iface <interface>     select interface");
	if (strlen(p->interface)) {
		printf(" (default: %s)", p->interface);
	}
	printf("\n");
	printf("      -b,  --begin <date>          set list begin date\n");
	printf("      -e,  --end <date>            set list end date\n");
	printf("      -?,  --help                  this help\n");
	printf("      -D,  --debug                 show some additional debug information\n");
	printf("      -v,  --version               show version\n");
	printf("      --limit <limit>              set list entry limit\n");
	printf("      --dbdir <directory>          select database directory\n");
	printf("      --style <mode>               select output style (0-3)\n");
	printf("      --locale <locale>            set locale\n");
	printf("      --config <config file>       select config file\n");
	printf("      --altdate                    use alternative date location\n");
	printf("      --headertext <text>          specify header text string\n");
	printf("      --transparent [enabled]      toggle background transparency\n\n");

	printf("See also \"man vnstati\".\n");
}

void parseargs(IPARAMS *p, IMAGECONTENT *ic, int argc, char **argv)
{
	int currentarg;

	/* parse parameters */
	for (currentarg = 1; currentarg < argc; currentarg++) {
		if (debug)
			printf("arg %d: \"%s\"\n", currentarg, argv[currentarg]);
		if ((strcmp(argv[currentarg], "-?") == 0) || (strcmp(argv[currentarg], "--help")) == 0) {
			p->help = 1;
		} else if ((strcmp(argv[currentarg], "-i") == 0) || (strcmp(argv[currentarg], "--iface")) == 0) {
			if (currentarg + 1 < argc) {
				if (strlen(argv[currentarg + 1]) > 31) {
					printf("Error: Interface name is limited to 31 characters.\n");
					exit(EXIT_FAILURE);
				}
				strncpy_nt(p->interface, argv[currentarg + 1], 32);
				if (debug)
					printf("Used interface: \"%s\"\n", p->interface);
				currentarg++;
			} else {
				printf("Error: Interface for -i missing.\n");
				exit(EXIT_FAILURE);
			}
		} else if ((strcmp(argv[currentarg], "-o") == 0) || (strcmp(argv[currentarg], "--output")) == 0) {
			if (currentarg + 1 < argc) {
				strncpy_nt(p->filename, argv[currentarg + 1], 512);
				if (debug)
					printf("Output file: \"%s\"\n", p->filename);
				currentarg++;
			} else {
				printf("Error: Filename for -o missing.\n");
				exit(EXIT_FAILURE);
			}
		} else if ((strcmp(argv[currentarg], "-c") == 0) || (strcmp(argv[currentarg], "--cache")) == 0) {
			if (currentarg + 1 < argc && isdigit(argv[currentarg + 1][0])) {
				p->cache = atoi(argv[currentarg + 1]);
				if (debug)
					printf("Cache time: %d minutes\n", p->cache);
				currentarg++;
			} else {
				printf("Error: Parameter for -c missing or invalid.\n");
				exit(EXIT_FAILURE);
			}
		} else if ((strcmp(argv[currentarg], "--style")) == 0) {
			if (currentarg + 1 < argc && isdigit(argv[currentarg + 1][0])) {
				cfg.ostyle = atoi(argv[currentarg + 1]);
				if (cfg.ostyle > 3 || cfg.ostyle < 0) {
					printf("Error: Invalid style parameter \"%d\" for --style.\n", cfg.ostyle);
					exit(EXIT_FAILURE);
				}
				if (debug)
					printf("Style changed: %d\n", cfg.ostyle);
				currentarg++;
			} else {
				printf("Error: Style parameter for --style missing.\n");
				exit(EXIT_FAILURE);
			}
		} else if ((strcmp(argv[currentarg], "--transparent")) == 0) {
			if (currentarg + 1 < argc && isdigit(argv[currentarg + 1][0])) {
				cfg.transbg = atoi(argv[currentarg + 1]);
				if (cfg.transbg > 1 || cfg.transbg < 0) {
					printf("Error: Invalid parameter \"%d\" for --transparent.\n", cfg.transbg);
					exit(EXIT_FAILURE);
				}
				if (debug)
					printf("Transparency changed: %d\n", cfg.transbg);
				currentarg++;
			} else {
				cfg.transbg = !cfg.transbg;
				if (debug)
					printf("Transparency changed: %d\n", cfg.transbg);
			}
		} else if ((strcmp(argv[currentarg], "--dbdir")) == 0) {
			if (currentarg + 1 < argc) {
				strncpy_nt(cfg.dbdir, argv[currentarg + 1], 512);
				if (debug)
					printf("DatabaseDir: \"%s\"\n", cfg.dbdir);
				currentarg++;
			} else {
				printf("Error: Directory for --dbdir missing.\n");
				exit(EXIT_FAILURE);
			}
		} else if ((strcmp(argv[currentarg], "--locale")) == 0) {
			if (currentarg + 1 < argc) {
				setlocale(LC_ALL, argv[currentarg + 1]);
				if (debug)
					printf("Locale: \"%s\"\n", argv[currentarg + 1]);
				currentarg++;
			} else {
				printf("Error: Locale for --locale missing.\n");
				exit(EXIT_FAILURE);
			}
		} else if (strcmp(argv[currentarg], "--config") == 0) {
			/* config has already been parsed earlier so nothing to do here */
			currentarg++;
		} else if ((strcmp(argv[currentarg], "--headertext")) == 0) {
			if (currentarg + 1 < argc) {
				strncpy_nt(ic->headertext, argv[currentarg + 1], 65);
				if (debug)
					printf("Header text: \"%s\"\n", ic->headertext);
				currentarg++;
			} else {
				printf("Error: Text string parameter for --headertext missing.\n");
				exit(EXIT_FAILURE);
			}
		} else if (strcmp(argv[currentarg], "--altdate") == 0) {
			ic->altdate = 1;
		} else if ((strcmp(argv[currentarg], "-D") == 0) || (strcmp(argv[currentarg], "--debug")) == 0) {
			debug = 1;
		} else if ((strcmp(argv[currentarg], "-d") == 0) || (strcmp(argv[currentarg], "--days")) == 0) {
			cfg.qmode = 1;
			if (currentarg + 1 < argc && isdigit(argv[currentarg + 1][0])) {
				cfg.listdays = atoi(argv[currentarg + 1]);
				if (cfg.listdays < 0) {
					printf("Error: Invalid limit parameter \"%s\" for %s. Only a zero and positive numbers are allowed.\n", argv[currentarg + 1], argv[currentarg]);
					exit(EXIT_FAILURE);
				}
				currentarg++;
			}
		} else if ((strcmp(argv[currentarg], "-m") == 0) || (strcmp(argv[currentarg], "--months")) == 0) {
			cfg.qmode = 2;
			if (currentarg + 1 < argc && isdigit(argv[currentarg + 1][0])) {
				cfg.listmonths = atoi(argv[currentarg + 1]);
				if (cfg.listmonths < 0) {
					printf("Error: Invalid limit parameter \"%s\" for %s. Only a zero and positive numbers are allowed.\n", argv[currentarg + 1], argv[currentarg]);
					exit(EXIT_FAILURE);
				}
				currentarg++;
			}
		} else if ((strcmp(argv[currentarg], "-t") == 0) || (strcmp(argv[currentarg], "--top")) == 0) {
			cfg.qmode = 3;
			if (currentarg + 1 < argc && isdigit(argv[currentarg + 1][0])) {
				cfg.listtop = atoi(argv[currentarg + 1]);
				if (cfg.listtop < 0) {
					printf("Error: Invalid limit parameter \"%s\" for %s. Only a zero and positive numbers are allowed.\n", argv[currentarg + 1], argv[currentarg]);
					exit(EXIT_FAILURE);
				}
				currentarg++;
			}
		} else if ((strcmp(argv[currentarg], "-y") == 0) || (strcmp(argv[currentarg], "--years")) == 0) {
			cfg.qmode = 4;
			if (currentarg + 1 < argc && isdigit(argv[currentarg + 1][0])) {
				cfg.listyears = atoi(argv[currentarg + 1]);
				if (cfg.listyears < 0) {
					printf("Error: Invalid limit parameter \"%s\" for %s. Only a zero and positive numbers are allowed.\n", argv[currentarg + 1], argv[currentarg]);
					exit(EXIT_FAILURE);
				}
				currentarg++;
			}
		} else if ((strcmp(argv[currentarg], "-h") == 0) || (strcmp(argv[currentarg], "--hours")) == 0) {
			cfg.qmode = 8;
			if (currentarg + 1 < argc && isdigit(argv[currentarg + 1][0])) {
				cfg.listhours = atoi(argv[currentarg + 1]);
				if (cfg.listhours < 0) {
					printf("Error: Invalid limit parameter \"%s\" for %s. Only a zero and positive numbers are allowed.\n", argv[currentarg + 1], argv[currentarg]);
					exit(EXIT_FAILURE);
				}
				currentarg++;
			}
		} else if ((strcmp(argv[currentarg], "-5") == 0) || (strcmp(argv[currentarg], "--fiveminutes")) == 0) {
			cfg.qmode = 9;
			if (currentarg + 1 < argc && isdigit(argv[currentarg + 1][0])) {
				cfg.listfivemins = atoi(argv[currentarg + 1]);
				if (cfg.listfivemins < 0) {
					printf("Error: Invalid limit parameter \"%s\" for %s. Only a zero and positive numbers are allowed.\n", argv[currentarg + 1], argv[currentarg]);
					exit(EXIT_FAILURE);
				}
				currentarg++;
			}
		} else if ((strcmp(argv[currentarg], "-s") == 0) || (strcmp(argv[currentarg], "--summary")) == 0) {
			cfg.qmode = 5;
		} else if ((strcmp(argv[currentarg], "-hg") == 0) || (strcmp(argv[currentarg], "--hoursgraph")) == 0) {
			cfg.qmode = 7;
		} else if ((strcmp(argv[currentarg], "-hs") == 0) || (strcmp(argv[currentarg], "--hsummary")) == 0) {
			cfg.qmode = 51;
		} else if ((strcmp(argv[currentarg], "-vs") == 0) || (strcmp(argv[currentarg], "--vsummary")) == 0) {
			cfg.qmode = 52;
		} else if ((strcmp(argv[currentarg], "-nh") == 0) || (strcmp(argv[currentarg], "--noheader")) == 0) {
			ic->showheader = 0;
		} else if ((strcmp(argv[currentarg], "-ne") == 0) || (strcmp(argv[currentarg], "--noedge")) == 0) {
			ic->showedge = 0;
		} else if ((strcmp(argv[currentarg], "-nl") == 0) || (strcmp(argv[currentarg], "--nolegend")) == 0) {
			ic->showlegend = 0;
		} else if ((strcmp(argv[currentarg], "-ru") == 0) || (strcmp(argv[currentarg], "--rateunit")) == 0) {
			if (currentarg + 1 < argc && isdigit(argv[currentarg + 1][0])) {
				cfg.rateunit = atoi(argv[currentarg + 1]);
				if (cfg.rateunit > 1 || cfg.rateunit < 0) {
					printf("Error: Invalid parameter \"%d\" for --rateunit.\n", cfg.rateunit);
					printf(" Valid parameters:\n");
					printf("    0 - bytes\n");
					printf("    1 - bits\n");
					exit(EXIT_FAILURE);
				}
				if (debug)
					printf("Rateunit changed: %d\n", cfg.rateunit);
				currentarg++;
			} else {
				cfg.rateunit = !cfg.rateunit;
				if (debug)
					printf("Rateunit changed: %d\n", cfg.rateunit);
			}
		} else if ((strcmp(argv[currentarg], "-b") == 0) || (strcmp(argv[currentarg], "--begin") == 0)) {
			if (currentarg + 1 < argc) {
				if (!validatedatetime(argv[currentarg + 1])) {
					printf("Error: Invalid date format, expected YYYY-MM-DD HH:MM or YYYY-MM-DD.\n");
					exit(EXIT_FAILURE);
				}
				strncpy_nt(ic->databegin, argv[currentarg + 1], 18);
				currentarg++;
			} else {
				printf("Error: Date of format YYYY-MM-DD HH:MM or YYYY-MM-DD for %s missing.\n", argv[currentarg]);
				exit(EXIT_FAILURE);
			}
		} else if ((strcmp(argv[currentarg], "-e") == 0) || (strcmp(argv[currentarg], "--end") == 0)) {
			if (currentarg + 1 < argc) {
				if (!validatedatetime(argv[currentarg + 1])) {
					printf("Error: Invalid date format, expected YYYY-MM-DD HH:MM or YYYY-MM-DD.\n");
					exit(EXIT_FAILURE);
				}
				strncpy_nt(ic->dataend, argv[currentarg + 1], 18);
				currentarg++;
			} else {
				printf("Error: Date of format YYYY-MM-DD HH:MM or YYYY-MM-DD for %s missing.\n", argv[currentarg]);
				exit(EXIT_FAILURE);
			}
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
		} else if ((strcmp(argv[currentarg], "-v") == 0) || (strcmp(argv[currentarg], "--version")) == 0) {
			printf("vnStat image output %s by Teemu Toivola <tst at iki dot fi>\n", getversion());
			exit(EXIT_SUCCESS);
		} else {
			printf("Unknown arg \"%s\". Use --help for help.\n", argv[currentarg]);
			exit(EXIT_FAILURE);
		}
	}

	if (p->help || argc == 1) {
		showihelp(p);
		exit(EXIT_SUCCESS);
	}

	if (p->limit != -1) {
		cfg.listfivemins = cfg.listhours = cfg.listdays = cfg.listmonths = cfg.listyears = cfg.listtop = cfg.listjsonxml = p->limit;
	}
}

void validateinput(IPARAMS *p)
{
	if (!cfg.qmode || !strlen(p->filename)) {
		printf("At least output mode and file parameter needs to be given.\n");
		printf("Use -? or --help for getting short help.\n");
		exit(EXIT_FAILURE);
	}
}

void handlecaching(IPARAMS *p, IMAGECONTENT *ic)
{
	struct stat filestat;

	if (p->cache == 0 || p->filename[0] == '-') {
		return;
	}

	if (stat(p->filename, &filestat) == 0) {
		if ((ic->current - filestat.st_mtime) < (p->cache * 60)) {
			if (debug)
				printf("Using cached file (%d<%d).\n", (int)(ic->current - filestat.st_mtime), p->cache * 60);
			exit(EXIT_SUCCESS);
		}
	} else {
		/* abort if error is something else than file not found */
		if (errno != ENOENT) {
			printf("Error: Getting status for file \"%s\" failed: %s (%d)\n", p->filename, strerror(errno), errno);
			exit(EXIT_FAILURE);
		}
	}
}

void handledatabase(IPARAMS *p, IMAGECONTENT *ic)
{
	iflist *dbifl = NULL;

	if (!db_open_ro()) {
		printf("Error: Failed to open database \"%s/%s\" in read-only mode.\n", cfg.dbdir, DATABASEFILE);
		exit(EXIT_FAILURE);
	}
	if (strlen(p->interface)) {
		if (!db_getinterfacecountbyname(p->interface)) {
			if (strchr(p->interface, '+') == NULL) {
				printf("Error: Interface \"%s\" not found in database.\n", p->interface);
			} else {
				printf("Error: Not all requested interfaces found in database or given interfaces aren't unique.\n");
			}
			exit(EXIT_FAILURE);
		}
	} else {
		if (db_getiflist_sorted(&dbifl, 1) <= 0) {
			printf("Error: Unable to discover suitable interface from database.\n");
			exit(EXIT_FAILURE);
		}
		strncpy_nt(p->interface, dbifl->interface, 32);
		iflistfree(&dbifl);
		if (debug)
			printf("Automatically selected interface: \"%s\"\n", p->interface);
	}
	if (!db_getinterfaceinfo(p->interface, &ic->interface)) {
		printf("Error: Failed to fetch interface \"%s\" info from database.\n", p->interface);
		exit(EXIT_FAILURE);
	}
}

void openoutput(IPARAMS *p)
{
	if (p->filename[0] != '-') {
		if ((p->pngout = fopen(p->filename, "w")) == NULL) {
			printf("Error: Opening file \"%s\" for output failed: %s\n", p->filename, strerror(errno));
			exit(EXIT_FAILURE);
		}
	} else {
		/* output to stdout */
		if ((p->pngout = fdopen(1, "w")) == NULL) {
			printf("Error: Opening stdout for output failed: %s\n", strerror(errno));
			exit(EXIT_FAILURE);
		}
	}
}

void writeoutput(IPARAMS *p, IMAGECONTENT *ic)
{
	gdImagePng(ic->im, p->pngout);
	fclose(p->pngout);
	gdImageDestroy(ic->im);
}
