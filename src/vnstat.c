/*
vnStat - Copyright (C) 2002-2022 Teemu Toivola <tst@iki.fi>

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
#include "dbsql.h"
#include "misc.h"
#include "cfg.h"
#include "ibw.h"
#include "vnstat_func.h"

int main(int argc, char *argv[])
{
	int currentarg;
	DIR *dir = NULL;
	PARAMS p;

	initparams(&p);

	/* early check for debug and config parameter */
	if (argc > 1) {
		for (currentarg = 1; currentarg < argc; currentarg++) {
			if ((strcmp(argv[currentarg], "-D") == 0) || (strcmp(argv[currentarg], "--debug") == 0)) {
				debug = 1;
				printf("Debug enabled, vnstat %s\n", VERSION);
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
	if (!loadcfg(p.cfgfile, CT_CLI)) {
		return 1;
	}
	if (!ibwloadcfg(p.cfgfile)) {
		return 1;
	}

	configlocale();
	strncpy_nt(p.interface, "default", 32);
	strncpy_nt(p.definterface, cfg.iface, 32);
	strncpy_nt(p.alias, "none", 32);

	parseargs(&p, argc, argv);

	/* open database and see if it contains any interfaces */
	if (!p.traffic && !p.livetraffic) {
		if ((dir = opendir(cfg.dbdir)) != NULL) {
			if (debug)
				printf("Dir OK\n");
			closedir(dir);
			if (!db_open_ro()) {
				printf("Error: Failed to open database \"%s/%s\" in read-only mode.\n", cfg.dbdir, DATABASEFILE);
				if (errno == ENOENT) {
					printf("The vnStat daemon should have created the database when started.\n");
					printf("Check that it is configured and running. See also \"man vnstatd\".\n");
				}
				return 1;
			}
			p.dbifcount = db_getinterfacecount();
			if (debug)
				printf("%" PRIu64 " interface(s) found\n", p.dbifcount);

			if (p.dbifcount > 1) {
				strncpy_nt(p.definterface, cfg.iface, 32);
			}
		} else {
			printf("Error: Unable to open database directory \"%s\": %s\n", cfg.dbdir, strerror(errno));
			if (errno == ENOENT) {
				printf("The vnStat daemon should have created this directory when started.\n");
				printf("Check that it is configured and running. See also \"man vnstatd\".\n");
			} else {
				printf("Make sure it is at least read enabled for current user.\n");
				printf("Use --help for help.\n");
			}
			return 1;
		}
	}

	/* set used interface if none specified */
	handleifselection(&p);

	/* parameter handlers */
	handleshowalert(&p);
	handleremoveinterface(&p);
	handlerenameinterface(&p);
	handleaddinterface(&p);
	handlesetalias(&p);
	handleshowdata(&p);
	handletrafficmeters(&p);

	/* show something if nothing was shown previously */
	if (!p.query && !p.traffic && !p.livetraffic) {

		/* give more help if there's no database */
		if (p.dbifcount == 0) {
			getifliststring(&p.ifacelist, 1);
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
	db_close();
	ibwflush();

	return 0;
}
