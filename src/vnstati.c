/*
vnStat image output - Copyright (c) 2007-09 Teemu Toivola <tst@iki.fi>

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
#include "image.h"
#include "cfg.h"
#include "dbaccess.h"
#include "vnstati.h"

int main(int argc, char *argv[])
{
	FILE *pngout;
	int currentarg, cache=0, help=0, showheader=1, showedge=1, showrate=0;
	char interface[32], dirname[512], filename[512], cfgfile[512];
	struct stat filestat;

	noexit = 0; /* allow functions to exit in case of error */
	debug = 0; /* debug disabled by default */
	cfgfile[0] = '\0';

	/* early check for debug and config parameter */
	if (argc > 1) {
		for (currentarg=1; currentarg<argc; currentarg++) {
			if ((strcmp(argv[currentarg],"-D")==0) || (strcmp(argv[currentarg],"--debug")==0)) {
				debug = 1;
			} else if (strcmp(argv[currentarg],"--config")==0) {
				if (currentarg+1<argc) {
					strncpy(cfgfile, argv[currentarg+1], 512);
					if (debug)
						printf("Used config file: %s\n", cfgfile);
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
	if (!loadcfg(cfgfile)) {
		return 1;
	}
	cfg.qmode = 0;

	setlocale(LC_ALL, cfg.locale);
	strncpy(interface, cfg.iface, 32);
	strncpy(dirname, cfg.dbdir, 512);
	filename[0] = '\0';
	current = time(NULL);

	/* parse parameters */
	for (currentarg=1; currentarg<argc; currentarg++) {
		if (debug)
			printf("arg %d: \"%s\"\n",currentarg,argv[currentarg]);
		if ((strcmp(argv[currentarg],"-?")==0) || (strcmp(argv[currentarg],"--help"))==0) {
			help = 1;
		} else if ((strcmp(argv[currentarg],"-i")==0) || (strcmp(argv[currentarg],"--iface"))==0) {
			if (currentarg+1<argc) {
				strncpy(interface, argv[currentarg+1], 32);
				if (debug)
					printf("Used interface: \"%s\"\n", filename);
				currentarg++;
				continue;
			} else {
				printf("Error: Interface for -i missing.\n");
				return 1;
			}
		} else if ((strcmp(argv[currentarg],"-o")==0) || (strcmp(argv[currentarg],"--output"))==0) {
			if (currentarg+1<argc) {
				strncpy(filename, argv[currentarg+1], 512);
				if (debug)
					printf("Output file: \"%s\"\n", filename);
				currentarg++;
				continue;
			} else {
				printf("Error: Filename for -o missing.\n");
				return 1;
			}			
		} else if ((strcmp(argv[currentarg],"-c")==0) || (strcmp(argv[currentarg],"--cache"))==0) {
			if (currentarg+1<argc && isdigit(argv[currentarg+1][0])) {
				cache = atoi(argv[currentarg+1]);
				if (debug)
					printf("Cache time: %d minutes\n", cache);
				currentarg++;
				continue;
			} else {
				printf("Error: Parameter for -c missing or invalid.\n");
				return 1;
			}
		} else if (strcmp(argv[currentarg],"--config")==0) {
			/* config has already been parsed earlier so not but to do here*/
			currentarg++;
			continue;				
		} else if ((strcmp(argv[currentarg],"-D")==0) || (strcmp(argv[currentarg],"--debug"))==0) {
			debug = 1;
		} else if ((strcmp(argv[currentarg],"-d")==0) || (strcmp(argv[currentarg],"--days"))==0) {
			cfg.qmode = 1;
		} else if ((strcmp(argv[currentarg],"-m")==0) || (strcmp(argv[currentarg],"--months"))==0) {
			cfg.qmode = 2;
		} else if ((strcmp(argv[currentarg],"-t")==0) || (strcmp(argv[currentarg],"--top10"))==0) {
			cfg.qmode = 3;
		} else if ((strcmp(argv[currentarg],"-s")==0) || (strcmp(argv[currentarg],"--summary"))==0) {
			cfg.qmode = 5;
		} else if ((strcmp(argv[currentarg],"-h")==0) || (strcmp(argv[currentarg],"--hours"))==0) {
			cfg.qmode = 7;
		} else if ((strcmp(argv[currentarg],"-hs")==0) || (strcmp(argv[currentarg],"--hsummary"))==0) {
			cfg.qmode = 51;
		} else if ((strcmp(argv[currentarg],"-vs")==0) || (strcmp(argv[currentarg],"--vsummary"))==0) {
			cfg.qmode = 52;
		} else if ((strcmp(argv[currentarg],"-nh")==0) || (strcmp(argv[currentarg],"--noheader"))==0) {
			showheader = 0;
		} else if ((strcmp(argv[currentarg],"-ne")==0) || (strcmp(argv[currentarg],"--noedge"))==0) {
			showedge = 0;
		} else if ((strcmp(argv[currentarg],"-r")==0) || (strcmp(argv[currentarg],"--rate"))==0) {
			showrate = 1;
		} else if ((strcmp(argv[currentarg],"-v")==0) || (strcmp(argv[currentarg],"--version"))==0) {
			printf("vnStat image output %s by Teemu Toivola <tst at iki dot fi>\n", VNSTATVERSION);
			return 0;
		} else {
			printf("Unknown arg \"%s\". Use --help for help.\n",argv[currentarg]);
			return 1;
		}
	}

	if (help || argc == 1) {
		printf(" vnStat image output %s by Teemu Toivola <tst at iki dot fi>\n\n", VNSTATVERSION);
		printf("         -h,  --hours             output hours\n");
		printf("         -d,  --days              output days\n");
		printf("         -m,  --months            output months\n");
		printf("         -t,  --top10             output top10\n");
		printf("         -s,  --summary           output summary\n");
		printf("         -hs, --hsummary          output horizontal summary with hours\n");
		printf("         -vs, --vsummary          output vertical summary with hours\n");
		printf("         -nh, --noheader          remove header from output\n");
		printf("         -ne, --noedge            remove edge from output\n");
		printf("         -r,  --rate              show hours with rate\n");
		printf("         -o,  --output            select output filename\n");
		printf("         -c,  --cache             update output only when too old\n");
		printf("         -i,  --iface             used interface (default: %s)\n", interface);
		printf("         -?,  --help              this help\n");
		printf("         -D,  --debug             show some additional debug information\n");
		printf("         -v,  --version           show version\n");
		printf("         --config                 select used config file\n\n");
		printf("See also \"man vnstati\".\n");
		return 0;
	}

	/* validate input */
	if (!cfg.qmode || filename[0]=='\0') {
		printf("At least output mode and file parameter needs to be given.\n");
		printf("Use -? or --help for getting short help.\n");
		return 1;
	}

	/* check caching */
	if (cache>0 && filename[0]!='-') {
		if (stat(filename, &filestat)==0) {
			if ((current-filestat.st_mtime)<(cache*60)) {
				if (debug)
					printf("Using cached file (%d<%d).\n", (int)(current-filestat.st_mtime), cache*60);
				return 0;
			}
		} else {
			/* abort if error is something else than file not found */
			if (errno!=ENOENT) {
				printf("Error: Getting status for file \"%s\" failed: %s (%d)\n", filename, strerror(errno), errno);
				return 1;
			}
		}
	}

	/* load database */
	if (readdb(interface, dirname)==1) {
		return 1;
	}

	/* open file */	
	if (filename[0]!='-') {
		if ((pngout = fopen(filename, "w"))==NULL) {
			printf("Error: Opening file \"%s\" for output failed.\n", filename);
			return 1;
		}
	} else {
		/* output to stdout */
		if ((pngout = fdopen(1, "w"))==NULL) {
			printf("Error: Opening stdout for output failed.\n");
			return 1;
		}
	}

	if (debug)
		printf("Qmode: %d\n", cfg.qmode);

	/* draw image */
	switch (cfg.qmode) {
		case 1:
			drawdaily(showheader, showedge);
			break;
		case 2:
			drawmonthly(showheader, showedge);
			break;
		case 3:
			drawtop(showheader, showedge);
			break;
		case 5:
			drawsummary(0, showheader, showedge, showrate);
			break;
		case 51:
			drawsummary(1, showheader, showedge, showrate);
			break;
		case 52:
			drawsummary(2, showheader, showedge, showrate);
			break;
		case 7:
			drawhourly(showheader, showedge, showrate);
			break;
		default:
			break;
	}

	/* write image */
	gdImagePng(im, pngout);
	fclose(pngout);
	gdImageDestroy(im);

	if (debug)
		printf("all done\n");

	return 0;
}
