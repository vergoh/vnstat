#include "common.h"
#include "cfgoutput.h"

void printcfgfile(void)
{
	ibwnode *p = ifacebw;

	/* common/vnstat section */
	printf("# vnStat %s config file\n", getversion());
	printf("##\n\n");

	printf("# default interface\n");
	printf("Interface \"%s\"\n\n", cfg.iface);

	printf("# location of the database directory\n");
	printf("DatabaseDir \"%s\"\n\n", cfg.dbdir);

	printf("# locale (LC_ALL) (\"-\" = use system locale)\n");
	printf("Locale \"%s\"\n\n", cfg.locale);

	printf("# date output formats for -d, -m, -t and -w\n");
	printf("DayFormat    \"%s\"\n", cfg.dformat);
	printf("MonthFormat  \"%s\"\n", cfg.mformat);
	printf("TopFormat    \"%s\"\n\n", cfg.tformat);

	printf("# characters used for visuals\n");
	printf("RXCharacter       \"%c\"\n", cfg.rxchar[0]);
	printf("TXCharacter       \"%c\"\n", cfg.txchar[0]);
	printf("RXHourCharacter   \"%c\"\n", cfg.rxhourchar[0]);
	printf("TXHourCharacter   \"%c\"\n\n", cfg.txhourchar[0]);

	printf("# how units are prefixed when traffic is shown\n");
	printf("# 0 = IEC standard prefixes (KiB/MiB/GiB...)\n");
	printf("# 1 = old style binary prefixes (KB/MB/GB...)\n");
	printf("# 2 = SI decimal prefixes (kB/MB/GB...)\n");
	printf("UnitMode %d\n\n", cfg.unitmode);

	printf("# used rate unit (0 = bytes, 1 = bits)\n");
	printf("RateUnit %d\n\n", cfg.rateunit);

	printf("# how units are prefixed when traffic rate is shown in bits\n");
	printf("# 0 = IEC binary prefixes (Kibit/s...)\n");
	printf("# 1 = SI decimal prefixes (kbit/s...)\n");
	printf("RateUnitMode %d\n\n", cfg.rateunitmode);

	printf("# output style\n");
	printf("# 0 = minimal & narrow, 1 = bar column visible\n");
	printf("# 2 = same as 1 except rate in summary\n");
	printf("# 3 = rate column visible\n");
	printf("OutputStyle %d\n\n", cfg.ostyle);

	printf("# number of decimals to use in outputs\n");
	printf("DefaultDecimals %d\n", cfg.defaultdecimals);
	printf("HourlyDecimals %d\n\n", cfg.hourlydecimals);

	printf("# spacer for separating hourly sections (0 = none, 1 = '|', 2 = '][', 3 = '[ ]')\n");
	printf("HourlySectionStyle %d\n\n", cfg.hourlystyle);

	printf("# how many seconds should sampling for -tr take by default\n");
	printf("Sampletime %d\n\n", cfg.sampletime);

	printf("# default query mode\n");
	printf("# 0 = normal, 1 = days, 2 = months, 3 = top, 5 = short\n");
	printf("# 7 = hours, 8 = xml, 9 = one line, 10 = json\n");
	printf("QueryMode %d\n\n", cfg.qmode);

	printf("# default list output entry count (0 = all)\n");
	printf("List5Mins      %2d\n", cfg.listfivemins);
	printf("ListHours      %2d\n", cfg.listhours);
	printf("ListDays       %2d\n", cfg.listdays);
	printf("ListMonths     %2d\n", cfg.listmonths);
	printf("ListYears      %2d\n", cfg.listyears);
	printf("ListTop        %2d\n\n", cfg.listtop);

	printf("\n");

	/* vnstatd section */
	printf("# vnstatd\n##\n\n");

	printf("# switch to given user when started as root (leave empty to disable)\n");
	printf("DaemonUser \"%s\"\n\n", cfg.daemonuser);

	printf("# switch to given group when started as root (leave empty to disable)\n");
	printf("DaemonGroup \"%s\"\n\n", cfg.daemongroup);

	printf("# try to detect interface maximum bandwidth, 0 = disable feature\n");
	printf("# MaxBandwidth will be used as fallback value when enabled\n");
	printf("BandwidthDetection %d\n\n", cfg.bwdetection);

	printf("# maximum bandwidth (Mbit) for all interfaces, 0 = disable feature\n# (unless interface specific limit is given)\n");
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
	printf("5MinuteHours   %2d\n", cfg.fiveminutehours);
	printf("HourlyDays     %2d\n", cfg.hourlydays);
	printf("DailyDays      %2d\n", cfg.dailydays);
	printf("MonthlyMonths  %2d\n", cfg.monthlymonths);
	printf("YearlyYears    %2d\n", cfg.yearlyyears);
	printf("TopDayEntries  %2d\n\n", cfg.topdayentries);

	printf("# how often (in seconds) interface data is updated\n");
	printf("UpdateInterval %d\n\n", cfg.updateinterval);

	printf("# how often (in seconds) interface status changes are checked\n");
	printf("PollInterval %d\n\n", cfg.pollinterval);

	printf("# how often (in minutes) data is saved to database\n");
	printf("SaveInterval %d\n\n", cfg.saveinterval);

	printf("# how often (in minutes) data is saved when all interface are offline\n");
	printf("OfflineSaveInterval %d\n\n", cfg.offsaveinterval);

	printf("# on which day should months change\n");
	printf("MonthRotate %d\n", cfg.monthrotate);
	printf("MonthRotateAffectsYears %d\n\n", cfg.monthrotateyears);

	printf("# filesystem disk space check (1 = enabled, 0 = disabled)\n");
	printf("CheckDiskSpace %d\n\n", cfg.spacecheck);

	printf("# how much the boot time can variate between updates (seconds)\n");
	printf("BootVariation %d\n\n", cfg.bvar);

	printf("# create database entries even when there is no traffic (1 = enabled, 0 = disabled)\n");
	printf("TrafficlessEntries %d\n\n", cfg.trafficlessentries);

	printf("# how many minutes to wait during daemon startup for system clock to\n");
	printf("# sync time if most recent database update appears to be in the future\n");
	printf("TimeSyncWait %d\n\n", cfg.timesyncwait);

	printf("# how often (in minutes) bandwidth detection is done when\n");
	printf("# BandwidthDetection is enabled (0 = disabled)\n");
	printf("BandwidthDetectionInterval %d\n\n", cfg.bwdetectioninterval);

	printf("# force data save when interface status changes (1 = enabled, 0 = disabled)\n");
	printf("SaveOnStatusChange %d\n\n", cfg.savestatus);

	printf("# enable / disable logging (0 = disabled, 1 = logfile, 2 = syslog)\n");
	printf("UseLogging %d\n\n", cfg.uselogging);

	printf("# create dirs if needed (1 = enabled, 0 = disabled)\n");
	printf("CreateDirs %d\n\n", cfg.createdirs);

	printf("# update ownership of files if needed (1 = enabled, 0 = disabled)\n");
	printf("UpdateFileOwner %d\n\n", cfg.updatefileowner);

	printf("# file used for logging if UseLogging is set to 1\n");
	printf("LogFile \"%s\"\n\n", cfg.logfile);

	printf("# file used as daemon pid / lock file\n");
	printf("PidFile \"%s\"\n\n", cfg.pidfile);

	printf("# 1 = 64-bit, 0 = 32-bit, -1 = old style logic, -2 = automatic detection\n");
	printf("64bitInterfaceCounters %d\n\n", cfg.is64bit);

	printf("# use SQLite Write-Ahead Logging mode (1 = enabled, 0 = disabled)\n");
	printf("WriteAheadLoggingDatabase %d\n\n", cfg.waldb);

	printf("\n");

	/* vnstati section */
	printf("# vnstati\n##\n\n");

	printf("# title timestamp format\n");
	printf("HeaderFormat \"%s\"\n\n", cfg.hformat);

	printf("# show hours with rate (1 = enabled, 0 = disabled)\n");
	printf("HourlyRate %d\n\n", cfg.hourlyrate);

	printf("# show rate in summary (1 = enabled, 0 = disabled)\n");
	printf("SummaryRate %d\n\n", cfg.summaryrate);

	printf("# transparent background (1 = enabled, 0 = disabled)\n");
	printf("TransparentBg %d\n\n", cfg.transbg);

	printf("# image colors\n");
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
