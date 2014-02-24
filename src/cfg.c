#include "common.h"
#include "cfg.h"

void printcfgfile(void)
{
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

	printf("# how units are prefixed when traffic is shown\n");
	printf("# 0 = IEC standard prefixes (KiB/MiB/GiB/TiB)\n");
	printf("# 1 = old style binary prefixes (KB/MB/GB/TB)\n");
	printf("UnitMode %d\n\n", cfg.unit);

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

	printf("# filesystem disk space check (1 = enabled, 0 = disabled)\n");
	printf("CheckDiskSpace %d\n\n", cfg.spacecheck);
	
	printf("# database file locking (1 = enabled, 0 = disabled)\n");
	printf("UseFileLocking %d\n\n", cfg.flock);

	printf("# how much the boot time can variate between updates (seconds)\n");
	printf("BootVariation %d\n", cfg.bvar);

	printf("\n\n");

	printf("# vnstatd\n##\n\n");

	printf("# how often (in seconds) interface data is updated\n");
	printf("UpdateInterval %d\n\n", cfg.updateinterval);

	printf("# how often (in seconds) interface status changes are checked\n");
	printf("PollInterval %d\n\n", cfg.pollinterval);

	printf("# how often (in minutes) interface data is saved to file\n");
	printf("SaveInterval %d\n\n", cfg.saveinterval);

	printf("# enable / disable logging (0 = disabled, 1 = logfile, 2 = syslog)\n");
	printf("UseLogging %d\n\n", cfg.uselogging);

	printf("# file used for logging if UseLogging is set to 1\n");
	printf("LogFile \"%s\"\n\n", cfg.logfile);

	printf("# file used as daemon pid / lock file\n");
	printf("PidFile \"%s\"\n", cfg.pidfile);

	printf("\n\n");

	printf("# vnstati\n##\n\n");
	printf("HeaderFormat    \"%s\"\n\n", cfg.hformat);
	printf("# colors\n");
	printf("CBackground     \"%s\"\n", cfg.cbg);
	printf("CEdge           \"%s\"\n", cfg.cedge);
	printf("CHeader         \"%s\"\n", cfg.cheader);
	printf("CHeaderTitle    \"%s\"\n", cfg.cheadertitle);
	printf("CHeaderDate     \"%s\"\n", cfg.cheaderdate);
	printf("CText           \"%s\"\n", cfg.ctext);
	printf("CLine           \"%s\"\n", cfg.cline);
	printf("CLineL          \"%s\"\n", cfg.clinel);
	printf("CRx             \"%s\"\n", cfg.crx);
	printf("CTx             \"%s\"\n", cfg.ctx);
	printf("CRxD            \"%s\"\n", cfg.crxd);
	printf("CTxD            \"%s\"\n", cfg.ctxd);

}

int loadcfg(char *cfgfile)
{
	FILE *fd;
	char buffer[512];
	int i, j, k, linelen, cfglen, tryhome;

	char value[512], cfgline[512];
    
	char *cfgname[] = { "DatabaseDir", "Locale", "MonthRotate", "DayFormat", "MonthFormat", "TopFormat", "RXCharacter", "TXCharacter", "RXHourCharacter", "TXHourCharacter", "UnitMode", "Interface", "MaxBandwidth", "Sampletime", "QueryMode", "CheckDiskSpace", "UseFileLocking", "BootVariation", "UpdateInterval", "PollInterval", "SaveInterval", "UseLogging", "LogFile", "PidFile", "CBackground", "CEdge", "CHeader", "CHeaderTitle", "CHeaderDate", "CText", "CLine", "CLineL", "CRx", "CRxD", "CTx", "CTxD", "HeaderFormat", 0 };
	char *cfglocc[] = { cfg.dbdir, cfg.locale, 0, cfg.dformat, cfg.mformat, cfg.tformat, cfg.rxchar, cfg.txchar, cfg.rxhourchar, cfg.txhourchar, 0, cfg.iface, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, cfg.logfile, cfg.pidfile, cfg.cbg, cfg.cedge, cfg.cheader, cfg.cheadertitle, cfg.cheaderdate, cfg.ctext, cfg.cline, cfg.clinel, cfg.crx, cfg.crxd, cfg.ctx, cfg.ctxd, cfg.hformat };
	short *cfgloci[] = { 0, 0, &cfg.monthrotate, 0, 0, 0, 0, 0, 0, 0, &cfg.unit, 0, &cfg.maxbw, &cfg.sampletime, &cfg.qmode, &cfg.spacecheck, &cfg.flock, &cfg.bvar, &cfg.updateinterval, &cfg.pollinterval, &cfg.saveinterval, &cfg.uselogging, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
	short cfgnamelen[] = { 512, 32, 0, 64, 64, 64, 1, 1, 1, 1, 0, 32, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 512, 512, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 32 };
	short cfgfound[] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

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
			snprintf(errorstring, 512, "Unable to open given config file \"%s\".\n", cfgfile);
			printe(PT_Error);
			return 0;
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
			return 1;
		}
	}

	rewind(fd);

	/* parse every config file line */
	while (!feof(fd)) {
	
		cfgline[0] = '\0';
		
		/* get current line */
		if (fgets(cfgline, 512, fd)==NULL) {
			break;
		}
		
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

	/* validate config */
	validatecfg();

	return 1;

}

void validatecfg(void)
{
	if (cfg.unit<0 || cfg.unit>1) {
		cfg.unit = UNITMODE;
		snprintf(errorstring, 512, "Invalid value for UnitMode, resetting to \"%d\".", cfg.unit);
		printe(PT_Config);
	}

	if (cfg.bvar<0 || cfg.bvar>300) {
		cfg.bvar = BVAR;
		snprintf(errorstring, 512, "Invalid value for BootVariation, resetting to \"%d\".", cfg.bvar);
		printe(PT_Config);
	}

	if (cfg.sampletime<0 || cfg.sampletime>600) {
		cfg.sampletime = DEFSAMPTIME;
		snprintf(errorstring, 512, "Invalid value for Sampletime, resetting to \"%d\".", cfg.sampletime);
		printe(PT_Config);
	}

	if (cfg.monthrotate<1 || cfg.monthrotate>31) {
		cfg.monthrotate = MONTHROTATE;
		snprintf(errorstring, 512, "Invalid value for MonthRotate, resetting to \"%d\".", cfg.monthrotate);
		printe(PT_Config);
	}

	if (cfg.maxbw<0 || cfg.maxbw>10000) {
		cfg.maxbw = DEFMAXBW;
		snprintf(errorstring, 512, "Invalid value for MaxBandwidth, resetting to \"%d\".", cfg.maxbw);
		printe(PT_Config);
	}

	if (cfg.spacecheck<0 || cfg.spacecheck>1) {
		cfg.spacecheck = USESPACECHECK;
		snprintf(errorstring, 512, "Invalid value for CheckDiskSpace, resetting to \"%d\".", cfg.spacecheck);
		printe(PT_Config);
	}

	if (cfg.flock<0 || cfg.flock>1) {
		cfg.flock = USEFLOCK;
		snprintf(errorstring, 512, "Invalid value for UseFileLocking, resetting to \"%d\".", cfg.flock);
		printe(PT_Config);
	}

	if (cfg.dbdir[0] != '/') {
		strncpy(cfg.dbdir, DATABASEDIR, 512);
		snprintf(errorstring, 512, "DatabaseDir doesn't start with \"/\", resetting to default.");
		printe(PT_Config);
	}

	if (cfg.pollinterval<2 || cfg.pollinterval>60) {
		cfg.pollinterval = POLLINTERVAL;
		snprintf(errorstring, 512, "Invalid value for PollInterval, resetting to \"%d\".", cfg.pollinterval);
		printe(PT_Config);
	}

	if (cfg.updateinterval<cfg.pollinterval || cfg.updateinterval>300) {
		if (cfg.pollinterval>UPDATEINTERVAL) {
			cfg.updateinterval = cfg.pollinterval;
		} else {
			cfg.updateinterval = UPDATEINTERVAL;
		}
		snprintf(errorstring, 512, "Invalid value for UpdateInterval, resetting to \"%d\".", cfg.updateinterval);
		printe(PT_Config);
	}

	if ((cfg.saveinterval*60)<cfg.updateinterval || cfg.saveinterval>60) {
		if (cfg.updateinterval>(SAVEINTERVAL*60)) {
			cfg.saveinterval = cfg.updateinterval;
		} else {
			cfg.saveinterval = SAVEINTERVAL;
		}
		snprintf(errorstring, 512, "Invalid value for SaveInterval, resetting to \"%d\".", cfg.saveinterval);
		printe(PT_Config);
	}

	if (cfg.uselogging<0 || cfg.uselogging>2) {
		cfg.uselogging = USELOGGING;
		snprintf(errorstring, 512, "Invalid value for UseLogging, resetting to \"%d\".", cfg.uselogging);
		printe(PT_Config);
	}

	if (cfg.logfile[0] != '/') {
		strncpy(cfg.logfile, LOGFILE, 512);
		snprintf(errorstring, 512, "LogFile doesn't start with \"/\", resetting to default.");
		printe(PT_Config);
	}

	if (cfg.pidfile[0] != '/') {
		strncpy(cfg.pidfile, PIDFILE, 512);
		snprintf(errorstring, 512, "PidFile doesn't start with \"/\", resetting to default.");
		printe(PT_Config);
	}
}

void defaultcfg(void)
{
	ibwflush();

	cfg.bvar = BVAR;
	cfg.qmode = DEFQMODE;
	cfg.sampletime = DEFSAMPTIME;
	cfg.monthrotate = MONTHROTATE;
	cfg.unit = UNITMODE;
	cfg.maxbw = DEFMAXBW;
	cfg.spacecheck = USESPACECHECK;
	cfg.flock = USEFLOCK;
	strncpy(cfg.dbdir, DATABASEDIR, 512);
	strncpy(cfg.iface, DEFIFACE, 32);
	strncpy(cfg.locale, LOCALE, 32);
	strncpy(cfg.dformat, DFORMAT, 64);
	strncpy(cfg.mformat, MFORMAT, 64);
	strncpy(cfg.tformat, TFORMAT, 64);
	strncpy(cfg.hformat, HFORMAT, 32);
	strncpy(cfg.rxchar, RXCHAR, 1);
	strncpy(cfg.txchar, TXCHAR, 1);
	strncpy(cfg.rxhourchar, RXHOURCHAR, 1);
	strncpy(cfg.txhourchar, TXHOURCHAR, 1);

	cfg.updateinterval = UPDATEINTERVAL;
	cfg.pollinterval = POLLINTERVAL;
	cfg.saveinterval = SAVEINTERVAL;
	cfg.uselogging = USELOGGING;
	strncpy(cfg.logfile, LOGFILE, 512);
	strncpy(cfg.pidfile, PIDFILE, 512);
	
	strncpy(cfg.cbg, CBACKGROUND, 7);
	strncpy(cfg.cedge, CEDGE, 7);
	strncpy(cfg.cheader, CHEADER, 7);
	strncpy(cfg.cheadertitle, CHEADERTITLE, 7);
	strncpy(cfg.cheaderdate, CHEADERDATE, 7);
	strncpy(cfg.ctext, CTEXT, 7);
	strncpy(cfg.cline, CLINE, 7);
	strncpy(cfg.clinel, CLINEL, 7);
	strncpy(cfg.crx, CRX, 7);
	strncpy(cfg.crxd, CRXD, 7);
	strncpy(cfg.ctx, CTX, 7);
	strncpy(cfg.ctxd, CTXD, 7);
}

int ibwadd(char *iface, int limit)
{
	ibwnode *p = malloc(sizeof(ibwnode));

	if (p == NULL) {
		return 0;
	}

	p->next = ifacebw;
	ifacebw = p;
	strncpy(p->interface, iface, 32);
	p->limit = limit;

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

	/* search for interface specific limit */
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

	/* return default limit if specified */
	if (cfg.maxbw>0) {
		return cfg.maxbw;
	} else {
		return -1;
	}
}

void ibwflush(void)
{
	ibwnode *f, *p = ifacebw;

	while (p != NULL) {
		f = p;
		p = p->next;
		free(f);
	}
	
	ifacebw = NULL;
}

int ibwcfgread(FILE *fd)
{
	char cfgline[512], name[512], value[512];
	int i, j, linelen, count = 0, ivalue;

	/* start from value search from first line */
	rewind(fd);

	/* cycle all lines */
	while (!feof(fd)) {

		cfgline[0] = '\0';

		/* get current line */
		if (fgets(cfgline, 512, fd)==NULL) {
			break;
		}

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
			
				/* add interface and limit to list if value is within limits */
				ivalue = atoi(value);
				if (ivalue<0 || ivalue>10000) {
					snprintf(errorstring, 512, "Invalid value \"%d\" for MaxBW%s, ignoring parameter.", ivalue, name);
					printe(PT_Config);
				} else {
					ibwadd(name, ivalue);
				}
			}
		}
	}

	return count;
}
