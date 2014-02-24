#include "common.h"

int printe(PrintType type)
{
	int result = 1;

	/* daemon running but log not enabled */
	if (noexit==2 && cfg.uselogging==0) {
		return 1;

	/* daemon running, log enabled */
	} else if (noexit==2) {

		switch (type) {
			case PT_Multiline:
				break;
			default:
				result = logprint(type);
				break;
		}

	/* daemon isn't running */
	} else {

		switch (type) {
			case PT_Info:
				printf("Info: %s\n", errorstring);
				break;
			case PT_Error:
				printf("Error: %s\n", errorstring);
				break;
			case PT_Config:
				printf("Config: %s\n", errorstring);
				break;
			case PT_Multiline:
				printf("%s\n", errorstring);
				break;
			case PT_ShortMultiline:
				break;
			default:
				printf("%d: %s\n", type, errorstring);
				break;
		}	

	}

	return result;
}

int logprint(PrintType type)
{
	char timestamp[22], buffer[512];
	time_t current;
	FILE *logfile;

	/* logfile */
	if (cfg.uselogging==1) {

		if ((logfile = fopen(cfg.logfile, "a")) == NULL) {
			return 0;
		}

		current = time(NULL);
		strftime(timestamp, 22, "%Y.%m.%d %H:%M:%S", localtime(&current));

		switch (type) {
			case PT_Info:
				snprintf(buffer, 512, "[%s] %s\n", timestamp, errorstring);
				break;
			case PT_Error:
				snprintf(buffer, 512, "[%s] Error: %s\n", timestamp, errorstring);
				break;
			case PT_Config:
				snprintf(buffer, 512, "[%s] Config: %s\n", timestamp, errorstring);
				break;
			case PT_Multiline:
				break;
			case PT_ShortMultiline:
				snprintf(buffer, 512, "[%s] %s\n", timestamp, errorstring);
				break;
			default:
				snprintf(buffer, 512, "[%s] (%d): %s\n", timestamp, type, errorstring);
				break;
		}

		if (fwrite(buffer, strlen(buffer), 1, logfile)!=1) {
			fclose(logfile);
			return 0;
		}

		fclose(logfile);
		return 1;

	/* syslog */
	} else if (cfg.uselogging==2) {

		openlog("vnstatd", LOG_PID, LOG_DAEMON);

		switch (type) {
			case PT_Multiline:
				break;
			case PT_Error:
				snprintf(buffer, 512, "Error: %s", errorstring);
				syslog(LOG_ERR, "%s", buffer);
				break;
			case PT_Config:
				snprintf(buffer, 512, "Config: %s", errorstring);
				syslog(LOG_ERR, "%s", buffer);
				break;
			case PT_Info:
			case PT_ShortMultiline:
				snprintf(buffer, 512, "%s", errorstring);
				syslog(LOG_NOTICE, "%s", buffer);
				break;
			default:
				snprintf(buffer, 512, "(%d): %s", type, errorstring);
				syslog(LOG_NOTICE, "%s", buffer);
				break;
		}

		closelog();
		return 1;
	}

	return 0;
}

int dmonth(int month)
{
	static int dmon[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
	int year;
	time_t current;

	/* handle leap years */
	if (month==1) {
		current = time(NULL);
		year = localtime(&current)->tm_year;
		if ( ((year % 4 == 0) && (year % 100 != 0)) || (year % 400 == 0) ) {
			return 29;
		} else {
			return 28;
		}
	} else {
		return dmon[month];
	}
}

uint32_t mosecs(void)
{
	struct tm *d;

	d = localtime(&data.month[0].month);

	if (d == NULL) {
		return 0;
	}

	if (d->tm_mday < cfg.monthrotate) {
		return 0;
	}

	d->tm_mday = cfg.monthrotate;
	d->tm_hour = d->tm_min = d->tm_sec = 0;

	if ((data.lastupdated-data.month[0].month)>0) {
		return data.lastupdated-mktime(d);
	} else {
		return 0;
	}
}
