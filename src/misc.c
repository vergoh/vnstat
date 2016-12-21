#if !defined(__FreeBSD__) && !defined(__NetBSD__) && !defined(__OpenBSD__) && !defined(__APPLE__) && !defined(__FreeBSD_kernel__)
#define _XOPEN_SOURCE 600
#endif
#include "common.h"
#include "misc.h"
#include <wchar.h>

int kerneltest(void)
{
	int i=0;
	uint64_t bmax, bmin, btemp;

	bmax=bmin=getbtime();

	printf("This test will take about 60 seconds.\n");
	printf("[                              ]");
	for (i=0; i<=30; i++) {
		printf("\b");
	}
	fflush(stdout);

	for (i=0;i<30;i++) {
		sleep(2);
		fflush(stdout);

		btemp=getbtime();

		if (btemp > bmax) {
			bmax = btemp;
		}
		if (btemp < bmin) {
			bmin = btemp;
		}

		printf("=");
		fflush(stdout);
	}

	printf("] done.\n\n");

	printf("Detected boot time variation during test:  %2d\n", (int)(bmax-bmin));
	printf("Maximum boot time variation set in config: %2d\n\n", cfg.bvar);

	if ((bmax-bmin)>20) {
		printf("The current kernel has a broken boot time information and\n");
		printf("vnStat is likely not to work correctly. Upgrading the kernel\n");
		printf("is likely to solve this problem.\n\n");
		return 1;
	} else if ((bmax-bmin)>(uint32_t)cfg.bvar) {
		printf("The current kernel has a boot time variation greater than assumed\n");
		printf("in the vnStat config. That it likely to cause errors in results.\n");
		printf("Set \"BootVariation\" to something greater than \"%d\" and run this\n", (int)(bmax-bmin));
		printf("test again.\n\n");
		return 1;
	} else if ((bmax-bmin)==0) {
		printf("The current kernel doesn't seem to suffer from boot time variation problems.\n");
		printf("Everything is ok.\n\n");
		return 0;
	} else {
		printf("The current kernel is ok.\n\n");
		return 0;
	}

}

int spacecheck(char *path)
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
			snprintf(errorstring, 512, "Free diskspace check failed: %s", strerror(errno));
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
				snprintf(errorstring, 512, "DEBUG: SIGHUP (%d)", sig);
				break;

			case SIGTERM:
				snprintf(errorstring, 512, "DEBUG: SIGTERM (%d)", sig);
				break;

			case SIGINT:
				snprintf(errorstring, 512, "DEBUG: SIGINT (%d)", sig);
				break;

			default:
				snprintf(errorstring, 512, "DEBUG: Unknown signal %d", sig);
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
		snprintf(errorstring, 512, "Unable to read /proc/stat: %s", strerror(errno));
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
		snprintf(errorstring, 512, "btime missing from /proc/stat.");
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

char *getvalue(uint64_t mb, uint64_t kb, int len, int type)
{
	static char buffer[64];
	int declen=2;
	uint64_t kB;

	/* request types: 1) normal  2) estimate  3) image scale */
	if (type==3) {
		declen=0;
	}

	if (mb!=0) {
		kB=mbkbtokb(mb, kb);
	} else {
		kB=kb;
	}

	/* tune spacing according to unit */
	len -= strlen(getunitprefix(1)) + 1;
	if (len<0) {
		len = 1;
	}

	if ( (type==2) && (kB==0) ){
		snprintf(buffer, 64, "%*s    ", len-cfg.unitmode, "--");
	} else {
		/* try to figure out what unit to use */
		if (kB>=1048576000) { /* 1024*1024*1000 - value >=1000 GiB -> show in TiB */
			snprintf(buffer, 64, "%"DECCONV"*.*f %s", len, declen, kB/(float)1073741824, getunitprefix(4)); /* 1024*1024*1024 */
		} else if (kB>=1024000) { /* 1024*1000 - value >=1000 MiB -> show in GiB */
			snprintf(buffer, 64, "%"DECCONV"*.*f %s", len, declen, kB/(float)1048576, getunitprefix(3)); /* 1024*1024 */
		} else if (kB>=1000) {
			if (type==2) {
				declen=0;
			}
			snprintf(buffer, 64, "%"DECCONV"*.*f %s", len, declen, kB/(float)1024, getunitprefix(2));
		} else {
			snprintf(buffer, 64, "%"DECCONV"*"PRIu64" %s", len, kB, getunitprefix(1));
		}
	}

	return buffer;
}

char *getrate(uint64_t mb, uint64_t kb, uint32_t interval, int len)
{
	return gettrafficrate(mbkbtokb(mb, kb) * 1024, interval, len);
}

char *gettrafficrate(uint64_t bytes, uint32_t interval, int len)
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

uint64_t getscale(uint64_t kb)
{
	int i;
	uint64_t result;

	result = kb;

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

char *getunitprefix(int index)
{
	static char *unitprefix[] = { "na", "KiB", "MiB", "GiB", "TiB",
                                   "KB",  "MB",  "GB",  "TB" };

	if (index>UNITPREFIXCOUNT) {
		return unitprefix[0];
	} else {
		return unitprefix[(cfg.unitmode*UNITPREFIXCOUNT)+index];
	}
}

char *getrateunitprefix(int unitmode, int index)
{
	static char *rateunitprefix[] = { "na", "KiB/s", "MiB/s", "GiB/s", "TiB/s",
                                    "KB/s",  "MB/s",  "GB/s",  "TB/s",
                                    "Kibit/s",  "Mibit/s",  "Gibit/s",  "Tibit/s",
                                    "kbit/s",  "Mbit/s",  "Gbit/s",  "Tbit/s" };

	if (index>UNITPREFIXCOUNT) {
		return rateunitprefix[0];
	} else {
		return rateunitprefix[(unitmode*UNITPREFIXCOUNT)+index];
	}
}

uint64_t getunitdivisor(int unitmode, int index)
{
	uint64_t unitdiv[] = { 0, 1024, 1048576, 1073741824, 1099511627776,
                              1024, 1048576, 1073741824, 1099511627776,
                              1024, 1048576, 1073741824, 1099511627776,
                              1000, 1000000, 1000000000, 1000000000000};

	if (index>UNITPREFIXCOUNT) {
		return unitdiv[0];
	} else {
		return unitdiv[(unitmode*UNITPREFIXCOUNT)+index];
	}
}

char *getratestring(uint64_t rate, int len, int declen, int unitmode)
{
	static char buffer[64];
	uint64_t limit[3] = { 1024000, 1048576000, 1073741824000 };

	if (cfg.rateunit == 1 && cfg.rateunitmode == 1) {
		limit[0] = 1000000;
		limit[1] = 1000000000;
		limit[2] = 1000000000000;
	}

	/* tune spacing according to unit */
	len -= strlen(getrateunitprefix(unitmode, 1)) + 1;
	if (len<0) {
		len = 1;
	}

	/* try to figure out what unit to use */
	if (rate>=limit[2]) {
		snprintf(buffer, 64, "%"DECCONV"*.2f %s", len, rate/(float)getunitdivisor(unitmode, 4), getrateunitprefix(unitmode, 4));
	} else if (rate>=limit[1]) {
		snprintf(buffer, 64, "%"DECCONV"*.2f %s", len, rate/(float)getunitdivisor(unitmode, 3), getrateunitprefix(unitmode, 3));
	} else if (rate>=limit[0]) {
		snprintf(buffer, 64, "%"DECCONV"*.2f %s", len, rate/(float)getunitdivisor(unitmode, 2), getrateunitprefix(unitmode, 2));
	} else {
		snprintf(buffer, 64, "%"DECCONV"*.*f %s", len, declen, rate/(float)getunitdivisor(unitmode, 1), getrateunitprefix(unitmode, 1));
	}

	return buffer;
}

int getpadding(int len, char *str)
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

void cursortocolumn(int column)
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
