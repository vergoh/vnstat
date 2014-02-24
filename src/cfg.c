#include "vnstat.h"
#include "cfg.h"

void printcfgfile(void) {

	ibwnode *p = ifacebw;

	printf("# vnStat %s config file\n", VNSTATVERSION);
	printf("##\n\n");

	printf("# location of the database directory\n");
	printf("DatabaseDir \"%s\"\n\n", cfg.dbdir);

	printf("# locale (LC_ALL)\n");
	printf("Locale \"%s\"\n\n", cfg.locale);

	printf("# on which day should months change\n");
	printf("MonthRotate %d\n\n", cfg.monthrotate);
	
	printf("# date output formats for -d, -m, -t and -w\n");
	printf("# see 'man date' for control codes\n");
	printf("DayFormat    \"%s\"\n", cfg.dformat);
	printf("MonthFormat  \"%s\"\n", cfg.mformat);
	printf("TopFormat    \"%s\"\n\n", cfg.tformat);

	printf("# characters used for visuals\n");
	printf("RXCharacter       \"%c\"\n", cfg.rxchar[0]);
	printf("TXCharacter       \"%c\"\n", cfg.txchar[0]);
	printf("RXHourCharacter   \"%c\"\n", cfg.rxhourchar[0]);
	printf("TXHourCharacter   \"%c\"\n\n", cfg.txhourchar[0]);

	printf("# default interface\n");
	printf("Interface \"%s\"\n\n", cfg.iface);

	printf("# maximum bandwidth (Mbit) for all interfaces, 0 = disable feature\n# (unless interface specific limit is given)\n");
	printf("MaxBandwidth %d\n\n", cfg.maxbw);

	printf("# interface specific limits\n");
	printf("#  example 8Mbit limit for eth0 (remove # to activate):\n");
	printf("#MaxBWeth0 8\n");

	while (p != NULL) {
		printf("MaxBW%s %d\n", p->interface, p->limit);
		p = p->next;
	}
	
	printf("\n");

	printf("# how many seconds should sampling for -tr take by default\n");
	printf("Sampletime %d\n\n", cfg.sampletime);

	printf("# default query mode\n");
	printf("# 0 = normal, 1 = days, 2 = months, 3 = top10\n");
	printf("# 4 = dumpdb, 5 = short, 6 = weeks, 7 = hours\n");
	printf("QueryMode %d\n\n", cfg.qmode);

	printf("# database file locking (1 = enabled, 0 = disabled)\n");
	printf("UseFileLocking %d\n\n", cfg.flock);

	printf("# how much the boot time can variate between updates (seconds)\n");
	printf("BootVariation %d\n", cfg.bvar);

}

int loadcfg(char *cfgfile) {

	FILE *fd;
	char buffer[512];
	int i, j, k, linelen, cfglen, tryhome;

	char value[512], cfgline[512];
    
	char *cfgname[] = { "DatabaseDir", "Locale", "MonthRotate", "DayFormat", "MonthFormat", "TopFormat", "RXCharacter", "TXCharacter", "RXHourCharacter", "TXHourCharacter", "Interface", "MaxBandwidth", "Sampletime", "QueryMode", "UseFileLocking", "BootVariation", 0 };
	char *cfglocc[] = { cfg.dbdir, cfg.locale, 0, cfg.dformat, cfg.mformat, cfg.tformat, cfg.rxchar, cfg.txchar, cfg.rxhourchar, cfg.txhourchar, cfg.iface, 0, 0, 0, 0, 0 };
	int *cfgloci[] = { 0, 0, &cfg.monthrotate, 0, 0, 0, 0, 0, 0, 0, 0, &cfg.maxbw, &cfg.sampletime, &cfg.qmode, &cfg.flock, &cfg.bvar };
	int cfgnamelen[] = { 512, 32, 0, 64, 64, 64, 1, 1, 1, 1, 32, 0, 0, 0, 0, 0 };
	int cfgfound[] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

	ifacebw = NULL;

	/* clear buffer */
	for (i=0; i<512; i++) {
		buffer[i] = '\0';
	}

	/* load default config */
	defaultcfg();

	/* possible config files: 1) --config   2) $HOME/.vnstatrc   3) /etc/vnstat.conf   4) none */

	if (cfgfile[0]!='\0') {
	
		/* try to open given file */
		if ((fd=fopen(cfgfile, "r"))!=NULL) {
			if (debug)
				printf("Config file: --config\n");
		} else {
			printf("Error:\nUnable to open given config file \"%s\".\n", cfgfile);
		}
	
	} else {

		if (getenv("HOME")) {
			strncpy(buffer, getenv("HOME"), 500);
			strcat(buffer, "/.vnstatrc");
			tryhome = 1;
		} else {
			tryhome = 0;
		}

		/* try to open first available config file */
		if (tryhome && (fd=fopen(buffer, "r"))!=NULL) {
			if (debug)
				printf("Config file: $HOME/.vnstatrc\n");
		} else if ((fd=fopen("/etc/vnstat.conf", "r"))!=NULL) {
			if (debug)
				printf("Config file: /etc/vnstat.conf\n");
		} else {
			if (debug)
				printf("Config file: none\n");
			return 0;
		}
	}

	rewind(fd);

	/* parse every config file line */
	while (!feof(fd)) {
	
		cfgline[0] = '\0';
		
		/* get current line */
		fgets(cfgline, 512, fd);
		
		linelen = strlen(cfgline);
		if (linelen>2 && cfgline[0]!='#') {
		
			for (i=0; cfgname[i]!=0; i++) {

				cfglen = strlen(cfgname[i]);
				if ( (cfgfound[i]==0) && (linelen>=(cfglen+2)) && (strncasecmp(cfgline, cfgname[i], cfglen)==0) ) {
				
					/* clear value buffer */
					for (j=0; j<512; j++) {
						value[j]='\0';
					}
				
					/* search value */
					j = 0;
					for (k=cfglen; k<linelen; k++) {
						if (cfgline[k]=='\n' || cfgline[k]=='\r') {
							break;
						} else if (cfgline[k]=='\"') {
							if (j==0) {
								continue;
							} else {
								break;
							}
						} else {
							if (j==0 && (cfgline[k]==' ' || cfgline[k]=='=' || cfgline[k]=='\t')) {
								continue;
							} else {
								value[j] = cfgline[k];
								j++;
							}
						}
					}
					
					/* set value and get new line if valid value was found */
					if (strlen(value)!=0) {

						if (cfgnamelen[i]>0) {
							strncpy(cfglocc[i], value, cfgnamelen[i]);
						} else if (isdigit(value[0])) {
							*cfgloci[i] = atoi(value);
						} else {
							continue;
						}
						
						cfgfound[i] = 1;
						
						if (debug)
							printf("  %s   -> \"%s\": \"%s\"\n", cfgline, cfgname[i], value);						
						break;
					}
				
				} /* if */
			
			} /* for */
		
		} /* if */
	
	} /* while */

	/* search for interface specific limits */
	ibwcfgread(fd);
	
	fclose(fd);

	if (debug)
		ibwlist();

	return 1;

}

void defaultcfg(void) {

	cfg.bvar = BVAR;
	cfg.qmode = DEFQMODE;
	cfg.sampletime = DEFSAMPTIME;
	cfg.monthrotate = MONTHROTATE;
	cfg.maxbw = DEFMAXBW;
	cfg.flock = USEFLOCK;
	strncpy(cfg.dbdir, DATABASEDIR, 512);
	strncpy(cfg.iface, DEFIFACE, 32);
	strncpy(cfg.locale, LOCALE, 32);
	strncpy(cfg.dformat, DFORMAT, 64);
	strncpy(cfg.mformat, MFORMAT, 64);
	strncpy(cfg.tformat, TFORMAT, 64);
	strncpy(cfg.rxchar, RXCHAR, 1);
	strncpy(cfg.txchar, TXCHAR, 1);
	strncpy(cfg.rxhourchar, RXHOURCHAR, 1);
	strncpy(cfg.txhourchar, TXHOURCHAR, 1);
	
}

int ibwadd(char *iface, int limit)
{
	ibwnode *n = malloc(sizeof(ibwnode));

	if (n == NULL) {
		return 0;
	}

	n->next = ifacebw;
	ifacebw = n;
	strcpy(n->interface, iface);
	n->limit = limit;

	return 1;
}

void ibwlist(void)
{
	int i=1;
	ibwnode *p = ifacebw;
	
	if (p == NULL) {
		printf("ibw list is empty.\n");

	} else {
	
		printf("ibw:\n");
		while (p != NULL) {
			printf(" %2d: \"%s\" \"%d\"\n", i, p->interface, p->limit);
			p = p->next;
			i++;
		}
	}
}

int ibwget(char *iface)
{
	ibwnode *p = ifacebw;
	
	if (p == NULL) {

		if (cfg.maxbw>0) {
			return cfg.maxbw;
		} else {
			return -1;
		}

	} else {
	
		while (p != NULL) {
			if (strcasecmp(p->interface, iface)==0) {
				if (p->limit>0) {
					return p->limit;
				} else {
					return -1;
				}
			}
			p = p->next;
		}
		
		return -1;
	}
}

int ibwcfgread(FILE *fd)
{
	char cfgline[512], name[512], value[512];
	int i, j, linelen, count = 0;

	/* start from value search from first line */
	rewind(fd);

	/* cycle all lines */
	while (!feof(fd)) {

		cfgline[0] = '\0';

		/* get current line */
		fgets(cfgline, 512, fd);

		linelen = strlen(cfgline);

		if (linelen>8 && cfgline[0]!='#') {

			if (strncasecmp(cfgline, "MaxBW", 5)==0) {

				/* clear name and value buffers */
				for (j=0; j<512; j++) {
					name[j]=value[j]='\0';
				}			

				/* get interface name */
				j=0;
				for (i=5; i<linelen; i++) {
					if (cfgline[i]==' ' || cfgline[i]=='=' || cfgline[i]=='\t' || cfgline[i]=='\n' || cfgline[i]=='\r') {
						break;
					} else {
						name[j]=cfgline[i];
						j++;
					}
				}

				/* get new line if no usable name was found */
				if (strlen(name)==0) {
					continue;
				}

				/* search value */
				j=0;
				for (i++; i<linelen; i++) {
					if (cfgline[i]=='\n' || cfgline[i]=='\r') {
						break;
					} else if (cfgline[i]=='\"') {
						if (j==0) {
							continue;
						} else {
							break;
						}
					} else {
						if (j==0 && (cfgline[i]==' ' || cfgline[i]=='=' || cfgline[i]=='\t')) {
							continue;
						} else {
							value[j]=cfgline[i];
							j++;
						}
					}
				}

				/* get new line if no usable value was found */
				if ((strlen(value)==0) || (!isdigit(value[0])) ) {
					continue;
				}
			
				/* add interface and limit to list */
				ibwadd(name, atoi(value));
			}
		}
	}

	return count;
}
