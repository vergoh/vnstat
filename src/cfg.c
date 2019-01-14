#include "common.h"
#include "cfg.h"

int loadcfg(const char *cfgfile)
{
	FILE *fd;
	int i;
	unsigned int linelen, cfglen;

	char value[512], cfgline[512];

	struct cfgsetting cset[] =
	{	/* cfg string, char var name, int var name, char len, fill status */
		{ "Interface", cfg.iface, 0, 32, 0 },
		{ "DatabaseDir", cfg.dbdir, 0, 512, 0 },
		{ "Locale", cfg.locale, 0, 32, 0 },
		{ "MonthRotate", 0, &cfg.monthrotate, 0, 0 },
		{ "MonthRotateAffectsYears", 0, &cfg.monthrotateyears, 0, 0 },
		{ "DayFormat", cfg.dformat, 0, 64, 0 },
		{ "MonthFormat", cfg.mformat, 0, 64, 0 },
		{ "TopFormat", cfg.tformat, 0, 64, 0 },
		{ "RXCharacter", cfg.rxchar, 0, 2, 0 },
		{ "TXCharacter", cfg.txchar, 0, 2, 0 },
		{ "RXHourCharacter", cfg.rxhourchar, 0, 2, 0 },
		{ "TXHourCharacter", cfg.txhourchar, 0, 2, 0 },
		{ "UnitMode", 0, &cfg.unitmode, 0, 0 },
		{ "RateUnitMode", 0, &cfg.rateunitmode, 0, 0 },
		{ "OutputStyle", 0, &cfg.ostyle, 0, 0 },
		{ "RateUnit", 0, &cfg.rateunit, 0, 0 },
		{ "DefaultDecimals", 0, &cfg.defaultdecimals, 0, 0 },
		{ "HourlyDecimals", 0, &cfg.hourlydecimals, 0, 0 },
		{ "HourlySectionStyle", 0, &cfg.hourlystyle, 0, 0 },
		{ "BandwidthDetection", 0, &cfg.bwdetection, 0, 0 },
		{ "MaxBandwidth", 0, &cfg.maxbw, 0, 0 },
		{ "Sampletime", 0, &cfg.sampletime, 0, 0 },
		{ "QueryMode", 0, &cfg.qmode, 0, 0 },
		{ "CheckDiskSpace", 0, &cfg.spacecheck, 0, 0 },
		{ "BootVariation", 0, &cfg.bvar, 0, 0 },
		{ "TrafficlessEntries", 0, &cfg.trafficlessentries, 0, 0 },
		{ "List5Mins", 0, &cfg.listfivemins, 0, 0 },
		{ "ListHours", 0, &cfg.listhours, 0, 0 },
		{ "ListDays", 0, &cfg.listdays, 0, 0 },
		{ "ListMonths", 0, &cfg.listmonths, 0, 0 },
		{ "ListYears", 0, &cfg.listyears, 0, 0 },
		{ "ListTop", 0, &cfg.listtop, 0, 0 },
		{ "5MinuteHours", 0, &cfg.fiveminutehours, 0, 0 },
		{ "HourlyDays", 0, &cfg.hourlydays, 0, 0 },
		{ "DailyDays", 0, &cfg.dailydays, 0, 0 },
		{ "MonthlyMonths", 0, &cfg.monthlymonths, 0, 0 },
		{ "YearlyYears", 0, &cfg.yearlyyears, 0, 0 },
		{ "TopDayEntries", 0, &cfg.topdayentries, 0, 0 },
		{ "DaemonUser", cfg.daemonuser, 0, 33, 0 },
		{ "DaemonGroup", cfg.daemongroup, 0, 33, 0 },
		{ "TimeSyncWait", 0, &cfg.timesyncwait, 0, 0 },
		{ "UpdateInterval", 0, &cfg.updateinterval, 0, 0 },
		{ "PollInterval", 0, &cfg.pollinterval, 0, 0 },
		{ "SaveInterval", 0, &cfg.saveinterval, 0, 0 },
		{ "OfflineSaveInterval", 0, &cfg.offsaveinterval, 0, 0 },
		{ "BandwidthDetectionInterval", 0, &cfg.bwdetectioninterval, 0, 0 },
		{ "SaveOnStatusChange", 0, &cfg.savestatus, 0, 0 },
		{ "UseLogging", 0, &cfg.uselogging, 0, 0 },
		{ "CreateDirs", 0, &cfg.createdirs, 0, 0 },
		{ "UpdateFileOwner", 0, &cfg.updatefileowner, 0, 0 },
		{ "LogFile", cfg.logfile, 0, 512, 0 },
		{ "PidFile", cfg.pidfile, 0, 512, 0 },
		{ "64bitInterfaceCounters", 0, &cfg.is64bit, 0, 0 },
		{ "HeaderFormat", cfg.hformat, 0, 64, 0 },
		{ "HourlyRate", 0, &cfg.hourlyrate, 0, 0 },
		{ "SummaryRate", 0, &cfg.summaryrate, 0, 0 },
		{ "TransparentBg", 0, &cfg.transbg, 0, 0 },
		{ "CBackground", cfg.cbg, 0, 8, 0 },
		{ "CEdge", cfg.cedge, 0, 8, 0 },
		{ "CHeader", cfg.cheader, 0, 8, 0 },
		{ "CHeaderTitle", cfg.cheadertitle, 0, 8, 0 },
		{ "CHeaderDate", cfg.cheaderdate, 0, 8, 0 },
		{ "CText", cfg.ctext, 0, 8, 0 },
		{ "CLine", cfg.cline, 0, 8, 0 },
		{ "CLineL", cfg.clinel, 0, 8, 0 },
		{ "CRx", cfg.crx, 0, 8, 0 },
		{ "CRxD", cfg.crxd, 0, 8, 0 },
		{ "CTx", cfg.ctx, 0, 8, 0 },
		{ "CTxD", cfg.ctxd, 0, 8, 0 },
		{ 0, 0, 0, 0, 0 }
	};

	/* load default config */
	defaultcfg();

	i = opencfgfile(cfgfile, &fd);
	if (i != 2)
		return i;

	rewind(fd);

	/* parse every config file line */
	while (!feof(fd)) {

		cfgline[0] = '\0';
		if (fgets(cfgline, 512, fd)==NULL) {
			break;
		}

		linelen = (unsigned int)strlen(cfgline);
		if (linelen<=2 || cfgline[0]=='#') {
			continue;
		}

		for (i=0; cset[i].name!=0; i++) {

			if (cset[i].found) {
				continue;
			}

			cfglen = (unsigned int)strlen(cset[i].name);
			if ( (linelen<(cfglen+2)) || (strncasecmp(cfgline, cset[i].name, cfglen)!=0) ) {
				continue;
			}

			if (!extractcfgvalue(value, 512, cfgline, cfglen)) {
				if (debug)
					printf("  c: %s   -> \"%s\" with no value, keeping default.\n", cfgline, cset[i].name);
				cset[i].found = 1;
				break;
			}

			if (!setcfgvalue(&cset[i], value, cfgline)) {
				continue;
			}

			cset[i].found = 1;
			break;
		}

		if ((debug) && (!cset[i].found) && (strncasecmp(cfgline, "MaxBW", 5)!=0))
			printf("Unknown configuration line: %s", cfgline);

	}

	fclose(fd);

	/* validate config */
	validatecfg();

	return 1;

}

void validatecfg(void)
{
	const char *invalidvalue = "Invalid value for";
	const char *resettingto = "resetting to";
	const char *noslashstart = "doesn't start with \"/\", resetting to default.";

	if (cfg.unitmode<0 || cfg.unitmode>2) {
		cfg.unitmode = UNITMODE;
		snprintf(errorstring, 1024, "%s UnitMode, %s \"%d\".", invalidvalue, resettingto, cfg.unitmode);
		printe(PT_Config);
	}

	if (cfg.rateunitmode<0 || cfg.rateunitmode>1) {
		cfg.rateunitmode = RATEUNITMODE;
		snprintf(errorstring, 1024, "%s RateUnitMode, %s \"%d\".", invalidvalue, resettingto, cfg.rateunitmode);
		printe(PT_Config);
	}

	if (cfg.ostyle<0 || cfg.ostyle>3) {
		cfg.ostyle = OSTYLE;
		snprintf(errorstring, 1024, "%s OutputStyle, %s \"%d\".", invalidvalue, resettingto, cfg.ostyle);
		printe(PT_Config);
	}

	if (cfg.defaultdecimals<0 || cfg.defaultdecimals>2) {
		cfg.defaultdecimals = DEFAULTDECIMALS;
		snprintf(errorstring, 1024, "%s DefaultDecimals, %s \"%d\".", invalidvalue, resettingto, cfg.defaultdecimals);
		printe(PT_Config);
	}

	if (cfg.hourlydecimals<0 || cfg.hourlydecimals>2) {
		cfg.hourlydecimals = HOURLYDECIMALS;
		snprintf(errorstring, 1024, "%s HourlyDecimals, %s \"%d\".", invalidvalue, resettingto, cfg.hourlydecimals);
		printe(PT_Config);
	}

	if (cfg.hourlystyle<0 || cfg.hourlystyle>3) {
		cfg.hourlystyle = HOURLYSTYLE;
		snprintf(errorstring, 1024, "%s HourlySectionStyle, %s \"%d\".", invalidvalue, resettingto, cfg.hourlystyle);
		printe(PT_Config);
	}

	if (cfg.bvar<0 || cfg.bvar>300) {
		cfg.bvar = BVAR;
		snprintf(errorstring, 1024, "%s BootVariation, %s \"%d\".", invalidvalue, resettingto, cfg.bvar);
		printe(PT_Config);
	}

	if (cfg.sampletime<2 || cfg.sampletime>600) {
		cfg.sampletime = DEFSAMPTIME;
		snprintf(errorstring, 1024, "%s Sampletime, %s \"%d\".", invalidvalue, resettingto, cfg.sampletime);
		printe(PT_Config);
	}

	if (cfg.monthrotate<1 || cfg.monthrotate>28) {
		cfg.monthrotate = MONTHROTATE;
		snprintf(errorstring, 1024, "%s MonthRotate, %s \"%d\".", invalidvalue, resettingto, cfg.monthrotate);
		printe(PT_Config);
	}

	if (cfg.monthrotateyears<0 || cfg.monthrotateyears>1) {
		cfg.monthrotateyears = MONTHROTATEYEARS;
		snprintf(errorstring, 1024, "%s MonthRotateAffectsYears, %s \"%d\".", invalidvalue, resettingto, cfg.monthrotateyears);
		printe(PT_Config);
	}

	if (cfg.maxbw<0 || cfg.maxbw>BWMAX) {
		cfg.maxbw = DEFMAXBW;
		snprintf(errorstring, 1024, "%s MaxBandwidth, %s \"%d\".", invalidvalue, resettingto, cfg.maxbw);
		printe(PT_Config);
	}

	if (cfg.spacecheck<0 || cfg.spacecheck>1) {
		cfg.spacecheck = USESPACECHECK;
		snprintf(errorstring, 1024, "%s CheckDiskSpace, %s \"%d\".", invalidvalue, resettingto, cfg.spacecheck);
		printe(PT_Config);
	}

	if (cfg.dbdir[0] != '/') {
		strncpy_nt(cfg.dbdir, DATABASEDIR, 512);
		snprintf(errorstring, 1024, "DatabaseDir %s", noslashstart);
		printe(PT_Config);
	}

	if (cfg.timesyncwait<0 || cfg.timesyncwait>60) {
		cfg.timesyncwait = TIMESYNCWAIT;
		snprintf(errorstring, 1024, "%s TimeSyncWait, %s \"%d\".", invalidvalue, resettingto, cfg.timesyncwait);
		printe(PT_Config);
	}

	if (cfg.pollinterval<2 || cfg.pollinterval>60) {
		cfg.pollinterval = POLLINTERVAL;
		snprintf(errorstring, 1024, "%s PollInterval, %s \"%d\".", invalidvalue, resettingto, cfg.pollinterval);
		printe(PT_Config);
	}

	if (cfg.updateinterval<cfg.pollinterval || cfg.updateinterval>300) {
		if (cfg.pollinterval>UPDATEINTERVAL) {
			cfg.updateinterval = cfg.pollinterval;
		} else {
			cfg.updateinterval = UPDATEINTERVAL;
		}
		snprintf(errorstring, 1024, "%s UpdateInterval, %s \"%d\".", invalidvalue, resettingto, cfg.updateinterval);
		printe(PT_Config);
	}

	if ((cfg.saveinterval*60)<cfg.updateinterval || cfg.saveinterval>60) {
		if (cfg.updateinterval>(SAVEINTERVAL*60)) {
			cfg.saveinterval = cfg.updateinterval;
		} else {
			cfg.saveinterval = SAVEINTERVAL;
		}
		snprintf(errorstring, 1024, "%s SaveInterval, %s \"%d\".", invalidvalue, resettingto, cfg.saveinterval);
		printe(PT_Config);
	}

	if (cfg.offsaveinterval<cfg.saveinterval || cfg.offsaveinterval>60) {
		if (cfg.saveinterval>OFFSAVEINTERVAL) {
			cfg.offsaveinterval = cfg.saveinterval;
		} else {
			cfg.offsaveinterval = OFFSAVEINTERVAL;
		}
		snprintf(errorstring, 1024, "%s OfflineSaveInterval, %s \"%d\".", invalidvalue, resettingto, cfg.offsaveinterval);
		printe(PT_Config);
	}

	if (cfg.savestatus<0 || cfg.savestatus>1) {
		cfg.savestatus = SAVESTATUS;
		snprintf(errorstring, 1024, "%s SaveOnStatusChange, %s \"%d\".", invalidvalue, resettingto, cfg.savestatus);
		printe(PT_Config);
	}

	if (cfg.uselogging<0 || cfg.uselogging>2) {
		cfg.uselogging = USELOGGING;
		snprintf(errorstring, 1024, "%s UseLogging, %s \"%d\".", invalidvalue, resettingto, cfg.uselogging);
		printe(PT_Config);
	}

	if (cfg.createdirs<0 || cfg.createdirs>2) {
		cfg.createdirs = CREATEDIRS;
		snprintf(errorstring, 1024, "%s CreateDirs, %s \"%d\".", invalidvalue, resettingto, cfg.createdirs);
		printe(PT_Config);
	}

	if (cfg.updatefileowner<0 || cfg.updatefileowner>2) {
		cfg.updatefileowner = UPDATEFILEOWNER;
		snprintf(errorstring, 1024, "%s UpdateFileOwner, %s \"%d\".", invalidvalue, resettingto, cfg.updatefileowner);
		printe(PT_Config);
	}

	if (cfg.logfile[0] != '/') {
		strncpy_nt(cfg.logfile, LOGFILE, 512);
		snprintf(errorstring, 1024, "LogFile %s", noslashstart);
		printe(PT_Config);
	}

	if (cfg.pidfile[0] != '/') {
		strncpy_nt(cfg.pidfile, PIDFILE, 512);
		snprintf(errorstring, 1024, "PidFile %s", noslashstart);
		printe(PT_Config);
	}

	if (cfg.is64bit<-2 || cfg.is64bit>1) {
		cfg.is64bit = IS64BIT;
		snprintf(errorstring, 1024, "%s 64bitInterfaceCounters, %s \"%d\".", invalidvalue, resettingto, cfg.is64bit);
		printe(PT_Config);
	}

	if (cfg.transbg<0 || cfg.transbg>1) {
		cfg.transbg = TRANSBG;
		snprintf(errorstring, 1024, "%s TransparentBg, %s \"%d\".", invalidvalue, resettingto, cfg.transbg);
		printe(PT_Config);
	}

	if (cfg.hourlyrate<0 || cfg.hourlyrate>1) {
		cfg.hourlyrate = HOURLYRATE;
		snprintf(errorstring, 1024, "%s HourlyRate, %s \"%d\".", invalidvalue, resettingto, cfg.hourlyrate);
		printe(PT_Config);
	}

	if (cfg.summaryrate<0 || cfg.summaryrate>1) {
		cfg.summaryrate = SUMMARYRATE;
		snprintf(errorstring, 1024, "%s SummaryRate, %s \"%d\".", invalidvalue, resettingto, cfg.summaryrate);
		printe(PT_Config);
	}

	if (cfg.trafficlessentries<0 || cfg.trafficlessentries>1) {
		cfg.trafficlessentries = TRAFFICLESSENTRIES;
		snprintf(errorstring, 1024, "%s TrafficlessEntries, %s \"%d\".", invalidvalue, resettingto, cfg.trafficlessentries);
		printe(PT_Config);
	}

	if (cfg.listfivemins<0) {
		cfg.listfivemins = LISTFIVEMINS;
		snprintf(errorstring, 1024, "%s List5Mins, %s \"%d\".", invalidvalue, resettingto, cfg.listfivemins);
		printe(PT_Config);
	}

	if (cfg.listhours<0) {
		cfg.listhours = LISTHOURS;
		snprintf(errorstring, 1024, "%s ListHours, %s \"%d\".", invalidvalue, resettingto, cfg.listhours);
		printe(PT_Config);
	}

	if (cfg.listdays<0) {
		cfg.listdays = LISTDAYS;
		snprintf(errorstring, 1024, "%s ListDays, %s \"%d\".", invalidvalue, resettingto, cfg.listdays);
		printe(PT_Config);
	}

	if (cfg.listmonths<0) {
		cfg.listmonths = LISTMONTHS;
		snprintf(errorstring, 1024, "%s ListMonths, %s \"%d\".", invalidvalue, resettingto, cfg.listmonths);
		printe(PT_Config);
	}

	if (cfg.listyears<0) {
		cfg.listyears = LISTYEARS;
		snprintf(errorstring, 1024, "%s ListYears, %s \"%d\".", invalidvalue, resettingto, cfg.listyears);
		printe(PT_Config);
	}

	if (cfg.listtop<0) {
		cfg.listtop = LISTTOP;
		snprintf(errorstring, 1024, "%s ListTop, %s \"%d\".", invalidvalue, resettingto, cfg.listtop);
		printe(PT_Config);
	}

	if (cfg.fiveminutehours<-1) {
		cfg.fiveminutehours = FIVEMINUTEHOURS;
		snprintf(errorstring, 1024, "%s 5MinuteHours, %s \"%d\".", invalidvalue, resettingto, cfg.fiveminutehours);
		printe(PT_Config);
	}

	if (cfg.hourlydays<-1) {
		cfg.hourlydays = HOURLYDAYS;
		snprintf(errorstring, 1024, "%s HourlyDays, %s \"%d\".", invalidvalue, resettingto, cfg.hourlydays);
		printe(PT_Config);
	}

	if (cfg.dailydays<-1) {
		cfg.dailydays = DAILYDAYS;
		snprintf(errorstring, 1024, "%s DailyDays, %s \"%d\".", invalidvalue, resettingto, cfg.dailydays);
		printe(PT_Config);
	}

	if (cfg.monthlymonths<-1) {
		cfg.monthlymonths = MONTHLYMONTHS;
		snprintf(errorstring, 1024, "%s MonthlyMonths, %s \"%d\".", invalidvalue, resettingto, cfg.monthlymonths);
		printe(PT_Config);
	}

	if (cfg.yearlyyears<-1) {
		cfg.yearlyyears = YEARLYYEARS;
		snprintf(errorstring, 1024, "%s YearlyYears, %s \"%d\".", invalidvalue, resettingto, cfg.yearlyyears);
		printe(PT_Config);
	}

	if (cfg.topdayentries<-1) {
		cfg.topdayentries = TOPDAYENTRIES;
		snprintf(errorstring, 1024, "%s TopDayEntries, %s \"%d\".", invalidvalue, resettingto, cfg.topdayentries);
		printe(PT_Config);
	}

	if (cfg.bwdetection<0 || cfg.bwdetection>1) {
		cfg.bwdetection = BWDETECT;
		snprintf(errorstring, 1024, "%s BandwidthDetection, %s \"%d\".", invalidvalue, resettingto, cfg.bwdetection);
		printe(PT_Config);
	}

	if (cfg.bwdetectioninterval<0 || cfg.bwdetectioninterval>30) {
		cfg.bwdetectioninterval = BWDETECTINTERVAL;
		snprintf(errorstring, 1024, "%s BandwidthDetectionInterval, %s \"%d\".", invalidvalue, resettingto, cfg.bwdetectioninterval);
		printe(PT_Config);
	}
}

void defaultcfg(void)
{
	ifacebw = NULL;

	cfg.bvar = BVAR;
	cfg.qmode = DEFQMODE;
	cfg.sampletime = DEFSAMPTIME;
	cfg.monthrotate = MONTHROTATE;
	cfg.monthrotateyears = MONTHROTATEYEARS;
	cfg.unitmode = UNITMODE;
	cfg.rateunitmode = RATEUNITMODE;
	cfg.ostyle = OSTYLE;
	cfg.rateunit = RATEUNIT;
	cfg.defaultdecimals = DEFAULTDECIMALS;
	cfg.hourlydecimals = HOURLYDECIMALS;
	cfg.hourlystyle = HOURLYSTYLE;
	cfg.bwdetection = BWDETECT;
	cfg.bwdetectioninterval = BWDETECTINTERVAL;
	cfg.maxbw = DEFMAXBW;
	cfg.spacecheck = USESPACECHECK;
	cfg.hourlyrate = HOURLYRATE;
	cfg.summaryrate = SUMMARYRATE;
	cfg.trafficlessentries = TRAFFICLESSENTRIES;
	cfg.utflocale = UTFLOCALE;

	cfg.listfivemins = LISTFIVEMINS;
	cfg.listhours = LISTHOURS;
	cfg.listdays = LISTDAYS;
	cfg.listmonths = LISTMONTHS;
	cfg.listyears = LISTYEARS;
	cfg.listtop = LISTTOP;
	cfg.listjsonxml = LISTJSONXML;

	cfg.fiveminutehours = FIVEMINUTEHOURS;
	cfg.hourlydays = HOURLYDAYS;
	cfg.dailydays = DAILYDAYS;
	cfg.monthlymonths = MONTHLYMONTHS;
	cfg.yearlyyears = YEARLYYEARS;
	cfg.topdayentries = TOPDAYENTRIES;

	strncpy_nt(cfg.dbdir, DATABASEDIR, 512);
	strncpy_nt(cfg.iface, DEFIFACE, 32);
	strncpy_nt(cfg.locale, LOCALE, 32);
	strncpy_nt(cfg.dformat, DFORMAT, 64);
	strncpy_nt(cfg.mformat, MFORMAT, 64);
	strncpy_nt(cfg.tformat, TFORMAT, 64);
	strncpy_nt(cfg.hformat, HFORMAT, 64);
	strncpy_nt(cfg.rxchar, RXCHAR, 2);
	strncpy_nt(cfg.txchar, TXCHAR, 2);
	strncpy_nt(cfg.rxhourchar, RXHOURCHAR, 2);
	strncpy_nt(cfg.txhourchar, TXHOURCHAR, 2);

	cfg.daemonuser[0] = '\0';
	cfg.daemongroup[0] = '\0';
	cfg.timesyncwait = TIMESYNCWAIT;
	cfg.updateinterval = UPDATEINTERVAL;
	cfg.pollinterval = POLLINTERVAL;
	cfg.saveinterval = SAVEINTERVAL;
	cfg.offsaveinterval = OFFSAVEINTERVAL;
	cfg.savestatus = SAVESTATUS;
	cfg.uselogging = USELOGGING;
	cfg.createdirs = CREATEDIRS;
	cfg.updatefileowner = UPDATEFILEOWNER;
	strncpy_nt(cfg.logfile, LOGFILE, 512);
	strncpy_nt(cfg.pidfile, PIDFILE, 512);
	cfg.is64bit = IS64BIT;

	cfg.transbg = TRANSBG;
	strncpy_nt(cfg.cbg, CBACKGROUND, 8);
	strncpy_nt(cfg.cedge, CEDGE, 8);
	strncpy_nt(cfg.cheader, CHEADER, 8);
	strncpy_nt(cfg.cheadertitle, CHEADERTITLE, 8);
	strncpy_nt(cfg.cheaderdate, CHEADERDATE, 8);
	strncpy_nt(cfg.ctext, CTEXT, 8);
	strncpy_nt(cfg.cline, CLINE, 8);
	strncpy_nt(cfg.clinel, CLINEL, 8);
	strncpy_nt(cfg.crx, CRX, 8);
	strncpy_nt(cfg.crxd, CRXD, 8);
	strncpy_nt(cfg.ctx, CTX, 8);
	strncpy_nt(cfg.ctxd, CTXD, 8);
}

int opencfgfile(const char *cfgfile, FILE **fd)
{
	char buffer[512];
	int i, tryhome;

	/* clear buffer */
	for (i=0; i<512; i++) {
		buffer[i] = '\0';
	}
	cfg.cfgfile[0] = '\0';

	/* possible config files: 1) --config   2) $HOME/.vnstatrc   3) /etc/vnstat.conf   4) none */

	if (cfgfile[0]!='\0') {

		/* try to open given file */
		if ((*fd=fopen(cfgfile, "r"))!=NULL) {
			strncpy_nt(cfg.cfgfile, cfgfile, 512);
			if (debug)
				printf("Config file: --config\n");
		} else {
			snprintf(errorstring, 1024, "Unable to open given config file \"%s\": %s", cfgfile, strerror(errno));
			printe(PT_Error);
			return 0;
		}

	} else {

		if (getenv("HOME")) {
			strncpy_nt(buffer, getenv("HOME"), 500);
			strcat(buffer, "/.vnstatrc");
			tryhome = 1;
		} else {
			tryhome = 0;
		}

		/* try to open first available config file */
		if (tryhome && (*fd=fopen(buffer, "r"))!=NULL) {
			strncpy_nt(cfg.cfgfile, buffer, 512);
		} else if ((*fd=fopen("/etc/vnstat.conf", "r"))!=NULL) {
			snprintf(cfg.cfgfile, 512, "/etc/vnstat.conf");
		} else if ((*fd=fopen("/usr/local/etc/vnstat.conf", "r"))!=NULL) {
			snprintf(cfg.cfgfile, 512, "/usr/local/etc/vnstat.conf");
		} else {
			if (debug)
				printf("Config file: none\n");
			return 1;
		}
		if (debug)
			printf("Config file: %s\n", cfg.cfgfile);
	}

	return 2;
}

int extractcfgvalue(char *value, const unsigned int valuelen, const char *cfgline, const unsigned int cfglen) {

	unsigned int i, j, linelen;

	linelen = (unsigned int)strlen(cfgline);

	for (i=0; i<valuelen; i++) {
		value[i]='\0';
	}

	i = 0;
	for (j=cfglen; j<linelen; j++) {
		if (cfgline[j]=='\n' || cfgline[j]=='\r') {
			break;
		} else if (cfgline[j]=='\"') {
			if (i==0) {
				continue;
			} else {
				break;
			}
		} else {
			if (i==0 && (cfgline[j]==' ' || cfgline[j]=='=' || cfgline[j]=='\t')) {
				continue;
			} else {
				value[i] = cfgline[j];
				i++;
			}
		}
	}

	return (int)strlen(value);
}

int setcfgvalue(const struct cfgsetting *cset, const char *value, const char *cfgline)
{
	if (cset->namelen>0) {
		strncpy_nt(cset->locc, value, (size_t)cset->namelen);
		if (debug)
			printf("  c: %s   -> \"%s\": \"%s\"\n", cfgline, cset->name, cset->locc);
	} else if ( ( strlen(value)>1 && isdigit(value[1]) ) || isdigit(value[0]) ) {
		*cset->loci = (int32_t)strtol(value, (char **)NULL, 0);
		if (debug)
			printf("  i: %s   -> \"%s\": %d\n", cfgline, cset->name, *cset->loci);
	} else {
		return 0;
	}

	return 1;
}

void configlocale(void)
{
	if (cfg.locale[0]!='-' && strlen(cfg.locale)>0) {
		setlocale(LC_ALL, cfg.locale);
	} else {
		if (getenv("LC_ALL")) {
			setlocale(LC_ALL, getenv("LC_ALL"));
		} else {
			setlocale(LC_ALL, "");
		}
	}
	if (getenv("LC_ALL")) {
		if (strstr(getenv("LC_ALL"), "UTF") != NULL) {
			cfg.utflocale = 1;
		} else {
			cfg.utflocale = 0;
		}
	}
}
