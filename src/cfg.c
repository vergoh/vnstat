#include "common.h"
#include "cfg.h"

int loadcfg(const char *cfgfile)
{
	FILE *fd;
	int i;
	unsigned int linelen, cfglen;

	char value[512], cfgline[512];

	struct cfgsetting cset[] =
		{/* cfg string, char var name, int var name, char len, fill status */
		 {"Interface", cfg.iface, 0, 32, 0},
		 {"DatabaseDir", cfg.dbdir, 0, 512, 0},
		 {"Locale", cfg.locale, 0, 32, 0},
		 {"MonthRotate", 0, &cfg.monthrotate, 0, 0},
		 {"MonthRotateAffectsYears", 0, &cfg.monthrotateyears, 0, 0},
		 {"DayFormat", cfg.dformat, 0, 64, 0},
		 {"MonthFormat", cfg.mformat, 0, 64, 0},
		 {"TopFormat", cfg.tformat, 0, 64, 0},
		 {"RXCharacter", cfg.rxchar, 0, 2, 0},
		 {"TXCharacter", cfg.txchar, 0, 2, 0},
		 {"RXHourCharacter", cfg.rxhourchar, 0, 2, 0},
		 {"TXHourCharacter", cfg.txhourchar, 0, 2, 0},
		 {"UnitMode", 0, &cfg.unitmode, 0, 0},
		 {"RateUnitMode", 0, &cfg.rateunitmode, 0, 0},
		 {"OutputStyle", 0, &cfg.ostyle, 0, 0},
		 {"EstimateBarVisible", 0, &cfg.estimatebarvisible, 0, 0},
		 {"RateUnit", 0, &cfg.rateunit, 0, 0},
		 {"DefaultDecimals", 0, &cfg.defaultdecimals, 0, 0},
		 {"HourlyDecimals", 0, &cfg.hourlydecimals, 0, 0},
		 {"HourlySectionStyle", 0, &cfg.hourlystyle, 0, 0},
		 {"BandwidthDetection", 0, &cfg.bwdetection, 0, 0},
		 {"MaxBandwidth", 0, &cfg.maxbw, 0, 0},
		 {"Sampletime", 0, &cfg.sampletime, 0, 0},
		 {"QueryMode", 0, &cfg.qmode, 0, 0},
		 {"CheckDiskSpace", 0, &cfg.spacecheck, 0, 0},
		 {"BootVariation", 0, &cfg.bvar, 0, 0},
		 {"TrafficlessEntries", 0, &cfg.trafficlessentries, 0, 0},
		 {"List5Mins", 0, &cfg.listfivemins, 0, 0},
		 {"ListHours", 0, &cfg.listhours, 0, 0},
		 {"ListDays", 0, &cfg.listdays, 0, 0},
		 {"ListMonths", 0, &cfg.listmonths, 0, 0},
		 {"ListYears", 0, &cfg.listyears, 0, 0},
		 {"ListTop", 0, &cfg.listtop, 0, 0},
		 {"5MinuteHours", 0, &cfg.fiveminutehours, 0, 0},
		 {"HourlyDays", 0, &cfg.hourlydays, 0, 0},
		 {"DailyDays", 0, &cfg.dailydays, 0, 0},
		 {"MonthlyMonths", 0, &cfg.monthlymonths, 0, 0},
		 {"YearlyYears", 0, &cfg.yearlyyears, 0, 0},
		 {"TopDayEntries", 0, &cfg.topdayentries, 0, 0},
		 {"DaemonUser", cfg.daemonuser, 0, 33, 0},
		 {"DaemonGroup", cfg.daemongroup, 0, 33, 0},
		 {"TimeSyncWait", 0, &cfg.timesyncwait, 0, 0},
		 {"UpdateInterval", 0, &cfg.updateinterval, 0, 0},
		 {"PollInterval", 0, &cfg.pollinterval, 0, 0},
		 {"SaveInterval", 0, &cfg.saveinterval, 0, 0},
		 {"OfflineSaveInterval", 0, &cfg.offsaveinterval, 0, 0},
		 {"BandwidthDetectionInterval", 0, &cfg.bwdetectioninterval, 0, 0},
		 {"SaveOnStatusChange", 0, &cfg.savestatus, 0, 0},
		 {"UseLogging", 0, &cfg.uselogging, 0, 0},
		 {"CreateDirs", 0, &cfg.createdirs, 0, 0},
		 {"UpdateFileOwner", 0, &cfg.updatefileowner, 0, 0},
		 {"LogFile", cfg.logfile, 0, 512, 0},
		 {"PidFile", cfg.pidfile, 0, 512, 0},
		 {"64bitInterfaceCounters", 0, &cfg.is64bit, 0, 0},
		 {"DatabaseWriteAheadLogging", 0, &cfg.waldb, 0, 0},
		 {"DatabaseSynchronous", 0, &cfg.dbsynchronous, 0, 0},
		 {"HeaderFormat", cfg.hformat, 0, 64, 0},
		 {"HourlyRate", 0, &cfg.hourlyrate, 0, 0},
		 {"SummaryRate", 0, &cfg.summaryrate, 0, 0},
		 {"TransparentBg", 0, &cfg.transbg, 0, 0},
		 {"LargeFonts", 0, &cfg.largefonts, 0, 0},
		 {"LineSpacingAdjustment", 0, &cfg.linespaceadjust, 0, 0},
		 {"ImageScale", 0, &cfg.imagescale, 0, 0},
		 {"5MinuteGraphResultCount", 0, &cfg.fivegresultcount, 0, 0},
		 {"5MinuteGraphHeight", 0, &cfg.fivegheight, 0, 0},
		 {"EstimateStyle", 0, &cfg.estimatestyle, 0, 0},
		 {"BarColumnShowsRate", 0, &cfg.barshowsrate, 0, 0},
		 {"CBackground", cfg.cbg, 0, 8, 0},
		 {"CEdge", cfg.cedge, 0, 8, 0},
		 {"CHeader", cfg.cheader, 0, 8, 0},
		 {"CHeaderTitle", cfg.cheadertitle, 0, 8, 0},
		 {"CHeaderDate", cfg.cheaderdate, 0, 8, 0},
		 {"CText", cfg.ctext, 0, 8, 0},
		 {"CLine", cfg.cline, 0, 8, 0},
		 {"CLineL", cfg.clinel, 0, 8, 0},
		 {"CRx", cfg.crx, 0, 8, 0},
		 {"CRxD", cfg.crxd, 0, 8, 0},
		 {"CTx", cfg.ctx, 0, 8, 0},
		 {"CTxD", cfg.ctxd, 0, 8, 0},
		 {"Experimental", 0, &cfg.experimental, 0, 0},
		 {0, 0, 0, 0, 0}};

	/* load default config */
	defaultcfg();

	i = opencfgfile(cfgfile, &fd);
	if (i != 2)
		return i;

	rewind(fd);

	/* parse every config file line */
	while (!feof(fd)) {

		cfgline[0] = '\0';
		if (fgets(cfgline, 512, fd) == NULL) {
			break;
		}

		linelen = (unsigned int)strlen(cfgline);
		if (linelen <= 2 || cfgline[0] == '#') {
			continue;
		}

		for (i = 0; cset[i].name != 0; i++) {

			if (cset[i].found) {
				continue;
			}

			cfglen = (unsigned int)strlen(cset[i].name);
			if ((linelen < (cfglen + 2)) || (strncasecmp(cfgline, cset[i].name, cfglen) != 0)) {
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

		if ((debug) && (!cset[i].found) && (strncasecmp(cfgline, "MaxBW", 5) != 0))
			printf("Unknown configuration line: %s", cfgline);
	}

	fclose(fd);

	/* validate config */
	validatecfg();

	return 1;
}

void validatebool(const char *cfgname, int32_t *cfgptr, const int32_t defaultvalue)
{
	validateint(cfgname, cfgptr, defaultvalue, 0, 1);
}

void validateint(const char *cfgname, int32_t *cfgptr, const int32_t defaultvalue, const int32_t minvalue, const int32_t maxvalue)
{
	if (maxvalue > minvalue) {
		if (*cfgptr < minvalue || *cfgptr > maxvalue) {
			*cfgptr = defaultvalue;
			snprintf(errorstring, 1024, "Invalid value for %s, resetting to \"%d\".", cfgname, *cfgptr);
			printe(PT_Config);
		}
	} else {
		if (*cfgptr < minvalue) {
			*cfgptr = defaultvalue;
			snprintf(errorstring, 1024, "Invalid value for %s, resetting to \"%d\".", cfgname, *cfgptr);
			printe(PT_Config);
		}
	}
}

void validatecfg(void)
{
	uint32_t rolloversecs;
	const char *invalidvalue = "Invalid value for";
	const char *resettingto = "resetting to";
	const char *noslashstart = "doesn't start with \"/\", resetting to default.";

	validateint("UnitMode", &cfg.unitmode, UNITMODE, 0, 2);
	validatebool("RateUnitMode", &cfg.rateunitmode, RATEUNITMODE);
	validateint("OutputStyle", &cfg.ostyle, OSTYLE, 0, 3);
	validatebool("EstimateBarVisible", &cfg.estimatebarvisible, ESTIMATEBARVISIBLE);
	validateint("DefaultDecimals", &cfg.defaultdecimals, DEFAULTDECIMALS, 0, 2);
	validateint("HourlyDecimals", &cfg.hourlydecimals, HOURLYDECIMALS, 0, 2);
	validateint("HourlySectionStyle", &cfg.hourlystyle, HOURLYSTYLE, 0, 3);
	validateint("BootVariation", &cfg.bvar, BVAR, 0, 300);
	validateint("Sampletime", &cfg.sampletime, DEFSAMPTIME, 2, 600);
	validateint("MonthRotate", &cfg.monthrotate, MONTHROTATE, 1, 28);
	validatebool("MonthRotateAffectsYears", &cfg.monthrotateyears, MONTHROTATEYEARS);
	validateint("MaxBandwidth", &cfg.maxbw, DEFMAXBW, 0, BWMAX);
	validatebool("CheckDiskSpace", &cfg.spacecheck, USESPACECHECK);
	validateint("TimeSyncWait", &cfg.timesyncwait, TIMESYNCWAIT, 0, 60);
	validateint("PollInterval", &cfg.pollinterval, POLLINTERVAL, 2, 60);
	validatebool("SaveOnStatusChange", &cfg.savestatus, SAVESTATUS);
	validateint("UseLogging", &cfg.uselogging, USELOGGING, 0, 2);
	validateint("CreateDirs", &cfg.createdirs, CREATEDIRS, 0, 2);
	validateint("UpdateFileOwner", &cfg.updatefileowner, UPDATEFILEOWNER, 0, 2);
	validateint("64bitInterfaceCounters", &cfg.is64bit, IS64BIT, -2, 1);
	validatebool("DatabaseWriteAheadLogging", &cfg.waldb, WALDB);
	validateint("DatabaseSynchronous", &cfg.dbsynchronous, DBSYNCHRONOUS, -1, 3);
	validatebool("TransparentBg", &cfg.transbg, TRANSBG);
	validatebool("LargeFonts", &cfg.largefonts, LARGEFONTS);
	validateint("LineSpacingAdjustment", &cfg.linespaceadjust, LINESPACEADJUST, -5, 10);
	validateint("ImageScale", &cfg.imagescale, IMAGESCALE, 50, 500);
	validateint("5MinuteGraphResultCount", &cfg.fivegresultcount, FIVEGRESULTCOUNT, FIVEGMINRESULTCOUNT, 2000);
	validateint("5MinuteGraphHeight", &cfg.fivegheight, FIVEGHEIGHT, FIVEGMINHEIGHT, 2000);
	validateint("EstimateStyle", &cfg.estimatestyle, ESTIMATESTYLE, 0, 2);
	validatebool("BarColumnShowsRate", &cfg.barshowsrate, BARSHOWSRATE);
	validatebool("HourlyRate", &cfg.hourlyrate, HOURLYRATE);
	validatebool("SummaryRate", &cfg.summaryrate, SUMMARYRATE);
	validatebool("TrafficlessEntries", &cfg.trafficlessentries, TRAFFICLESSENTRIES);
	validateint("List5Mins", &cfg.listfivemins, LISTFIVEMINS, 0, 0);
	validateint("ListHours", &cfg.listhours, LISTHOURS, 0, 0);
	validateint("ListDays", &cfg.listdays, LISTDAYS, 0, 0);
	validateint("ListMonths", &cfg.listmonths, LISTMONTHS, 0, 0);
	validateint("ListYears", &cfg.listyears, LISTYEARS, 0, 0);
	validateint("ListTop", &cfg.listtop, LISTTOP, 0, 0);
	validateint("5MinuteHours", &cfg.fiveminutehours, FIVEMINUTEHOURS, -1, -1);
	validateint("HourlyDays", &cfg.hourlydays, HOURLYDAYS, -1, -1);
	validateint("DailyDays", &cfg.dailydays, DAILYDAYS, -1, -1);
	validateint("MonthlyMonths", &cfg.monthlymonths, MONTHLYMONTHS, -1, -1);
	validateint("YearlyYears", &cfg.yearlyyears, YEARLYYEARS, -1, -1);
	validateint("TopDayEntries", &cfg.topdayentries, TOPDAYENTRIES, -1, -1);
	validatebool("BandwidthDetection", &cfg.bwdetection, BWDETECT);
	validateint("BandwidthDetectionInterval", &cfg.bwdetectioninterval, BWDETECTINTERVAL, 0, 30);
	validatebool("Experimental", &cfg.experimental, 0);

	if (cfg.dbdir[0] != '/') {
		strncpy_nt(cfg.dbdir, DATABASEDIR, 512);
		snprintf(errorstring, 1024, "DatabaseDir %s", noslashstart);
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

	if (cfg.updateinterval < cfg.pollinterval || cfg.updateinterval > 300) {
		if (cfg.pollinterval > UPDATEINTERVAL) {
			cfg.updateinterval = cfg.pollinterval;
		} else {
			cfg.updateinterval = UPDATEINTERVAL;
		}
		snprintf(errorstring, 1024, "%s UpdateInterval, %s \"%d\".", invalidvalue, resettingto, cfg.updateinterval);
		printe(PT_Config);
	}

	if ((cfg.saveinterval * 60) < cfg.updateinterval || cfg.saveinterval > 60) {
		if (cfg.updateinterval > (SAVEINTERVAL * 60)) {
			cfg.saveinterval = cfg.updateinterval;
		} else {
			cfg.saveinterval = SAVEINTERVAL;
		}
		snprintf(errorstring, 1024, "%s SaveInterval, %s \"%d\".", invalidvalue, resettingto, cfg.saveinterval);
		printe(PT_Config);
	}

	if (cfg.offsaveinterval < cfg.saveinterval || cfg.offsaveinterval > 60) {
		if (cfg.saveinterval > OFFSAVEINTERVAL) {
			cfg.offsaveinterval = cfg.saveinterval;
		} else {
			cfg.offsaveinterval = OFFSAVEINTERVAL;
		}
		snprintf(errorstring, 1024, "%s OfflineSaveInterval, %s \"%d\".", invalidvalue, resettingto, cfg.offsaveinterval);
		printe(PT_Config);
	}

	/* enforce update interval to be short enough that 32-bit interface counter rollover can be detected */
	/* 1.02 is the same 2% safety buffer as used in processifinfo() in daemon.c */
	/* noexit check results in warning being shown only when the daemon is started */
	if (noexit && cfg.maxbw > 0) {
		rolloversecs = (uint32_t)((float)MAX32 / ((float)cfg.maxbw * 1024 * 1024 * (float)1.02 / 8));
		if (rolloversecs <= (uint32_t)cfg.updateinterval) {
			cfg.updateinterval = UPDATEINTERVAL;
			if (rolloversecs <= (uint32_t)cfg.updateinterval) {
				cfg.updateinterval /= 2;
			}
			snprintf(errorstring, 1024, "UpdateInterval has been reset to %d seconds in order to ensure correct counter rollover detection at %d Mbit.", cfg.updateinterval, cfg.maxbw);
			printe(PT_Config);
		}
	}

	/* affects only image output */
	if (cfg.barshowsrate && cfg.estimatebarvisible) {
		cfg.estimatestyle = 0;
		if (debug) {
			printf("BarColumnShowsRate and EstimateBarVisible both enabled -> EstimateStyle set to 0\n");
		}
	}

	if (cfg.fiveminutehours > 0 && cfg.fivegresultcount > cfg.fiveminutehours * 12) {
		cfg.fivegresultcount = cfg.fiveminutehours * 12;
		snprintf(errorstring, 1024, "5MinuteGraphResultCount has been reset to %d due to request being larger than data retention configured with 5MinuteHours.", cfg.fivegresultcount);
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
	cfg.estimatebarvisible = ESTIMATEBARVISIBLE;
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
	cfg.waldb = WALDB;
	cfg.dbsynchronous = DBSYNCHRONOUS;

	cfg.transbg = TRANSBG;
	cfg.largefonts = LARGEFONTS;
	cfg.linespaceadjust = LINESPACEADJUST;
	cfg.imagescale = IMAGESCALE;
	cfg.fivegresultcount = FIVEGRESULTCOUNT;
	cfg.fivegheight = FIVEGHEIGHT;
	cfg.estimatestyle = ESTIMATESTYLE;
	cfg.barshowsrate = BARSHOWSRATE;
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

	cfg.experimental = 0;
}

int opencfgfile(const char *cfgfile, FILE **fd)
{
	char buffer[512];
	int i, tryhome;

	/* clear buffer */
	for (i = 0; i < 512; i++) {
		buffer[i] = '\0';
	}
	cfg.cfgfile[0] = '\0';

	/* possible config files: 1) --config   2) $HOME/.vnstatrc   3) /etc/vnstat.conf   4) none */

	if (cfgfile[0] != '\0') {

		/* try to open given file */
		if ((*fd = fopen(cfgfile, "r")) != NULL) {
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
		if (tryhome && (*fd = fopen(buffer, "r")) != NULL) {
			strncpy_nt(cfg.cfgfile, buffer, 512);
		} else if ((*fd = fopen("/etc/vnstat.conf", "r")) != NULL) {
			snprintf(cfg.cfgfile, 512, "/etc/vnstat.conf");
		} else if ((*fd = fopen("/usr/local/etc/vnstat.conf", "r")) != NULL) {
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

int extractcfgvalue(char *value, const unsigned int valuelen, const char *cfgline, const unsigned int cfglen)
{

	unsigned int i, j, linelen;

	linelen = (unsigned int)strlen(cfgline);

	for (i = 0; i < valuelen; i++) {
		value[i] = '\0';
	}

	i = 0;
	for (j = cfglen; j < linelen; j++) {
		if (cfgline[j] == '\n' || cfgline[j] == '\r') {
			break;
		} else if (cfgline[j] == '\"') {
			if (i == 0) {
				continue;
			} else {
				break;
			}
		} else {
			if (i == 0 && (cfgline[j] == ' ' || cfgline[j] == '=' || cfgline[j] == '\t')) {
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
	if (cset->namelen > 0) {
		strncpy_nt(cset->locc, value, (size_t)cset->namelen);
		if (debug)
			printf("  c: %s   -> \"%s\": \"%s\"\n", cfgline, cset->name, cset->locc);
	} else if ((strlen(value) > 1 && isdigit(value[1])) || isdigit(value[0])) {
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
	if (cfg.locale[0] != '-' && strlen(cfg.locale) > 0) {
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
