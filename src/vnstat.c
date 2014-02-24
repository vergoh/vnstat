/*
vnStat - Copyright (c) 2003 Teemu Toivola <tst@iki.fi>

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

#include "vnstat.h"
#include "proc.h"
#include "db.h"
#include "misc.h"

int main(int argc, char *argv[]) {

	int i;
	int currentarg, update=0, query=0, newdb=0, qmode=0, reset=0;
	int active=-1, files=0, force=0, cleartop=0, rebuildtotal=0;
	char interface[32], filename[512], dirname[512], nick[32];
	char definterface[32];
	time_t current;
	DIR *dir;
	struct dirent *di;

	debug=0; /* debug disabled by default */

	setlocale(LC_ALL, "en_US");
	strcpy(interface, "default");
	strcpy(definterface, DEFIFACE);
	strcpy(nick, "none");
	qmode=atoi(DEFQMODE);

	current=time(NULL);

	/* init dirname */
#ifndef SINGLE
	strcpy(dirname, DATABASEDIR);
#else
	strcpy(dirname, getenv("HOME"));
	strcat(dirname,"/.vnstat");
#endif

	/* check if the database dir exists and if it contais files */
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
			printf("%d files found\n", files);
		if (files) {
			query=1;
			if (files>1)
				strcpy(definterface, DEFIFACE);
		}
		closedir(dir);
	} else {
		printf("Error:\nUnable to open database directory \"%s\".\n", dirname);
		printf("Make sure it exists and is read enabled for this user.\n");
		printf("Exiting...\n");
		exit(0);
	}


	/* parse parameters, maybe not the best way but... */
	for (currentarg=1; currentarg<argc; currentarg++) {
		if (debug)
			printf("arg %d: \"%s\"\n",currentarg,argv[currentarg]);
		if ((strcmp(argv[currentarg],"-h")==0) || (strcmp(argv[currentarg],"--help"))==0) {
		
			printf(" vnStat %s by Teemu Toivola <tst@iki.fi>\n\n", VNSTATVERSION);
			
			printf("   Update:\n");
			printf("\t -u, --update\t update database\n");
			printf("\t -r, --reset\t reset interface counters\n");
			printf("\t --enable\t enable interface\n");
			printf("\t --disable\t disable interface\n");
			printf("\t --nick  \t set a nickname for interface\n");
			printf("\t --cleartop\t clear the top10\n");
			printf("\t --rebuildtotal\t rebuild total transfers from months\n");
			
			printf("   Query:\n");
			printf("\t -q, --query\t query database\n");	
			printf("\t -d, --days\t show days\n");
			printf("\t -m, --months\t show months\n");
			printf("\t -t, --top10\t show top10\n");
			printf("\t -s, --short\t use short output\n");
			printf("\t --dumpdb\t show database in parseable format\n");
			
			printf("   Misc:\n");
			printf("\t -i, --iface\t interface (default: %s)\n", definterface);
			printf("\t -h, --help\t help\n");
			printf("\t -D, --debug\t show some additional debug information\n");
			printf("\t -v, --version\t show version\n");
			printf("\t --testkernel\t check if the kernel is broken\n");

			exit(0);
		} else if ((strcmp(argv[currentarg],"-i")==0) || (strcmp(argv[currentarg],"--iface"))==0) {
			if (currentarg+1<argc) {
				strncpy(interface,argv[currentarg+1],32);
				if (debug)
					printf("Used interface: %s\n",interface);
				currentarg++;
				continue;
			} else {
				printf("Interface for -i missing\n");
				exit(1);
			}
		} else if ((strcmp(argv[currentarg],"--nick"))==0) {
			if (currentarg+1<argc) {
				strncpy(nick,argv[currentarg+1],32);
				if (debug)
					printf("Used nick: %s\n",nick);
				currentarg++;
				continue;
			} else {
				printf("Nick for --nick missing\n");
				exit(1);
			}
		} else if ((strcmp(argv[currentarg],"-u")==0) || (strcmp(argv[currentarg],"--update"))==0) {
			update=1;
			query=0;
			if (debug)
				printf("Updating database...\n");
		} else if ((strcmp(argv[currentarg],"-q")==0) || (strcmp(argv[currentarg],"--query"))==0) {
			query=1;
		} else if ((strcmp(argv[currentarg],"-D")==0) || (strcmp(argv[currentarg],"--debug"))==0) {
			debug=1;
			printf("arg %d: \"%s\"\n",currentarg,argv[currentarg]);
		} else if ((strcmp(argv[currentarg],"-d")==0) || (strcmp(argv[currentarg],"--days"))==0) {
			qmode=1;
		} else if ((strcmp(argv[currentarg],"-m")==0) || (strcmp(argv[currentarg],"--months"))==0) {
			qmode=2;
		} else if ((strcmp(argv[currentarg],"-t")==0) || (strcmp(argv[currentarg],"--top10"))==0) {
			qmode=3;
		} else if ((strcmp(argv[currentarg],"-s")==0) || (strcmp(argv[currentarg],"--short"))==0) {
			qmode=5;
		} else if (strcmp(argv[currentarg],"--dumpdb")==0) {
			qmode=4;
		} else if (strcmp(argv[currentarg],"--enable")==0) {
			active=1;
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
			kerneltest();
			exit(0);
		} else if ((strcmp(argv[currentarg],"-v")==0) || (strcmp(argv[currentarg],"--version"))==0) {
			printf("vnStat %s by Teemu Toivola <tst@iki.fi>\n", VNSTATVERSION);
#ifndef SINGLE
			if (debug)
				printf("(Root install)\n");
#else
			if (debug)
				printf("(Singleuser install)\n");
#endif
			exit(0);
		} else if ((strcmp(argv[currentarg],"-r")==0) || (strcmp(argv[currentarg],"--reset"))==0) {
			reset=1;
			query=0;
		} else {
			printf("Unknown arg \"%s\".\n",argv[currentarg]);
			exit(1);
		}
		
	}

	/* counter reset */
	if (reset) {
		if (!spacecheck(dirname)) {
			printf("Error:\nNot enough free diskspace available.\n");
			exit(0);
		}
		if (strcmp(interface, "default")==0)
			strcpy(interface, definterface);
		sprintf(filename, "%s/%s", dirname, interface);
		readdb(interface, filename);
		data.currx=0;
		data.curtx=0;
		writedb(filename, 0);
		if (debug)
			printf("Counters reseted for \"%s\"\n", data.interface);
	}

	/* clear top10 */
	if (cleartop) {
		if (!spacecheck(dirname)) {
			printf("Error:\nNot enough free diskspace available.\n");
			exit(0);
		}
		if (strcmp(interface, "default")==0)
			strcpy(interface, definterface);
		if (force) {
			sprintf(filename, "%s/%s", dirname, interface);
			readdb(interface, filename);

			for (i=0; i<=9; i++) {
				data.top10[i].rx=data.top10[i].tx=0;
				data.top10[i].used=0;
			}

			writedb(filename, 0);
			if (debug)
				printf("Top10 reseted for \"%s\"\n", data.interface);
		} else {
			printf("Warning:\nThe current option would clear the top10 for \"%s\".\n", interface);
			printf("Use --force to override this message.\n");
			exit(0);
		}
	}

	/* rebuild total */
	if (rebuildtotal) {
		if (!spacecheck(dirname)) {
			printf("Error:\nNot enough free diskspace available.\n");
			exit(0);
		}
		if (strcmp(interface, "default")==0)
			strcpy(interface, definterface);
		if (force) {
			sprintf(filename, "%s/%s", dirname, interface);
			readdb(interface, filename);

			data.totalrx=data.totaltx=data.totalrxk=data.totaltxk=0;
			for (i=0; i<=29; i++) {
				if (data.month[i].used) {
					data.totalrx+=data.month[i].rx;
					data.totaltx+=data.month[i].tx;
				}
			}

			writedb(filename, 0);
			if (debug)
				printf("Total transfer rebuild completed for \"%s\"\n", data.interface);
		} else {
			printf("Warning:\nThe current option would rebuild total tranfers for \"%s\".\n", interface);
			printf("Use --force to override this message.\n");
			exit(0);
		}
	}

	/* enable & disable */
	if (active==1) {
		if (!spacecheck(dirname)) {
			printf("Error:\nNot enough free diskspace available.\n");
			exit(0);
		}
		if (strcmp(interface, "default")==0)
			strcpy(interface, definterface);
		sprintf(filename, "%s/%s", dirname, interface);
		newdb=readdb(interface, filename);
		if (!data.active && !newdb) {
			data.active=1;
			writedb(filename, 0);
			if (debug)
				printf("Interface \"%s\" enabled.\n", data.interface);
		} else if (!newdb) {
			printf("Interface \"%s\" is already enabled.\n", data.interface);
		}
	} else if (active==0) {
		if (!spacecheck(dirname)) {
			printf("Error:\nNot enough free diskspace available.\n");
			exit(0);
		}
		if (strcmp(interface, "default")==0)
			strcpy(interface, definterface);
		sprintf(filename, "%s/%s", dirname, interface);
		newdb=readdb(interface, filename);
		if (data.active && !newdb) {
			data.active=0;
			writedb(filename, 0);
			if (debug)
				printf("Interface \"%s\" disabled.\n", data.interface);
		} else if (!newdb) {
			printf("Interface \"%s\" is already disabled.\n", data.interface);
		}	
	}
	
	/* update */
	if (update) {
	
		/* check that there's some free diskspace left */
		if (!spacecheck(dirname)) {
			printf("Error:\nNot enough free diskspace available.\n");
			exit(0);
		}
		
		/* update every file if -i isn't specified */
		if (strcmp(interface, "default")==0) {
			
			dir=opendir(dirname);

			files=0;
			while ((di=readdir(dir))) {
				if (di->d_name[0]!='.') {
					files++;
					strcpy(interface, di->d_name);
					sprintf(filename, "%s/%s", dirname, interface);
					if (debug)
						printf("\nProcessing file \"%s\"...\n", filename);
					newdb=readdb(interface, filename);
					if (data.active) {
						readproc(data.interface);
						parseproc(newdb);
						
						/* check that the time is correct */
						if ((current>=data.lastupdated) || force) {
							writedb(filename, newdb);
						} else {
							printf("Error:\nThe previous update was after the current date.\n\n");
							printf("Previous update: %s", (char*)asctime(localtime(&data.lastupdated)));
							printf("   Current time: %s\n", (char*)asctime(localtime(&current)));
							printf("Use --force to override this message.\n");
							exit(0);
						}
					} else {
						if (debug)
							printf("Disabled interface \"%s\" not updated.\n", data.interface);
					}				
				}
			}

			closedir(dir);
			if (files==0)
				printf("No database found.\n");
			
			/* reset to default */
			strcpy(interface, "default");
		
		/* update only selected file */
		} else {
			sprintf(filename, "%s/%s", dirname, interface);
			newdb=readdb(interface, filename);
			if (data.active) {
				readproc(data.interface);
				parseproc(newdb);
				if ((current>=data.lastupdated) || force) {
					if (strcmp(nick, "none")!=0)
						strncpy(data.nick, nick, 32);
					writedb(filename, newdb);
				} else {
					printf("Error:\nThe previous update was after the current date.\n\n");
					printf("Previous update: %s", (char*)asctime(localtime(&data.lastupdated)));
					printf("   Current time: %s\n", (char*)asctime(localtime(&current)));
					printf("Use --force to override this message.\n");
					exit(0);
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
		if (strcmp(interface, "default")==0) {
			
			if (files==0) {
				printf("No database found.\n");
			} else if ((qmode==0) && (files>1)) {

				dir=opendir(dirname);
				printf("\n                     rx      /     tx      /    total    /  estimated\n");
				while ((di=readdir(dir))) {
					if (di->d_name[0]!='.') {
						strcpy(interface, di->d_name);
						sprintf(filename, "%s/%s", dirname, interface);
						if (debug)
							printf("\nProcessing file \"%s\"...\n", filename);
						readdb(interface, filename);
						if (!newdb)
							showdb(5);
						
					}
				}
				closedir(dir);
				
			/* show in qmode if there's only one file or qmode!=0 */
			} else {
				sprintf(filename, "%s/%s", dirname, definterface);
				readdb(definterface, filename);
				if (!newdb) {
					if (qmode==5)
						printf("\n                     rx      /     tx      /    total    /  estimated\n");
					showdb(qmode);
				}
			}
		
		/* show only specified file */
		} else {
			sprintf(filename, "%s/%s", dirname, interface);
			readdb(interface, filename);
			if (!newdb) {
				if (qmode==5)
					printf("\n                     rx      /     tx      /    total    /  estimated\n");
				showdb(qmode);
			}
		}
	}
	
	/* if nothing was shown previously */
	if (!query && !update && !reset && active==-1 && !cleartop && !rebuildtotal)
		printf("Nothing to do. Use -h for help\n");
	
	return 0;
}
