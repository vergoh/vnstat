#include "common.h"

/* global variables */
CFG cfg;
IFINFO ifinfo;
char errorstring[1024];
ibwnode *ifacebw;
int debug;
int noexit;      /* = running as daemon if 2 */
int intsignal;
int pidfile;
int disableprints;


int printe(PrintType type)
{
	int result = 1;

	if (disableprints) {
		return 1;

	/* daemon running but log not enabled */
	} else if (noexit==2 && cfg.uselogging==0) {
		return 1;

	/* daemon running, log enabled */
	} else if (noexit==2) {

		switch (type) {
			case PT_Multiline:
				break;
			case PT_Info:
			case PT_Infoless:
			case PT_Error:
			case PT_Config:
			case PT_ShortMultiline:
				result = logprint(type);
				break;
		}

	/* daemon isn't running */
	} else {

		switch (type) {
			case PT_Info:
				printf("Info: %s\n", errorstring);
				break;
			case PT_Infoless:
				printf("%s\n", errorstring);
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
		}
		fflush(stdout);
	}

	return result;
}

int logprint(PrintType type)
{
	/* buffer needs some extra space for timestamp + infor compared to errorstring */
	char timestamp[22], buffer[1060];
	time_t current;
	FILE *logfile;

	buffer[0] = '\0';

	/* logfile */
	if (cfg.uselogging==1) {

		if (type == PT_Multiline) {
			return 0;
		}

		if ((logfile = fopen(cfg.logfile, "a")) == NULL) {
			return 0;
		}

		current = time(NULL);
		strftime(timestamp, 22, DATETIMEFORMAT, localtime(&current));

		switch (type) {
			case PT_Info:
			case PT_Infoless:
				snprintf(buffer, 1060, "[%s] %s\n", timestamp, errorstring);
				break;
			case PT_Error:
				snprintf(buffer, 1060, "[%s] Error: %s\n", timestamp, errorstring);
				break;
			case PT_Config:
				snprintf(buffer, 1060, "[%s] Config: %s\n", timestamp, errorstring);
				break;
			case PT_Multiline:
				break;
			case PT_ShortMultiline:
				snprintf(buffer, 1060, "[%s] %s\n", timestamp, errorstring);
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
				syslog(LOG_ERR, "Error: %s", errorstring);
				break;
			case PT_Config:
				syslog(LOG_ERR, "Config: %s", errorstring);
				break;
			case PT_Info:
			case PT_Infoless:
			case PT_ShortMultiline:
				syslog(LOG_NOTICE, "%s", errorstring);
				break;
		}

		closelog();
		return 1;
	}

	return 0;
}

int verifylogaccess(void)
{
	FILE *logfile;

	/* only logfile logging can be verified */
	if (cfg.uselogging==1) {
		if ((logfile = fopen(cfg.logfile, "a")) == NULL) {
			return 0;
		}
		fclose(logfile);
	}
	return 1;
}

int dmonth(int month)
{
	static int dmon[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
	int year;
	time_t current;

	/* handle leap years */
	if (month==1) {
		current = time(NULL);
		year = localtime(&current)->tm_year + 1900;
		if (isleapyear(year)) {
			return 29;
		} else {
			return 28;
		}
	} else {
		return dmon[month];
	}
}

int isleapyear(int year)
{
	if (year % 4 != 0) {
		return 0;
	} else if (year % 100 != 0) {
		return 1;
	} else if (year % 400 != 0) {
		return 0;
	}
	return 1;
}

time_t mosecs(time_t month, time_t updated)
{
	struct tm d;
#if defined(_SVID_SOURCE) || defined(_XOPEN_SOURCE) || defined(__APPLE__) || defined(__linux__)
	/* extern long timezone; from time.h */
#else
	int timezone = 0;
#endif

	if (localtime_r(&month, &d) == NULL) {
		return 1;
	}

	d.tm_mday = cfg.monthrotate;
	d.tm_hour = d.tm_min = d.tm_sec = 0;

	if ((updated-month)>0) {
		return updated-mktime(&d)+timezone;
	} else {
		return 1;
	}
}

uint64_t countercalc(const uint64_t *a, const uint64_t *b)
{
	/* no flip */
	if (*b>=*a) {
		if (debug)
			printf("cc: %"PRIu64" - %"PRIu64" = %"PRIu64"\n", *b, *a, *b-*a);
		return *b-*a;

	/* flip exists */
	} else {
		/* original counter is 64bit */
		if (*a>MAX32) {
			if (debug)
				printf("cc64: uint64 - %"PRIu64" + %"PRIu64" = %"PRIu64"\n", *a, *b, (uint64_t)MAX64-*a+*b);
			return MAX64-*a+*b;

		/* original counter is 32bit */
		} else {
			if (debug)
				printf("cc32: uint32 - %"PRIu64" + %"PRIu64" = %"PRIu64"\n", *a, *b, (uint64_t)MAX32-*a+*b);
			return MAX32-*a+*b;
		}
	}
}

/* strncpy with ensured null termination */
char *strncpy_nt(char *dest, const char *src, size_t n)
{
	strncpy(dest, src, n);
	dest[n-1] = '\0';
	return dest;
}

int isnumeric(const char *s)
{
	size_t i, len = strlen(s);

	if (!len) {
		return 0;
	}

	for (i=0; i<len; i++) {
		if (!isdigit(s[i])) {
			return 0;
		}
	}

	return 1;
}

__attribute__((noreturn))
void panicexit(const char *sourcefile, const int sourceline)
{
	snprintf(errorstring, 1024, "Unexpected error (%s), exiting. (%s:%d)", strerror(errno), sourcefile, sourceline);
	fprintf(stderr, "%s\n", errorstring);
	printe(PT_Error);
	exit(EXIT_FAILURE);
}

char *getversion(void)
{
	int i;
	static char versionbuffer[16];
	strncpy_nt(versionbuffer, VERSION, 16);
	for (i=0; i<(int)strlen(versionbuffer); i++) {
		if (versionbuffer[i] == '_') {
			versionbuffer[i] = ' ';
		}
	}
	return versionbuffer;
}

void timeused(const char *func, const int reset)
{
	static struct timeval starttime;
	struct timeval endtime;

	if (!debug) {
		return;
	}

	if (reset) {
		gettimeofday(&starttime, NULL);
		return;
	}

	if (gettimeofday(&endtime, NULL) != 0) {
		return;
	}
	printf("%s() in %f s\n", func, (double)(endtime.tv_usec - starttime.tv_usec) / 1000000 + (double)(endtime.tv_sec - starttime.tv_sec));
}
