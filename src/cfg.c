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

int loadcfg(void) {

	FILE *fd;
	char buffer[512];
	int i;

	char *cfgname[] = { "DatabaseDir", "Locale", "MonthRotate", "DayFormat", "MonthFormat", "TopFormat", "RXCharacter", "TXCharacter", "RXHourCharacter", "TXHourCharacter", "Interface", "MaxBandwidth", "Sampletime", "QueryMode", "UseFileLocking", "BootVariation", 0 };
	char *cfglocc[] = { cfg.dbdir, cfg.locale, 0, cfg.dformat, cfg.mformat, cfg.tformat, cfg.rxchar, cfg.txchar, cfg.rxhourchar, cfg.txhourchar, cfg.iface, 0, 0, 0, 0, 0 };
	int *cfgloci[] = { 0, 0, &cfg.monthrotate, 0, 0, 0, 0, 0, 0, 0, 0, &cfg.maxbw, &cfg.sampletime, &cfg.qmode, &cfg.flock, &cfg.bvar };
	int cfgnamelen[] = { 512, 32, 0, 64, 64, 64, 1, 1, 1, 1, 32, 0, 0, 0, 0, 0 };

	ifacebw = NULL;

	/* clear buffer */
	for (i=0; i<512; i++) {
		buffer[i] = '\0';
	}

	/* load default config */
	defaultcfg();

	/* possible config files: 1) $HOME/.vnstatrc   2) /etc/vnstat.conf   3) none */

	strncpy(buffer, getenv("HOME"), 500);
	strcat(buffer, "/.vnstatrc");

	/* try to open first available config file */
	if ((fd=fopen(buffer,"r"))!=NULL) {
		if (debug)
			printf("Config file: $HOME/.vnstatrc\n");
	} else if ((fd=fopen("/etc/vnstat.conf","r"))!=NULL) {
		if (debug)
			printf("Config file: /etc/vnstat.conf\n");
/*	} else if ((fd=fopen("vnstat.conf","r"))!=NULL) {
		if (debug)
			printf("Config file: ./vnstat.conf\n"); */
	} else {
		if (debug)
			printf("Config file: none\n");
		return 0;
	}

	rewind(fd);

	/* try to read all config options from file */
	for (i=0; cfgname[i]!=0; i++) {
		strcpy(buffer, cfgname[i]);
		if (getcfgvalue(fd, buffer)) {
			if (cfgnamelen[i]>0) {
				strncpy(cfglocc[i], buffer, cfgnamelen[i]);

			} else if (isdigit(buffer[0])) {
				*cfgloci[i] = atoi(buffer);
			}
		}
	}

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
#ifdef SINGLE
	strncpy(cfg.dbdir, getenv("HOME"), 500);
	strcat(cfg.dbdir, "/.vnstat");
#else
	strncpy(cfg.dbdir, DATABASEDIR, 512);
#endif
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

int getcfgvalue(FILE *fd, char *search) {

	char value[512], cfgline[512];
	int i, j, linelen, searchlen, rewinds;
	long startpos;
	
	rewinds = 0;
	searchlen = strlen(search);
	
	/* rewind file if needed */
	if (feof(fd)) {
		rewind(fd);
		rewinds++;
	}

	/* get current position in file */
	startpos = ftell(fd);
	
	if (debug)
		printf(" search & startpos: '%s' '%ld'\n", search, startpos);

	/* cycle all lines if needed */
	while (!feof(fd)) {

		cfgline[0] = '\0';

		/* get current line */
		fgets(cfgline, 512, fd);

		linelen = strlen(cfgline);

		if (linelen>2 && cfgline[0]!='#') {

			if ( (strncasecmp(cfgline, search, searchlen)==0) && (linelen>=searchlen+2) ) {

				/* value buffers */
				for (j=0; j<512; j++) {
					value[j]='\0';
				}
			
				/* search value */
				j=0;
				for (i=searchlen; i<linelen; i++) {
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
			
				/* continue search if found value wasn't suitable */
				if (strlen(value)==0) {
					continue;
				} else {
				
					if (debug)
						printf("  %s   -> \"%s\"\n", cfgline, value);
						
					/* fill answer and return to caller */
					strcpy(search, value);
					return 1;
				}
			}
		}
		
		/* rewind file if end of file was reached and 
		   reading didn't start from the beginning */
		if (feof(fd) && startpos && !rewinds)  {
			rewind(fd);
			rewinds++;
			
			if (debug)
				printf(" rewind\n");
		}

	}

	return 0;
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
