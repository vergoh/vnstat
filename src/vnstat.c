/*
vnStat - Copyright (c) 2002-11 Teemu Toivola <tst@iki.fi>

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
#include "ifinfo.h"
#include "traffic.h"
#include "dbxml.h"
#include "dbshow.h"
#include "dbaccess.h"
#include "dbmerge.h"
#include "misc.h"
#include "cfg.h"
#include "vnstat.h"

int main(int argc, char *argv[]) {

	int i;
	int currentarg, update=0, query=1, newdb=0, reset=0, sync=0, merged=0, savemerged=0;
	int active=-1, files=0, force=0, cleartop=0, rebuildtotal=0, traffic=0;
	int livetraffic=0, defaultiface=1, delete=0, livemode=0;
	char interface[32], dirname[512], nick[32];
	char definterface[32], cfgfile[512], *ifacelist=NULL;
	time_t current;
	DIR *dir=NULL;
	struct dirent *di=NULL;

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

	if (cfg.locale[0]!='-' && strlen(cfg.locale)>0) {
		setlocale(LC_ALL, cfg.locale);
	} else {
		if (getenv("LC_ALL")) {
			setlocale(LC_ALL, getenv("LC_ALL"));
		}
	}
	strncpy(interface, "default", 32);
	strncpy(definterface, cfg.iface, 32);
	strncpy(nick, "none", 32);

	current = time(NULL);

	/* init dirname */
	strncpy(dirname, cfg.dbdir, 512);

	/* parse parameters, maybe not the best way but... */
	for (currentarg=1; currentarg<argc; currentarg++) {
		if (debug)
			printf("arg %d: \"%s\"\n",currentarg,argv[currentarg]);
		if (strcmp(argv[currentarg],"--longhelp")==0) {

			printf(" vnStat %s by Teemu Toivola <tst at iki dot fi>\n\n", VNSTATVERSION);

			printf("   Update:\n");
			printf("         -u, --update          update database\n");
			printf("         -r, --reset           reset interface counters\n");
			printf("         --sync                sync interface counters\n");
			printf("         --enable              enable interface\n");
			printf("         --disable             disable interface\n");
			printf("         --nick                set a nickname for interface\n");
			printf("         --cleartop            clear the top10\n");
			printf("         --rebuildtotal        rebuild total transfers from months\n");

			printf("   Query:\n");
			printf("         -q, --query           query database\n");
			printf("         -h, --hours           show hours\n");
			printf("         -d, --days            show days\n");
			printf("         -m, --months          show months\n");
			printf("         -w, --weeks           show weeks\n");
			printf("         -t, --top10           show top10\n");
			printf("         -s, --short           use short output\n");
			printf("         -ru, --rateunit       swap configured rate unit\n");
			printf("         --oneline             show simple parseable format\n");
			printf("         --dumpdb              show database in parseable format\n");
			printf("         --xml                 show database in xml format\n");

			printf("   Misc:\n");
			printf("         -i,  --iface          select interface (default: %s)\n", definterface);
			printf("         -?,  --help           short help\n");
			printf("         -D,  --debug          show some additional debug information\n");
			printf("         -v,  --version        show version\n");
			printf("         -tr, --traffic        calculate traffic\n");
			printf("         -l,  --live           show transfer rate in real time\n");
			printf("         --style               select output style (0-4)\n");
			printf("         --delete              delete database and stop monitoring\n");
			printf("         --iflist              show list of available interfaces\n");
			printf("         --dbdir               select database directory\n");
			printf("         --locale              set locale\n");
			printf("         --config              select config file\n");
			printf("         --savemerged          save merged database to current directory\n");
			printf("         --showconfig          dump config file with current settings\n");
			printf("         --testkernel          check if the kernel is broken\n");
			printf("         --longhelp            display this help\n\n");

			printf("See also \"man vnstat\".\n");

			return 0;

		} else if ((strcmp(argv[currentarg],"-?")==0) || (strcmp(argv[currentarg],"--help")==0)) {

			printf(" vnStat %s by Teemu Toivola <tst at iki dot fi>\n\n", VNSTATVERSION);

			printf("         -q,  --query          query database\n");
			printf("         -h,  --hours          show hours\n");
			printf("         -d,  --days           show days\n");
			printf("         -m,  --months         show months\n");
			printf("         -w,  --weeks          show weeks\n");
			printf("         -t,  --top10          show top10\n");
			printf("         -s,  --short          use short output\n");
			printf("         -u,  --update         update database\n");			
			printf("         -i,  --iface          select interface (default: %s)\n", definterface);
			printf("         -?,  --help           short help\n");
			printf("         -v,  --version        show version\n");
			printf("         -tr, --traffic        calculate traffic\n");
			printf("         -ru, --rateunit       swap configured rate unit\n");
			printf("         -l,  --live           show transfer rate in real time\n\n");

			printf("See also \"--longhelp\" for complete options list and \"man vnstat\".\n");

			return 0;

		} else if ((strcmp(argv[currentarg],"-i")==0) || (strcmp(argv[currentarg],"--iface")==0)) {
			if (currentarg+1<argc) {
				strncpy(interface, argv[currentarg+1], 32);
				defaultiface = 0;
				if (debug)
					printf("Used interface: %s\n", interface);
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
				strncpy(nick, argv[currentarg+1], 32);
				if (debug)
					printf("Used nick: %s\n", nick);
				currentarg++;
				continue;
			} else {
				printf("Error: Nick for --nick missing.\n");
				return 1;
			}
		} else if ((strcmp(argv[currentarg],"--style"))==0) {
			if (currentarg+1<argc && isdigit(argv[currentarg+1][0])) {
				cfg.ostyle = atoi(argv[currentarg+1]);
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
				strncpy(dirname, argv[currentarg+1], 512);
				if (debug)
					printf("DatabaseDir: \"%s\"\n", dirname);
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
		} else if ((strcmp(argv[currentarg],"-u")==0) || (strcmp(argv[currentarg],"--update")==0)) {
			update=1;
			query=0;
			if (debug)
				printf("Updating database...\n");
		} else if ((strcmp(argv[currentarg],"-q")==0) || (strcmp(argv[currentarg],"--query")==0)) {
			query=1;
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
		} else if (strcmp(argv[currentarg],"--dumpdb")==0) {
			cfg.qmode=4;
		} else if (strcmp(argv[currentarg],"--oneline")==0) {
			cfg.qmode=9;
		} else if (strcmp(argv[currentarg],"--xml")==0) {
			cfg.qmode=8;
		} else if (strcmp(argv[currentarg],"--savemerged")==0) {
			savemerged=1;
		} else if ((strcmp(argv[currentarg],"-ru")==0) || (strcmp(argv[currentarg],"--rateunit"))==0) {
			if (currentarg+1<argc && isdigit(argv[currentarg+1][0])) {
				cfg.rateunit = atoi(argv[currentarg+1]);
				if (cfg.rateunit > 1 || cfg.rateunit < 0) {
					printf("Error: Invalid parameter \"%d\" for --rateunit.\n", cfg.rateunit);
					printf(" Valid parameters:\n");
					printf("    0 - bytes\n");
					printf("    1 - bits\n");
					return 1;
				}
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
			active=1;
			query=0;
		} else if ((strcmp(argv[currentarg],"-tr")==0) || (strcmp(argv[currentarg],"--traffic")==0)) {
			if (currentarg+1<argc) {
				if (isdigit(argv[currentarg+1][0])) {
					cfg.sampletime=atoi(argv[currentarg+1]);
					currentarg++;
					traffic=1;
					query=0;
					continue;
				}
			}
			traffic=1;
			query=0;
		} else if ((strcmp(argv[currentarg],"-l")==0) || (strcmp(argv[currentarg],"--live")==0)) {
			if (currentarg+1<argc && argv[currentarg+1][0]!='-') {
				livemode = atoi(argv[currentarg+1]);
				if (!isdigit(argv[currentarg+1][0]) || livemode > 1 || livemode < 0) {
					printf("Error: Invalid mode parameter \"%s\" for -l / --live.\n", argv[currentarg+1]);
					printf(" Valid parameters:\n");
					printf("    0 - show packets per second (default)\n");
					printf("    1 - show transfer counters\n");
					return 1;
				}
				currentarg++;
			}
			livetraffic=1;
			query=0;
		} else if (strcmp(argv[currentarg],"--force")==0) {
			force=1;
		} else if (strcmp(argv[currentarg],"--cleartop")==0) {
			cleartop=1;
		} else if (strcmp(argv[currentarg],"--rebuildtotal")==0) {
			rebuildtotal=1;
		} else if (strcmp(argv[currentarg],"--disable")==0) {
			active=0;
			query=0;
		} else if (strcmp(argv[currentarg],"--testkernel")==0) {
			i=kerneltest();
			return i;
		} else if (strcmp(argv[currentarg],"--showconfig")==0) {
			printcfgfile();
			return 0;			
		} else if (strcmp(argv[currentarg],"--delete")==0) {
			delete=1;
			query=0;
		} else if (strcmp(argv[currentarg],"--iflist")==0) {
			getiflist(&ifacelist);
			printf("Available interfaces: %s\n", ifacelist);
			free(ifacelist);
			return 0;	
		} else if ((strcmp(argv[currentarg],"-v")==0) || (strcmp(argv[currentarg],"--version")==0)) {
			printf("vnStat %s by Teemu Toivola <tst at iki dot fi>\n", VNSTATVERSION);
			return 0;
		} else if ((strcmp(argv[currentarg],"-r")==0) || (strcmp(argv[currentarg],"--reset")==0)) {
			reset=1;
			query=0;
		} else if (strcmp(argv[currentarg],"--sync")==0) {
			sync=1;
			query=0;			
		} else {
			printf("Unknown parameter \"%s\". Use --help for help.\n",argv[currentarg]);
			return 1;
		}

	}

	/* check if the database dir exists and if it contains files */
	if (!traffic && !livetraffic) {
		if ((dir=opendir(dirname))!=NULL) {
			if (debug)
				printf("Dir OK\n");
			while ((di=readdir(dir))) {
				if (di->d_name[0]!='.') {
					strncpy(definterface, di->d_name, 32);
					files++;
				}
			}
			if (debug)
				printf("%d file(s) found\n", files);
			if (files>1) {
				strncpy(definterface, cfg.iface, 32);
			}
			closedir(dir);
		} else {
			printf("Error: Unable to open database directory \"%s\".\n", dirname);
			printf("Make sure it exists and is at least read enabled for current user.\n");
			printf("Exiting...\n");
			return 1;
		}
	}

	/* set used interface if none specified and make sure it's null terminated */
	if (defaultiface) {
		strncpy(interface, definterface, 32);
	}
	interface[31]='\0';

	/* db merge */
	if (query && strstr(interface, "+")) {
		if (mergedb(interface, dirname)) {
			files = merged = 1;
		} else {
			return 1;
		}
	}

	/* save merged database */
	if (merged && savemerged) {
		data.lastupdated = 0;
		if (writedb("mergeddb", ".", 2)) {
			printf("Database saved as \"mergeddb\" in the current directory.\n");
		}
		return 0;
	}

	/* counter reset */
	if (reset) {
		if (!spacecheck(dirname) && !force) {
			printf("Error: Not enough free diskspace available.\n");
			return 1;
		}
		readdb(interface, dirname);
		data.currx=0;
		data.curtx=0;
		writedb(interface, dirname, 0);
		if (debug)
			printf("Counters reseted for \"%s\"\n", data.interface);
	}

	/* counter sync */
	if (sync) {
		if (!spacecheck(dirname) && !force) {
			printf("Error: Not enough free diskspace available.\n");
			return 1;
		}
		if (!synccounters(interface, dirname)) {
			return 1;
		}
		if (debug)
			printf("Counters synced for \"%s\"\n", data.interface);
	}

	/* delete */
	if (delete) {
		if (force) {
			if (checkdb(interface, dirname)) {
				if (removedb(interface, dirname)) {
					printf("Database for interface \"%s\" deleted.\n", interface);
					printf("The interface will no longer be monitored.\n");
					return 0;
				} else {
					printf("Error: Deleting database for interface \"%s\" failed.\n", interface);
					return 1;
				}
			} else {
					printf("Error: No database found for interface \"%s\".\n", interface);
					return 1;				
			}
		} else {
			printf("Warning:\nThe current option would delete the database for \"%s\".\n", interface);
			printf("Use --force in order to really do that.\n");
			return 1;
		}		
	}

	/* clear top10 */
	if (cleartop) {
		if (!spacecheck(dirname) && !force) {
			printf("Error: Not enough free diskspace available.\n");
			return 1;
		}
		if (force) {
			readdb(interface, dirname);

			for (i=0; i<=9; i++) {
				data.top10[i].rx=data.top10[i].tx=0;
				data.top10[i].rxk=data.top10[i].txk=0;
				data.top10[i].used=0;
			}

			writedb(interface, dirname, 0);
			printf("Top10 cleared for interface \"%s\".\n", data.interface);
			query=0;
		} else {
			printf("Warning:\nThe current option would clear the top10 for \"%s\".\n", interface);
			printf("Use --force in order to really do that.\n");
			return 1;
		}
	}

	/* rebuild total */
	if (rebuildtotal) {
		if (!spacecheck(dirname)) {
			printf("Error: Not enough free diskspace available.\n");
			return 1;
		}
		if (force) {
			readdb(interface, dirname);

			data.totalrx=data.totaltx=data.totalrxk=data.totaltxk=0;
			for (i=0; i<=29; i++) {
				if (data.month[i].used) {
					addtraffic(&data.totalrx, &data.totalrxk, data.month[i].rx, data.month[i].rxk);
					addtraffic(&data.totaltx, &data.totaltxk, data.month[i].tx, data.month[i].txk);
				}
			}

			writedb(interface, dirname, 0);
			printf("Total transfer rebuild completed for interface \"%s\".\n", data.interface);
			query=0;
		} else {
			printf("Warning:\nThe current option would rebuild total tranfers for \"%s\".\n", interface);
			printf("Use --force in order to really do that.\n");
			return 1;
		}
	}

	/* enable & disable */
	if (active==1) {
		if (!spacecheck(dirname) && !force) {
			printf("Error: Not enough free diskspace available.\n");
			return 1;
		}
		newdb=readdb(interface, dirname);
		if (!data.active && !newdb) {
			data.active=1;
			writedb(interface, dirname, 0);
			if (debug)
				printf("Interface \"%s\" enabled.\n", data.interface);
		} else if (!newdb) {
			printf("Interface \"%s\" is already enabled.\n", data.interface);
		}
	} else if (active==0) {
		if (!spacecheck(dirname) && !force) {
			printf("Error: Not enough free diskspace available.\n");
			return 1;
		}
		newdb=readdb(interface, dirname);
		if (data.active && !newdb) {
			data.active=0;
			writedb(interface, dirname, 0);
			if (debug)
				printf("Interface \"%s\" disabled.\n", data.interface);
		} else if (!newdb) {
			printf("Interface \"%s\" is already disabled.\n", data.interface);
		}	
	}

	/* update */
	if (update) {

		/* check that there's some free diskspace left */
		if (!spacecheck(dirname) && !force) {
			printf("Error: Not enough free diskspace available.\n");
			return 1;
		}

		/* update every file if -i isn't specified */
		if (defaultiface) {

			dir=opendir(dirname);

			files=0;
			while ((di=readdir(dir))) {
				if (di->d_name[0]!='.') {
					files++;
					strncpy(interface, di->d_name, 32);
					if (debug)
						printf("\nProcessing file \"%s/%s\"...\n", dirname, interface);
					newdb=readdb(interface, dirname);
					if (data.active) {
						/* skip interface if not available */
						if (!getifinfo(data.interface)) {
							if (debug)
								printf("Interface \"%s\" not available, skipping.\n", data.interface);
							continue;
						}
						parseifinfo(newdb);

						/* check that the time is correct */
						if ((current>=data.lastupdated) || force) {
							writedb(interface, dirname, newdb);
						} else {
							/* print error if previous update is more than 6 hours in the future */
							/* otherwise do nothing */
							if (data.lastupdated>=(current+21600)) {
								printf("Error: The previous update was after the current date.\n\n");
								printf("Previous update: %s", (char*)asctime(localtime(&data.lastupdated)));
								printf("   Current time: %s\n", (char*)asctime(localtime(&current)));
								printf("Use --force to override this message.\n");
								return 1;
							} else {
								if (debug)
									printf("\"%s\" not updated, %s > %s.\n", data.interface, (char*)asctime(localtime(&data.lastupdated)), (char*)asctime(localtime(&current)));
							}
						}
					} else {
						if (debug)
							printf("Disabled interface \"%s\" not updated.\n", data.interface);
					}				
				}
			}

			closedir(dir);
			if (files==0) {
				// printf("No database found.\n");
				update=0;
			}

		/* update only selected file */
		} else {
			newdb=readdb(interface, dirname);
			if (data.active) {
				if (!getifinfo(data.interface) && !force) {
					getiflist(&ifacelist);
					printf("Error: Interface \"%s\" couldn't be found.\n Only available interfaces can be added for monitoring.\n", data.interface);
					printf("\n The following interfaces are currently available:\n    %s\n", ifacelist);
					free(ifacelist);
					return 1;
				}
				parseifinfo(newdb);
				if ((current>=data.lastupdated) || force) {
					if (strcmp(nick, "none")!=0)
						strncpy(data.nick, nick, 32);
					writedb(interface, dirname, newdb);
				} else {
					/* print error if previous update is more than 6 hours in the future */
					/* otherwise do nothing */
					if (data.lastupdated>=(current+21600)) {
						printf("Error: The previous update was after the current date.\n\n");
						printf("Previous update: %s", (char*)asctime(localtime(&data.lastupdated)));
						printf("   Current time: %s\n", (char*)asctime(localtime(&current)));
						printf("Use --force to override this message.\n");
						return 1;
					} else {
						if (debug)
							printf("\"%s\" not updated, %s > %s.\n", data.interface, (char*)asctime(localtime(&data.lastupdated)), (char*)asctime(localtime(&current)));
					}
				}
			} else {
				if (debug)
					printf("Disabled interface \"%s\" not updated.\n", data.interface);
			}
		}
	}

	/* show databases */
	if (query) {

		/* show all interfaces if -i isn't specified */
		if (defaultiface) {

			if (files==0) {
				/* printf("No database found.\n"); */
				query=0;
			} else if ((cfg.qmode==0 || cfg.qmode==8) && (files>1)) {

				if (cfg.qmode==0) {
					if (cfg.ostyle!=0) {
						printf("\n                      rx      /      tx      /     total    /   estimated\n");
					} else {
						printf("\n                      rx      /      tx      /     total\n");
					}
				} else {
					printf("<vnstat version=\"%s\" xmlversion=\"%d\">\n", VNSTATVERSION, XMLVERSION);
				}
				dir=opendir(dirname);
				while ((di=readdir(dir))) {
					if (di->d_name[0]!='.') {
						strncpy(interface, di->d_name, 32);
						if (debug)
							printf("\nProcessing file \"%s/%s\"...\n", dirname, interface);
						newdb=readdb(interface, dirname);
						if (!newdb) {
							if (cfg.qmode==0) {
								showdb(5);
							} else {
								showxml();
							}
						}
					}
				}
				closedir(dir);
				if (cfg.qmode==8) {
					printf("</vnstat>\n");
				}

			/* show in qmode if there's only one file or qmode!=0 */
			} else {
				if (!merged) {
					newdb=readdb(definterface, dirname);
				}
				if (!newdb) {
					if (cfg.qmode==5) {
						if (cfg.ostyle!=0) {
							printf("\n                      rx      /      tx      /     total    /   estimated\n");
						} else {
							printf("\n                      rx      /      tx      /     total\n");
						}
					}
					if (cfg.qmode!=8) {
						showdb(cfg.qmode);
					} else {
						printf("<vnstat version=\"%s\" xmlversion=\"%d\">\n", VNSTATVERSION, XMLVERSION);
						showxml();
						printf("</vnstat>\n");
					}
				}
			}

		/* show only specified file */
		} else {
			if (!merged) {
				newdb=readdb(interface, dirname);
			}
			if (!newdb) {
				if (cfg.qmode==5) {
					if (cfg.ostyle!=0) {
						printf("\n                      rx      /      tx      /     total    /   estimated\n");
					} else {
						printf("\n                      rx      /      tx      /     total\n");
					}
				}
				if (cfg.qmode!=8) {
					showdb(cfg.qmode);
				} else {
					printf("<vnstat version=\"%s\" xmlversion=\"%d\">\n", VNSTATVERSION, XMLVERSION);
					showxml();
					printf("</vnstat>\n");
				}
			}
		}
	}

	/* calculate traffic */
	if (traffic) {
		trafficmeter(interface, cfg.sampletime);
	}

	/* live traffic */
	if (livetraffic) {
		livetrafficmeter(interface, livemode);
	}

	/* if nothing was shown previously */
	if (!query && !update && !reset && !sync && active==-1 && !cleartop && !rebuildtotal && !traffic && !livetraffic) {

		/* give more help if there's no database */
		if (files==0) {
			getiflist(&ifacelist);
			printf("No database found, nothing to do. Use --help for help.\n\n");
			printf("A new database can be created with the following command:\n");
			printf("    %s -u -i eth0\n\n", argv[0]);
			printf("Replace 'eth0' with the interface that should be monitored.\n\n");
			printf("The following interfaces are currently available:\n    %s\n", ifacelist);
			free(ifacelist);
		} else {
			printf("Nothing to do. Use --help for help.\n");
		}
	}

	/* cleanup */
	ibwflush();

	return 0;
}


int synccounters(const char *iface, const char *dirname)
{
	readdb(iface, dirname);
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
