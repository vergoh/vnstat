#include "common.h"
#include "cfgoutput.h"

void printcfgfile(void)
{
	ibwnode *p = ifacebw;

	/* common / vnstat section */
	printf("# vnStat %s configuration file\n", getversion());
	printf("#\n");
	printf("# lines starting with # or ; are comments, everything has default\n");
	printf("# values, remove ; before each option to change its value\n\n\n");

	if (cfg.experimental) {
		printf("Experimental %d\n\n", cfg.experimental);
	}

	printf("# default interface (leave empty for automatic selection)\n");
	defaultcomment(strcmp(cfg.iface, DEFIFACE) == 0);
	printf("Interface \"%s\"\n\n", cfg.iface);

	printf("# location of the database directory\n");
	defaultcomment(strcmp(cfg.dbdir, DATABASEDIR) == 0);
	printf("DatabaseDir \"%s\"\n\n", cfg.dbdir);

	printf("# locale (LC_ALL) (\"-\" = use system locale)\n");
	defaultcomment(strcmp(cfg.locale, LOCALE) == 0);
	printf("Locale \"%s\"\n\n", cfg.locale);

	printf("# date output formats for -d, -m, -t and -w\n");
	defaultcomment(strcmp(cfg.dformat, DFORMAT) == 0);
	printf("DayFormat    \"%s\"\n", cfg.dformat);
	defaultcomment(strcmp(cfg.mformat, MFORMAT) == 0);
	printf("MonthFormat  \"%s\"\n", cfg.mformat);
	defaultcomment(strcmp(cfg.tformat, TFORMAT) == 0);
	printf("TopFormat    \"%s\"\n\n", cfg.tformat);

	printf("# characters used for visuals\n");
	defaultcomment(strcmp(cfg.rxchar, RXCHAR) == 0);
	printf("RXCharacter       \"%c\"\n", cfg.rxchar[0]);
	defaultcomment(strcmp(cfg.txchar, TXCHAR) == 0);
	printf("TXCharacter       \"%c\"\n", cfg.txchar[0]);
	defaultcomment(strcmp(cfg.rxhourchar, RXHOURCHAR) == 0);
	printf("RXHourCharacter   \"%c\"\n", cfg.rxhourchar[0]);
	defaultcomment(strcmp(cfg.txhourchar, TXHOURCHAR) == 0);
	printf("TXHourCharacter   \"%c\"\n\n", cfg.txhourchar[0]);

	printf("# how units are prefixed when traffic is shown\n");
	printf("# 0 = IEC standard prefixes (KiB/MiB/GiB...)\n");
	printf("# 1 = old style binary prefixes (KB/MB/GB...)\n");
	printf("# 2 = SI decimal prefixes (kB/MB/GB...)\n");
	defaultcomment(cfg.unitmode == UNITMODE);
	printf("UnitMode %d\n\n", cfg.unitmode);

	printf("# used rate unit (0 = bytes, 1 = bits)\n");
	defaultcomment(cfg.rateunit == RATEUNIT);
	printf("RateUnit %d\n\n", cfg.rateunit);

	printf("# how units are prefixed when traffic rate is shown in bits\n");
	printf("# 0 = IEC binary prefixes (Kibit/s...)\n");
	printf("# 1 = SI decimal prefixes (kbit/s...)\n");
	defaultcomment(cfg.rateunitmode == RATEUNITMODE);
	printf("RateUnitMode %d\n\n", cfg.rateunitmode);

	printf("# output style\n");
	printf("# 0 = minimal & narrow, 1 = bar column visible\n");
	printf("# 2 = same as 1 except rate in summary\n");
	printf("# 3 = rate column visible\n");
	defaultcomment(cfg.ostyle == OSTYLE);
	printf("OutputStyle %d\n", cfg.ostyle);
	defaultcomment(cfg.estimatebarvisible == ESTIMATEBARVISIBLE);
	printf("EstimateBarVisible %d\n\n", cfg.estimatebarvisible);

	printf("# number of decimals to use in outputs\n");
	defaultcomment(cfg.defaultdecimals == DEFAULTDECIMALS);
	printf("DefaultDecimals %d\n", cfg.defaultdecimals);
	defaultcomment(cfg.hourlydecimals == HOURLYDECIMALS);
	printf("HourlyDecimals %d\n\n", cfg.hourlydecimals);

	printf("# spacer for separating hourly sections (0 = none, 1 = '|', 2 = '][', 3 = '[ ]')\n");
	defaultcomment(cfg.hourlystyle == HOURLYSTYLE);
	printf("HourlySectionStyle %d\n\n", cfg.hourlystyle);

	printf("# how many seconds should sampling for -tr take by default\n");
	defaultcomment(cfg.sampletime == DEFSAMPTIME);
	printf("Sampletime %d\n\n", cfg.sampletime);

	printf("# default query mode\n");
	printf("# 0 = normal, 1 = days, 2 = months, 3 = top, 5 = short\n");
	printf("# 7 = hours, 8 = xml, 9 = one line, 10 = json\n");
	defaultcomment(cfg.qmode == DEFQMODE);
	printf("QueryMode %d\n\n", cfg.qmode);

	printf("# default list output entry limits (0 = all)\n");
	defaultcomment(cfg.listfivemins == LISTFIVEMINS);
	printf("List5Mins      %2d\n", cfg.listfivemins);
	defaultcomment(cfg.listhours == LISTHOURS);
	printf("ListHours      %2d\n", cfg.listhours);
	defaultcomment(cfg.listdays == LISTDAYS);
	printf("ListDays       %2d\n", cfg.listdays);
	defaultcomment(cfg.listmonths == LISTMONTHS);
	printf("ListMonths     %2d\n", cfg.listmonths);
	defaultcomment(cfg.listyears == LISTYEARS);
	printf("ListYears      %2d\n", cfg.listyears);
	defaultcomment(cfg.listtop == LISTTOP);
	printf("ListTop        %2d\n\n", cfg.listtop);

	printf("# how to match interface given for query to interface in database\n");
	printf("# 0 = case sensitive exact match to interface name\n");
	printf("# 1 = method 0 followed by case sensitive exact match of alias\n");
	printf("# 2 = method 1 followed by case insensitive exact match of alias\n");
	printf("# 3 = method 2 followed by case insensitive beginning match of alias\n");
	defaultcomment(cfg.ifacematchmethod == IFACEMATCHMETHOD);
	printf("InterfaceMatchMethod %d\n\n", cfg.ifacematchmethod);

	printf("\n");

	/* vnstatd section */
	printf("# vnstatd\n##\n\n");

	printf("# switch to given user when started as root (leave empty to disable)\n");
	defaultcomment(strlen(cfg.daemonuser) == 0);
	printf("DaemonUser \"%s\"\n\n", cfg.daemonuser);

	printf("# switch to given group when started as root (leave empty to disable)\n");
	defaultcomment(strlen(cfg.daemongroup) == 0);
	printf("DaemonGroup \"%s\"\n\n", cfg.daemongroup);

	printf("# try to detect interface maximum bandwidth, 0 = disable feature\n");
	printf("# MaxBandwidth will be used as fallback value when enabled\n");
	defaultcomment(cfg.bwdetection == BWDETECT);
	printf("BandwidthDetection %d\n\n", cfg.bwdetection);

	printf("# maximum bandwidth (Mbit) for all interfaces, 0 = disable feature\n# (unless interface specific limit is given)\n");
	defaultcomment(cfg.maxbw == DEFMAXBW);
	printf("MaxBandwidth %d\n\n", cfg.maxbw);

	printf("# interface specific limits\n");
	printf("#  example 8Mbit limit for eth0 (remove # to activate):\n");
	printf("#MaxBWeth0 8\n");

	while (p != NULL) {
		printf("MaxBW%s %u\n", p->interface, p->limit);
		p = p->next;
	}

	printf("\n");

	printf("# data retention durations (-1 = unlimited, 0 = feature disabled)\n");
	defaultcomment(cfg.fiveminutehours == FIVEMINUTEHOURS);
	printf("5MinuteHours   %2d\n", cfg.fiveminutehours);
	defaultcomment(cfg.hourlydays == HOURLYDAYS);
	printf("HourlyDays     %2d\n", cfg.hourlydays);
	defaultcomment(cfg.dailydays == DAILYDAYS);
	printf("DailyDays      %2d\n", cfg.dailydays);
	defaultcomment(cfg.monthlymonths == MONTHLYMONTHS);
	printf("MonthlyMonths  %2d\n", cfg.monthlymonths);
	defaultcomment(cfg.yearlyyears == YEARLYYEARS);
	printf("YearlyYears    %2d\n", cfg.yearlyyears);
	defaultcomment(cfg.topdayentries == TOPDAYENTRIES);
	printf("TopDayEntries  %2d\n\n", cfg.topdayentries);

	printf("# how often (in seconds) interface data is updated\n");
	defaultcomment(cfg.updateinterval == UPDATEINTERVAL);
	printf("UpdateInterval %d\n\n", cfg.updateinterval);

	printf("# how often (in seconds) interface status changes are checked\n");
	defaultcomment(cfg.pollinterval == POLLINTERVAL);
	printf("PollInterval %d\n\n", cfg.pollinterval);

	printf("# how often (in minutes) data is saved to database\n");
	defaultcomment(cfg.saveinterval == SAVEINTERVAL);
	printf("SaveInterval %d\n\n", cfg.saveinterval);

	printf("# how often (in minutes) data is saved when all interface are offline\n");
	defaultcomment(cfg.offsaveinterval == OFFSAVEINTERVAL);
	printf("OfflineSaveInterval %d\n\n", cfg.offsaveinterval);

	printf("# rescan database after save for new interfaces to be monitored (1 = enabled, 0 = disabled)\n");
	defaultcomment(cfg.rescanonsave == RESCANONSAVE);
	printf("RescanDatabaseOnSave %d\n\n", cfg.rescanonsave);

	printf("# automatically start monitoring all interfaces not found in the database\n");
	printf("# (1 = enabled, 0 = disabled)\n");
	defaultcomment(cfg.alwaysadd == ALWAYSADD);
	printf("AlwaysAddNewInterfaces %d\n\n", cfg.alwaysadd);

	printf("# on which day should months change\n");
	defaultcomment(cfg.monthrotate == MONTHROTATE);
	printf("MonthRotate %d\n", cfg.monthrotate);
	defaultcomment(cfg.monthrotateyears == MONTHROTATEYEARS);
	printf("MonthRotateAffectsYears %d\n\n", cfg.monthrotateyears);

	printf("# filesystem disk space check (1 = enabled, 0 = disabled)\n");
	defaultcomment(cfg.spacecheck == USESPACECHECK);
	printf("CheckDiskSpace %d\n\n", cfg.spacecheck);

	printf("# how much the boot time can variate between updates (seconds)\n");
	defaultcomment(cfg.bvar == BVAR);
	printf("BootVariation %d\n\n", cfg.bvar);

	printf("# create database entries even when there is no traffic (1 = enabled, 0 = disabled)\n");
	defaultcomment(cfg.trafficlessentries == TRAFFICLESSENTRIES);
	printf("TrafficlessEntries %d\n\n", cfg.trafficlessentries);

	printf("# how many minutes to wait during daemon startup for system clock to\n");
	printf("# sync time if most recent database update appears to be in the future\n");
	defaultcomment(cfg.timesyncwait == TIMESYNCWAIT);
	printf("TimeSyncWait %d\n\n", cfg.timesyncwait);

	printf("# how often (in minutes) bandwidth detection is done when\n");
	printf("# BandwidthDetection is enabled (0 = disabled)\n");
	defaultcomment(cfg.bwdetectioninterval == BWDETECTINTERVAL);
	printf("BandwidthDetectionInterval %d\n\n", cfg.bwdetectioninterval);

	printf("# force data save when interface status changes (1 = enabled, 0 = disabled)\n");
	defaultcomment(cfg.savestatus == SAVESTATUS);
	printf("SaveOnStatusChange %d\n\n", cfg.savestatus);

	printf("# enable / disable logging (0 = disabled, 1 = logfile, 2 = syslog)\n");
	defaultcomment(cfg.uselogging == USELOGGING);
	printf("UseLogging %d\n\n", cfg.uselogging);

	printf("# create dirs if needed (1 = enabled, 0 = disabled)\n");
	defaultcomment(cfg.createdirs == CREATEDIRS);
	printf("CreateDirs %d\n\n", cfg.createdirs);

	printf("# update ownership of files if needed (1 = enabled, 0 = disabled)\n");
	defaultcomment(cfg.updatefileowner == UPDATEFILEOWNER);
	printf("UpdateFileOwner %d\n\n", cfg.updatefileowner);

	printf("# file used for logging if UseLogging is set to 1\n");
	defaultcomment(strcmp(cfg.logfile, LOGFILE) == 0);
	printf("LogFile \"%s\"\n\n", cfg.logfile);

	printf("# file used as daemon pid / lock file\n");
	defaultcomment(strcmp(cfg.pidfile, PIDFILE) == 0);
	printf("PidFile \"%s\"\n\n", cfg.pidfile);

	printf("# 1 = 64-bit, 0 = 32-bit, -1 = old style logic, -2 = automatic detection\n");
	defaultcomment(cfg.is64bit == IS64BIT);
	printf("64bitInterfaceCounters %d\n\n", cfg.is64bit);

	printf("# use SQLite Write-Ahead Logging mode (1 = enabled, 0 = disabled)\n");
	defaultcomment(cfg.waldb == WALDB);
	printf("DatabaseWriteAheadLogging %d\n\n", cfg.waldb);

	printf("# change the setting of the SQLite \"synchronous\" flag\n");
	printf("# (-1 = auto, 0 = off, 1, = normal, 2 = full, 3 = extra)\n");
	defaultcomment(cfg.dbsynchronous == DBSYNCHRONOUS);
	printf("DatabaseSynchronous %d\n\n", cfg.dbsynchronous);

	printf("# database uses UTC instead of local timezone (1 = enabled, 0 = disabled)\n");
	defaultcomment(cfg.useutc == USEUTC);
	printf("UseUTC %d\n\n", cfg.useutc);

	printf("\n");

	/* vnstati section */
	printf("# vnstati\n##\n\n");

	printf("# title timestamp format\n");
	defaultcomment(strcmp(cfg.hformat, HFORMAT) == 0);
	printf("HeaderFormat \"%s\"\n\n", cfg.hformat);

	printf("# show hours with rate (1 = enabled, 0 = disabled)\n");
	defaultcomment(cfg.hourlyrate == HOURLYRATE);
	printf("HourlyRate %d\n\n", cfg.hourlyrate);

	printf("# show rate in summary (1 = enabled, 0 = disabled)\n");
	defaultcomment(cfg.summaryrate == SUMMARYRATE);
	printf("SummaryRate %d\n\n", cfg.summaryrate);

	printf("# transparent background (1 = enabled, 0 = disabled)\n");
	defaultcomment(cfg.transbg == TRANSBG);
	printf("TransparentBg %d\n\n", cfg.transbg);

	printf("# image size control\n");
	defaultcomment(cfg.largefonts == LARGEFONTS);
	printf("LargeFonts %d\n", cfg.largefonts);
	defaultcomment(cfg.linespaceadjust == LINESPACEADJUST);
	printf("LineSpacingAdjustment %d\n", cfg.linespaceadjust);
	defaultcomment(cfg.imagescale == IMAGESCALE);
	printf("ImageScale %d\n\n", cfg.imagescale);

	printf("# 5 minutes graph size control\n");
	defaultcomment(cfg.fivegresultcount == FIVEGRESULTCOUNT);
	printf("5MinuteGraphResultCount %d\n", cfg.fivegresultcount);
	defaultcomment(cfg.fivegheight == FIVEGHEIGHT);
	printf("5MinuteGraphHeight %d\n\n", cfg.fivegheight);

	printf("# hourly graph mode (0 = 24 hour sliding window, 1 = begins from midnight)\n");
	defaultcomment(cfg.hourlygmode == HOURLYGMODE);
	printf("HourlyGraphMode %d\n\n", cfg.hourlygmode);

	printf("# horizontal/vertical summary graph (0 = hours, 1 = 5 minutes)\n");
	defaultcomment(cfg.summarygraph == SUMMARYGRAPH);
	printf("SummaryGraph %d\n\n", cfg.summarygraph);

	printf("# traffic estimate bar style\n");
	printf("# (0 = not shown, 1 = continuation of existing bar, 2 = separate bar)\n");
	defaultcomment(cfg.estimatestyle == ESTIMATESTYLE);
	printf("EstimateStyle %d\n\n", cfg.estimatestyle);

	printf("# bar column in list outputs shows rate if OutputStyle is 3\n");
	printf("# (1 = enabled, 0 = disabled)\n");
	defaultcomment(cfg.barshowsrate == BARSHOWSRATE);
	printf("BarColumnShowsRate %d\n\n", cfg.barshowsrate);

	printf("# image colors\n");
	defaultcomment(strcmp(cfg.cbg, CBACKGROUND) == 0);
	printf("CBackground     \"%s\"\n", cfg.cbg);
	defaultcomment(strcmp(cfg.cedge, CEDGE) == 0);
	printf("CEdge           \"%s\"\n", cfg.cedge);
	defaultcomment(strcmp(cfg.cheader, CHEADER) == 0);
	printf("CHeader         \"%s\"\n", cfg.cheader);
	defaultcomment(strcmp(cfg.cheadertitle, CHEADERTITLE) == 0);
	printf("CHeaderTitle    \"%s\"\n", cfg.cheadertitle);
	defaultcomment(strcmp(cfg.cheaderdate, CHEADERDATE) == 0);
	printf("CHeaderDate     \"%s\"\n", cfg.cheaderdate);
	defaultcomment(strcmp(cfg.ctext, CTEXT) == 0);
	printf("CText           \"%s\"\n", cfg.ctext);
	defaultcomment(strcmp(cfg.cline, CLINE) == 0);
	printf("CLine           \"%s\"\n", cfg.cline);
	defaultcomment(strcmp(cfg.clinel, CLINEL) == 0);
	printf("CLineL          \"%s\"\n", cfg.clinel);
	defaultcomment(strcmp(cfg.crx, CRX) == 0);
	printf("CRx             \"%s\"\n", cfg.crx);
	defaultcomment(strcmp(cfg.ctx, CTX) == 0);
	printf("CTx             \"%s\"\n", cfg.ctx);
	defaultcomment(strcmp(cfg.crxd, CRXD) == 0);
	printf("CRxD            \"%s\"\n", cfg.crxd);
	defaultcomment(strcmp(cfg.ctxd, CTXD) == 0);
	printf("CTxD            \"%s\"\n", cfg.ctxd);
}

void defaultcomment(const int isdefault)
{
	if (isdefault) {
		printf(";");
	}
}
