/*
vnStat daemon - Copyright (c) 2008-11 Teemu Toivola <tst@iki.fi>

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
#include "dbaccess.h"
#include "dbcache.h"
#include "misc.h"
#include "cfg.h"
#include "vnstatd.h"

int main(int argc, char *argv[])
{
	int currentarg, running = 1, updateinterval, dbcount, dodbsave, rundaemon;
	int dbsaved = 1, showhelp = 1, sync = 0, saveinterval, forcesave = 0, noadd = 0;
	uint32_t dbhash = 0;
	char cfgfile[512], dirname[512];
	DIR *dir;
	struct dirent *di;
	datanode *datalist;
	time_t current, prevdbupdate, prevdbsave;

	noexit = 1;        /* disable exits in functions */
	debug = 0;         /* debug disabled by default */
	rundaemon = 0;     /* daemon disabled by default */
	cfgfile[0] = '\0';
	prevdbupdate = prevdbsave = dbcount = 0;

	/* early check for debug and config parameter */
	if (argc > 1) {
		for (currentarg=1; currentarg<argc; currentarg++) {
			if ((strcmp(argv[currentarg],"-D")==0) || (strcmp(argv[currentarg],"--debug")==0)) {
				debug = 1;
			} else if (strcmp(argv[currentarg],"--config")==0) {
				if (currentarg+1<argc) {
					strncpy(cfgfile, argv[currentarg+1], 512);
					cfgfile[511] = '\0';
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

	/* init dirname and other config settings */
	strncpy(dirname, cfg.dbdir, 512);
	updateinterval = cfg.updateinterval;
	saveinterval = cfg.saveinterval*60;

	/* parse parameters, maybe not the best way but... */
	for (currentarg=1; currentarg<argc; currentarg++) {
		if (debug)
			printf("arg %d: \"%s\"\n",currentarg,argv[currentarg]);
		if ((strcmp(argv[currentarg],"-?")==0) || (strcmp(argv[currentarg],"--help")==0)) {
			break;
		} else if (strcmp(argv[currentarg],"--config")==0) {
			/* config has already been parsed earlier so nothing to do here */
			currentarg++;
			continue;
		} else if ((strcmp(argv[currentarg],"-D")==0) || (strcmp(argv[currentarg],"--debug")==0)) {
			debug=1;
		} else if ((strcmp(argv[currentarg],"-d")==0) || (strcmp(argv[currentarg],"--daemon")==0)) {
			rundaemon = 1;
			showhelp = 0;
		} else if ((strcmp(argv[currentarg],"-n")==0) || (strcmp(argv[currentarg],"--nodaemon")==0)) {
			showhelp = 0;
		} else if ((strcmp(argv[currentarg],"-s")==0) || (strcmp(argv[currentarg],"--sync")==0)) {
			sync = 1;
		} else if (strcmp(argv[currentarg],"--noadd")==0) {
			noadd = 1;
		} else if ((strcmp(argv[currentarg],"-v")==0) || (strcmp(argv[currentarg],"--version")==0)) {
			printf("vnStat daemon %s by Teemu Toivola <tst at iki dot fi>\n", VNSTATVERSION);
			return 0;
		} else if ((strcmp(argv[currentarg],"-p")==0) || (strcmp(argv[currentarg],"--pidfile")==0)) {
			if (currentarg+1<argc) {
				strncpy(cfg.pidfile, argv[currentarg+1], 512);
				cfg.pidfile[511] = '\0';
				if (debug)
					printf("Used pid file: %s\n", cfg.pidfile);
				currentarg++;
				continue;
			} else {
				printf("Error: File for --pidfile missing.\n");
				return 1;
			}
		} else {
			printf("Unknown arg \"%s\". Use --help for help.\n",argv[currentarg]);
			return 1;
		}
	}

	/* show help if nothing else was asked to be done */
	if (showhelp) {
		printf(" vnStat daemon %s by Teemu Toivola <tst at iki dot fi>\n\n", VNSTATVERSION);
		printf("         -d, --daemon         fork process to background\n");
		printf("         -n, --nodaemon       stay in foreground attached to the terminal\n");
		printf("         -s, --sync           sync interface counters on first update\n");
		printf("         -D, --debug          show additional debug and disable daemon\n");
		printf("         -?, --help           show this help\n");
		printf("         -v, --version        show version\n");
		printf("         -p, --pidfile        select used pid file\n");
		printf("         --config             select used config file\n\n");
		printf("         --noadd              don't add found interfaces if no dbs are found\n");
		printf("See also \"man vnstatd\".\n");
		return 0;
	}

	/* check that directory is ok */
	if ((dir=opendir(dirname))==NULL) {
		printf("Error: Unable to open database directory \"%s\".\n", dirname);
		printf("Make sure it exists and is at least read enabled for current user.\n");
		printf("Exiting...\n");
		return 1;
	} else {
		/* check if there's something to work with */
		dbcount = 0;
		while ((di=readdir(dir))) {
			if (di->d_name[0]!='.') {
				dbcount++;
			}
		}
		closedir(dir);
		if (dbcount==0) {
			if (noadd) {
				printf("Zero database found, exiting.\n");
				return 1;
			} else {
				if (!spacecheck(dirname)) {
					printf("Error: Not enough free diskspace available, exiting.\n");
					return 1;
				}
				printf("Zero database found, adding available interfaces...\n");
				if (!addinterfaces(dirname)) {
					printf("Nothing to do, exiting.\n");
					return 1;
				}
				dbcount = 0;
			}
		} else {
			/* set counter back to zero so that dbs will be cached later */
			dbcount = 0;
		}
	}

	/* init signal traps */
	intsignal = 0;
	if (signal(SIGINT, sighandler) == SIG_ERR) {
		perror("signal");
		return 1;
	}
	if (signal(SIGHUP, sighandler) == SIG_ERR) {
		perror("signal");
		return 1;
	}
	if (signal(SIGTERM, sighandler) == SIG_ERR) {
		perror("signal");
		return 1;
	}

	/* start as daemon if needed and debug isn't enabled */
	if (rundaemon && !debug) {
		noexit++;
		daemonize();
	}

	/* main loop */
	while(running) {

		/* keep track of time */
		current = time(NULL);

		/* track interface status only if at least one database exists */
		if (dbcount!=0) {
			dbhash = dbcheck(dbhash, &forcesave);
		}

		if ((current-prevdbupdate)>=updateinterval) {

			updateinterval = cfg.updateinterval;

			if (debug) {
				cacheshow();
			}

			/* read database list if cache is empty */
			if (dbcount==0) {

				if ((dir=opendir(dirname))!=NULL) {

					while ((di=readdir(dir))) {
						if (di->d_name[0]!='.') {
							if (debug) {
								printf("\nProcessing file \"%s/%s\"...\n", dirname, di->d_name);
							}

							if (!cacheadd(di->d_name, sync)) {
								snprintf(errorstring, 512, "Cache memory allocation failed, exiting.");
								printe(PT_Error);

								/* clean daemon stuff before exit */
								if (rundaemon && !debug) {
									close(pidfile);
									unlink(cfg.pidfile);
								}
								ibwflush();
								return 1;
							}

							dbcount++;
						}
					}

					closedir(dir);
					sync = 0;

					/* disable update interval check for one loop if database list was refreshed */
					/* otherwise increase default update interval since there's nothing else to do */
					if (dbcount) {
						updateinterval = 0;
						intsignal = 42;
						prevdbsave = current;
						/* list monitored interfaces to log */
						cachestatus();
					} else {
						updateinterval = 120;
					}

				} else {
					snprintf(errorstring, 512, "Unable to access database directory \"%s\", exiting.", dirname);
					printe(PT_Error);

					/* clean daemon stuff before exit */
					if (rundaemon && !debug) {
						close(pidfile);
						unlink(cfg.pidfile);
					}
					ibwflush();
					return 1;
				}

			/* update data cache */
			} else {
				prevdbupdate = current;
				datalist = dataptr;

				/* alter save interval if all interfaces are unavailable */
				if (cacheactivecount()) {
					saveinterval = cfg.saveinterval*60;
				} else {
					saveinterval = cfg.offsaveinterval*60;
				}

				if ((current-prevdbsave)>=(saveinterval) || forcesave) {
					dodbsave = 1;
					forcesave = 0;
					prevdbsave = current;
				} else {
					dodbsave = 0;
				}

				/* check all list entries*/
				while (datalist!=NULL) {

					if (debug) {
						printf("d: processing %s (%d)...\n", datalist->data.interface, dodbsave);
					}

					/* get data from cache if available */
					if (cacheget(datalist)==0) {

						/* try to read data from file if not cached */
						if (readdb(datalist->data.interface, dirname)!=-1) {
							/* mark cache as filled on read success and force interface status update */
							datalist->filled = 1;
							dbhash = 0;
						} else {
							datalist = datalist->next;
							continue;						
						}
					}

					/* get info if interface has been marked as active */
					if (data.active) {
						if (getifinfo(data.interface)) {
							if (datalist->sync) { /* if --sync was used during startup */
								data.currx = ifinfo.rx;
								data.curtx = ifinfo.tx;
								datalist->sync = 0;
							} else {
								parseifinfo(0);
							}
						} else {
							/* disable interface since we can't access its data */
							data.active = 0;
							snprintf(errorstring, 512, "Interface \"%s\" not available, disabling.", data.interface);
							printe(PT_Info);
						}
					} else if (debug) {
						printf("d: interface is disabled\n");
					}

					/* check that the time is correct */
					if (current>=data.lastupdated) {
						data.lastupdated = current;
						cacheupdate();
					} else {
						/* skip update if previous update is less than a day in the future */
						/* otherwise exit with error message since the clock is problably messed */
						if (data.lastupdated>(current+86400)) {
							snprintf(errorstring, 512, "Interface \"%s\" has previous update date too much in the future, exiting.", data.interface);
							printe(PT_Error);

							/* clean daemon stuff before exit */
							if (rundaemon && !debug) {
								close(pidfile);
								unlink(cfg.pidfile);
							}
							ibwflush();
							return 1;
						} else {
							datalist = datalist->next;
							continue;
						}
					}

					/* write data to file if now is the time for it */
					if (dodbsave) {
						if (checkdb(datalist->data.interface, dirname)) {
							if (spacecheck(dirname)) {
								if (writedb(datalist->data.interface, dirname, 0)) {
									if (!dbsaved) {
										snprintf(errorstring, 512, "Database write possible again.");
										printe(PT_Info);
										dbsaved = 1;
									}
								} else {
									if (dbsaved) {
										snprintf(errorstring, 512, "Unable to write database, continuing with cached data.");
										printe(PT_Error);
										dbsaved = 0;
									}
								}
							} else {
								/* show freespace error only once */
								if (dbsaved) {
									snprintf(errorstring, 512, "Free diskspace check failed, unable to write database, continuing with cached data.");
									printe(PT_Error);
									dbsaved = 0;
								}
							}
						} else {
							/* remove interface from update list since the database file doesn't exist anymore */
							snprintf(errorstring, 512, "Database for interface \"%s\" no longer exists, removing from update list.", datalist->data.interface);
							printe(PT_Info);
							datalist = cacheremove(datalist->data.interface);
							dbcount--;
							cachestatus();
							continue;
						}
					}

					datalist = datalist->next;
				}

				if (debug) {
					printf("\n");
				}
			}
		} /* dbupdate */

		if (running && intsignal==0) {
			sleep(cfg.pollinterval);
		}

		/* take actions from signals */
		if (intsignal) {
			switch (intsignal) {

				case SIGHUP:
					snprintf(errorstring, 512, "SIGHUP received, flushing data to disk and reloading config.");
					printe(PT_Info);
					cacheflush(dirname);
					dbcount = 0;
					ibwflush();
					if (loadcfg(cfgfile)) {
						strncpy(dirname, cfg.dbdir, 512);
					}
					break;

				case SIGINT:
					snprintf(errorstring, 512, "SIGINT received, exiting.");
					printe(PT_Info);
					running = 0;
					break;

				case SIGTERM:
					snprintf(errorstring, 512, "SIGTERM received, exiting.");
					printe(PT_Info);
					running = 0;
					break;

				case 42:
					break;

				default:
					snprintf(errorstring, 512, "Unkown signal %d received, ignoring.", intsignal);
					printe(PT_Info);
					break;
			}

			intsignal = 0;
		}

	} /* while */

	cacheflush(dirname);
	ibwflush();

	/* clean daemon stuff */
	if (rundaemon && !debug) {
		close(pidfile);
		unlink(cfg.pidfile);
	}

	return 0;
}

void daemonize(void)
{
	int i;
	char str[10];

	if (getppid()==1) {
		return; /* already a daemon */
	}

	i = (int)fork();

	if (i<0) { /* fork error */
		perror("fork");
		exit(EXIT_FAILURE); 
	}
	if (i>0) { /* parent exits */
		exit(EXIT_SUCCESS);
	}
	/* child (daemon) continues */

	setsid(); /* obtain a new process group */

	if (cfg.uselogging) {
		snprintf(errorstring, 512, "vnStat daemon %s started.", VNSTATVERSION);
		if (!printe(PT_Info)) {
			printf("Error: Unable to use logfile. Exiting.\n");
			exit(EXIT_FAILURE);
		}
	}

	/* lock / pid file */
	pidfile = open(cfg.pidfile, O_RDWR|O_CREAT, 0644);
	if (pidfile<0) {
		perror("pidfile");
		snprintf(errorstring, 512, "pidfile failed, exiting.");
		printe(PT_Error);
		exit(EXIT_FAILURE); /* can't open */
	}
	if (lockf(pidfile,F_TLOCK,0)<0) {
		perror("pidfile lock");
		snprintf(errorstring, 512, "pidfile lock failed, exiting.");
		printe(PT_Error);
		exit(EXIT_FAILURE); /* can't lock */
	}

	/* close all descriptors except lock file */
	for (i=getdtablesize();i>=0;--i) {
		if (i!=pidfile) {
			close(i);
		}
	}

	/* redirect standard i/o to null */
	i=open("/dev/null",O_RDWR); /* stdin */

	/* stdout */
	if (dup(i) < 0) {
		perror("dup(stdout)");
		snprintf(errorstring, 512, "dup(stdout) failed, exiting.");
		printe(PT_Error);
		exit(EXIT_FAILURE);
	}
	/* stderr */
	if (dup(i) < 0) {
		perror("dup(stderr)");
		snprintf(errorstring, 512, "dup(stderr) failed, exiting.");
		printe(PT_Error);
		exit(EXIT_FAILURE);
	}

	umask(027); /* set newly created file permissions */

	/* change running directory */
	if (chdir("/") < 0) {
		perror("chdir(/)");
		snprintf(errorstring, 512, "directory change to / failed, exiting.");
		printe(PT_Error);
		exit(EXIT_FAILURE);
	}

	/* first instance continues */
	snprintf(str, 10, "%d\n", (int)getpid());

	/* record pid to lockfile */
	if (write(pidfile,str,strlen(str)) < 0) {
		perror("write(pidfile)");
		snprintf(errorstring, 512, "writing to pidfile %s failed, exiting.", cfg.pidfile);
		printe(PT_Error);
		exit(EXIT_FAILURE);
	}

	signal(SIGCHLD,SIG_IGN); /* ignore child */
	signal(SIGTSTP,SIG_IGN); /* ignore tty signals */
	signal(SIGTTOU,SIG_IGN);
	signal(SIGTTIN,SIG_IGN);

	if (cfg.uselogging==1) {
		snprintf(errorstring, 512, "Daemon running with pid %d.", (int)getpid());
		printe(PT_Info);
	}
}

int addinterfaces(const char *dirname)
{
	char *ifacelist, interface[32];
	int index = 0, count = 0;

	/* get list of currently visible interfaces */
	if (getiflist(&ifacelist)==0) {
		return 0;
	}

	if (strlen(ifacelist)<2) {
		return 0;
	}

	if (debug)
		printf("Interface list: \"%s\"\n", ifacelist);

	while (sscanf(ifacelist+index, "%32s", interface)!=EOF) {
		if (debug)
			printf("Processing: \"%s\"\n", interface);

		index += strlen(interface)+1;

		/* skip local interfaces */
		if ((strcmp(interface,"lo")==0) || (strcmp(interface,"lo0")==0) || (strcmp(interface,"sit0")==0)) {
			if (debug)
				printf("skip\n");
			continue;
		}

		/* create database for interface */
		initdb();
		strncpy(data.interface, interface, 32);
		strncpy(data.nick, data.interface, 32);
		if (!getifinfo(interface)) {
			if (debug)
				printf("getifinfo failed, skip\n");
			continue;
		}
		parseifinfo(1);
		if (!writedb(interface, dirname, 1)) {
			continue;
		}
		count++;
		printf("\"%s\" added, %d Mbit bandwidth limit.\n", interface, ibwget(interface));
	}

	if (count==1) {
		printf("-> %d interface added.", count);
	} else {
		printf("-> %d interfaces added.", count);
	}

	if (count) {
		printf(" Limits can be modified using the configuration file.");
	}

	printf("\n");

	free(ifacelist);
	return count;
}
