#if !defined(__FreeBSD__) && !defined(__NetBSD__) && !defined(__OpenBSD__) && !defined(__APPLE__) && !defined(__FreeBSD_kernel__)
#define _XOPEN_SOURCE 600
#endif
#include "common.h"
#include "misc.h"
#include <wchar.h>

int spacecheck(const char *path)
{
	struct statvfs buf;
	uint64_t free;

	/* do space check only when configured for it */
	if (!cfg.spacecheck) {
		return 1;
	}

	if (statvfs(path, &buf)) {
		if (noexit) {
			return 0;
		} else {
			snprintf(errorstring, 1024, "Free diskspace check failed: %s", strerror(errno));
			printe(PT_Error);
			exit(EXIT_FAILURE);
		}
	}

	free=(buf.f_bavail/(float)1024)*buf.f_bsize;

	if (debug) {
		printf("bsize %d\n", (int)buf.f_bsize);
		printf("blocks %lu\n", (unsigned long int)buf.f_blocks);
		printf("bfree %lu\n", (unsigned long int)buf.f_bfree);
		printf("bavail %lu\n", (unsigned long int)buf.f_bavail);
		printf("ffree %lu\n", (unsigned long int)buf.f_ffree);
		printf("%"PRIu64" free space left\n", free);
	}

	/* the database is less than 3kB but let's require */
	/* 1MB to be on the safe side, anyway, the filesystem should */
	/* always have more free space than that */
	if (free<=1024) {
		return 0;
	} else {
		return 1;
	}
}

void sighandler(int sig)
{
	/* set signal */
	intsignal = sig;

	if (debug) {
		switch (sig) {

			case SIGHUP:
				snprintf(errorstring, 1024, "DEBUG: SIGHUP (%d)", sig);
				break;

			case SIGTERM:
				snprintf(errorstring, 1024, "DEBUG: SIGTERM (%d)", sig);
				break;

			case SIGINT:
				snprintf(errorstring, 1024, "DEBUG: SIGINT (%d)", sig);
				break;

			default:
				snprintf(errorstring, 1024, "DEBUG: Unknown signal %d", sig);
				break;

		}
		printe(PT_Info);
	}
}

uint64_t getbtime(void)
{
	uint64_t result=0;
#if defined(__linux__)
	FILE *fp;
	int check;
	char temp[64], statline[128];

	if ((fp=fopen("/proc/stat","r"))==NULL) {
		snprintf(errorstring, 1024, "Unable to read /proc/stat: %s", strerror(errno));
		printe(PT_Error);
		if (noexit) {
			return 0;
		} else {
			exit(1);
		}
	}

	check=0;
	while (fgets(statline,128,fp)!=NULL) {
		sscanf(statline,"%63s",temp);
		if (strcmp(temp,"btime")==0) {
			/* if (debug)
				printf("\n%s\n",statline); */
			check=1;
			break;
		}
	}
	fclose(fp);

	if (check==0) {
		snprintf(errorstring, 1024, "btime missing from /proc/stat.");
		printe(PT_Error);
		if (noexit) {
			return 0;
		} else {
			exit(1);
		}
	}

	result = strtoull(statline+6, (char **)NULL, 0);

#elif defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__) || defined(__APPLE__) || defined(__FreeBSD_kernel__)
	struct timeval btm;
	size_t len = sizeof(btm);
	int mib[2] = {CTL_KERN, KERN_BOOTTIME};

	if (sysctl(mib, 2, &btm, &len, NULL, 0) < 0) {
		if (debug)
			printf("sysctl(kern.boottime) failed.\n");
		return 0;
	}

	result = (uint64_t)btm.tv_sec;
#endif

	return result;
}

char *getvalue(const uint64_t bytes, const int len, const int type)
{
	static char buffer[64];
	int declen = 2;

	/* request types: 1) normal  2) estimate  3) image scale */
	if (type == 3) {
		declen = 0;
	}

	if ( (type == 2) && (bytes == 0) ){
		declen = len-(int)strlen(getunitprefix(2))-2;
		if (declen < 2) {
			declen = 2;
		}
		snprintf(buffer, 64, "%*s  %*s", declen, "--", (int)strlen(getunitprefix(2)), " ");
	} else {
		/* try to figure out what unit to use */
		if (bytes >= 1073741824000) { /* 1024*1024*1024*1000 - value >=1000 GiB -> show in TiB */
			snprintf(buffer, 64, "%"DECCONV"*.*f %s", getunitspacing(len, 5), declen, bytes/(float)1099511627776, getunitprefix(5)); /* 1024*1024*1024*1024 */
		} else if (bytes >= 1048576000) { /* 1024*1024*1000 - value >=1000 MiB -> show in GiB */
			snprintf(buffer, 64, "%"DECCONV"*.*f %s", getunitspacing(len, 4), declen, bytes/(float)1073741824, getunitprefix(4)); /* 1024*1024*1024 */
		} else if (bytes >= 1024000) { /* 1024*1000 - value >=1000 B -> show in MiB */
			snprintf(buffer, 64, "%"DECCONV"*.*f %s", getunitspacing(len, 3), declen, bytes/(float)1048576, getunitprefix(3)); /* 1024*1024 */
		} else if (bytes >= 1000) { /* show in KiB */
			if (type == 2) {
				declen = 0;
			}
			snprintf(buffer, 64, "%"DECCONV"*.*f %s", getunitspacing(len, 2), declen, bytes/(float)1024, getunitprefix(2));
		} else {
			snprintf(buffer, 64, "%"DECCONV"*"PRIu64" %s", getunitspacing(len, 1), bytes, getunitprefix(1));
		}
	}

	return buffer;
}

int getunitspacing(const int len, const int index)
{
	int l = len;

	/* tune spacing according to unit */
	/* +1 for space between number and unit */
	l -= strlen(getunitprefix(index)) + 1;
	if (l < 0) {
		l = 1;
	}

	return l;
}

char *gettrafficrate(const uint64_t bytes, const uint32_t interval, const int len)
{
	static char buffer[64];
	int unitmode, declen = 2;
	uint64_t b;

	if (interval==0) {
		snprintf(buffer, 64, "%*s", len, "n/a");
		return buffer;
	}

	/* convert to proper unit */
	if (cfg.rateunit == 1) {
		b = bytes * 8;
		unitmode = 3;
		if (cfg.rateunitmode == 0) {
			unitmode = 2;
		}
		if (interval<5) {
			declen = 0;
		}
	} else {
		b = bytes;
		unitmode = cfg.unitmode;
	}

	return getratestring(b / interval, len, declen, unitmode);
}

uint64_t getscale(const uint64_t input)
{
	int i;
	uint64_t result;

	result = input;

	/* get unit */
	for (i=0; result>1024; i++) {
		result = result / 1024;
	}

	/* round result depending of scale */
	if (result>300) {
		result = result/4 + (100 - ((result/4) % 100));
	} else if (result>20) {
		result = result/4 + (10 - ((result/4) % 10));
	} else {
		result = result/4;
	}

	/* put unit back */
	if (i) {
		result = result * pow(1024, i);
	}

	/* make sure result isn't zero */
	if (!result) {
		result = pow(1024, i);
	}

	return result;
}

char *getunitprefix(const int index)
{
	static char *unitprefix[] = { "na", "B", "KiB", "MiB", "GiB", "TiB",
                                        "B", "KB",  "MB",  "GB",  "TB" };

	if (index>UNITPREFIXCOUNT) {
		return unitprefix[0];
	} else {
		return unitprefix[(cfg.unitmode*UNITPREFIXCOUNT)+index];
	}
}

char *getrateunitprefix(const int unitmode, const int index)
{
	static char *rateunitprefix[] = { "na", "KiB/s",   "MiB/s",   "GiB/s",   "TiB/s",
                                            "KB/s",    "MB/s",    "GB/s",    "TB/s",
                                            "Kibit/s", "Mibit/s", "Gibit/s", "Tibit/s",
                                            "kbit/s",  "Mbit/s",  "Gbit/s",  "Tbit/s" };

	if (index>UNITPREFIXCOUNT-1) {
		return rateunitprefix[0];
	} else {
		return rateunitprefix[(unitmode*(UNITPREFIXCOUNT-1))+index];
	}
}

uint64_t getunitdivisor(const int unitmode, const int index)
{
	uint64_t unitdiv[] = { 1, 1024, 1048576, 1073741824, 1099511627776,
                              1024, 1048576, 1073741824, 1099511627776,
                              1024, 1048576, 1073741824, 1099511627776,
                              1000, 1000000, 1000000000, 1000000000000};

	if (index>UNITPREFIXCOUNT-1) {
		return unitdiv[0];
	} else {
		return unitdiv[(unitmode*(UNITPREFIXCOUNT-1))+index];
	}
}

char *getratestring(const uint64_t rate, const int len, const int declen, const int unitmode)
{
	int l = len;
	static char buffer[64];
	uint64_t limit[3] = { 1024000, 1048576000, 1073741824000 };

	if (cfg.rateunit == 1 && cfg.rateunitmode == 1) {
		limit[0] = 1000000;
		limit[1] = 1000000000;
		limit[2] = 1000000000000;
	}

	/* tune spacing according to unit */
	l -= strlen(getrateunitprefix(unitmode, 1)) + 1;
	if (l < 0) {
		l = 1;
	}

	/* try to figure out what unit to use */
	if (rate>=limit[2]) {
		snprintf(buffer, 64, "%"DECCONV"*.2f %s", l, rate/(float)getunitdivisor(unitmode, 4), getrateunitprefix(unitmode, 4));
	} else if (rate>=limit[1]) {
		snprintf(buffer, 64, "%"DECCONV"*.2f %s", l, rate/(float)getunitdivisor(unitmode, 3), getrateunitprefix(unitmode, 3));
	} else if (rate>=limit[0]) {
		snprintf(buffer, 64, "%"DECCONV"*.2f %s", l, rate/(float)getunitdivisor(unitmode, 2), getrateunitprefix(unitmode, 2));
	} else {
		snprintf(buffer, 64, "%"DECCONV"*.*f %s", l, declen, rate/(float)getunitdivisor(unitmode, 1), getrateunitprefix(unitmode, 1));
	}

	return buffer;
}

int getpadding(const int len, const char *str)
{
#if defined(HAVE_MBSTOWCS) && defined(HAVE_WCSWIDTH)
	wchar_t wbuffer[64];
	if (!cfg.utflocale) {
		return len;
	}
	if ((int)mbstowcs(wbuffer, str, 64) < 0) {
		return len;
	}
	return len + ((int)strlen(str) - wcswidth(wbuffer, 64));
#else
	return len;
#endif
}

void cursortocolumn(const int column)
{
	printf("\033[%dG", column);
}

void cursorhide(void)
{
	printf("\033[?25l");
}

void cursorshow(void)
{
	printf("\033[?25h");
}

void eraseline(void)
{
	printf("\033[2K");
}
