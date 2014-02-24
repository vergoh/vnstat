#include "common.h"
#include "misc.h"

int kerneltest(void)
{
	int i=0, bmax, bmin, btemp;

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

	printf("Detected boot time variation during test:  %2d\n", bmax-bmin);
	printf("Maximum boot time variation set in config: %2d\n\n", cfg.bvar);

	if ((bmax-bmin)>20) {
		printf("The current kernel has a broken boot time information and\n");
		printf("vnStat is likely not to work correctly. Upgrading the kernel\n");
		printf("is likely to solve this problem.\n\n");
		return 1;
	} else if ((bmax-bmin)>cfg.bvar) {
		printf("The current kernel has a boot time variation greater than assumed\n");
		printf("in the vnStat config. That it likely to cause errors in results.\n");
		printf("Set \"BootVariation\" to something greater than \"%d\" and run this\n", (bmax-bmin));
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
		snprintf(errorstring, 512, "Free diskspace check failed.");
		printe(PT_Error);
		if (noexit) {
			return 0;
		} else {
			exit(1);
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

int getbtime(void)
{
	int result=0;
#if defined(__linux__)
	FILE *fp;
	int check;
	char temp[64], statline[128];

	if ((fp=fopen("/proc/stat","r"))==NULL) {
		snprintf(errorstring, 512, "Unable to read /proc/stat.");
		printe(PT_Error);
		if (noexit) {
			return 0;
		} else {
			exit(1);
		}
	}
	
	check=0;
	while (fgets(statline,128,fp)!=NULL) {
		sscanf(statline,"%64s",temp);
		if (strcmp(temp,"btime")==0) {
			/* if (debug)
				printf("\n%s\n",statline); */
			check=1;
			break;
		}
	}
	
	if (check==0) {
		snprintf(errorstring, 512, "btime missing from /proc/stat.");
		printe(PT_Error);
		if (noexit) {
			return 0;
		} else {
			exit(1);
		}
	}
	
	result = strtoul(statline+6, (char **)NULL, 0);
	fclose(fp);

#elif defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__) || defined(__APPLE__)
	struct timeval btm;
	size_t len = sizeof(btm);
	int mib[2] = {CTL_KERN, KERN_BOOTTIME};
	
	if (sysctl(mib, 2, &btm, &len, NULL, 0) < 0) {
		if (debug)
			printf("sysctl(kern.boottime) failed.\n");
		return 0;
	}
	
	result = btm.tv_sec;
#endif

	return result;
}

void addtraffic(uint64_t *destmb, int *destkb, uint64_t srcmb, int srckb)
{
        *destmb=*destmb+srcmb;
        *destkb=*destkb+srckb;

        if (*destkb>=1024) {
                *destmb+=*destkb/1024;
                *destkb-=(*destkb/1024)*1024;
        }
}

char *getvalue(uint64_t mb, uint64_t kb, int len, int type)
{
	static char buffer[64];
	int declen=2, pad=-3;
	uint64_t kB;

	/* negative type disables left padding of value */
	if (type<0) {
		type=type*-1;
		pad=0;
		if (cfg.unit>0) {
			len++;
		}
	}

	/* request types: 1) normal  2) estimate  3) image scale */
	if (type==3) {
		declen=0;
	}

	if (mb!=0) {
		if (kb>=1024) {
			mb+=kb/1024;
			kb-=(kb/1024)*1024;
		}
		kB=(mb*1024)+kb;
	} else {
		kB=kb;
	}

	if ( (type==2) && (kB==0) ){
		sprintf(buffer, "%*s    ", len, "--");
	} else {
#if !defined(__OpenBSD__)
		/* try to figure out what unit to use */
		if (kB>=1048576000) { /* 1024*1024*1000 - value >=1000 GiB -> show in TiB */
			sprintf(buffer, "%'*.*f %*s", len, declen, kB/(float)1073741824, pad, getunit(4)); /* 1024*1024*1024 */
		} else if (kB>=1024000) { /* 1024*1000 - value >=1000 MiB -> show in GiB */
			sprintf(buffer, "%'*.*f %*s", len, declen, kB/(float)1048576, pad, getunit(3)); /* 1024*1024 */
		} else if (kB>=1000) {
			if (type==2) {
				declen=0;
			}
			sprintf(buffer, "%'*.*f %*s", len, declen, kB/(float)1024, pad, getunit(2));
		} else {
			sprintf(buffer, "%'*"PRIu64" %*s", len, kB, pad, getunit(1));
		}
#else
		/* try to figure out what unit to use */
		if (kB>=1048576000) { /* 1024*1024*1000 - value >=1000 GiB -> show in TiB */
			sprintf(buffer, "%*.*f %*s", len, declen, kB/(float)1073741824, pad, getunit(4)); /* 1024*1024*1024 */
		} else if (kB>=1024000) { /* 1024*1000 - value >=1000 MiB -> show in GiB */
			sprintf(buffer, "%*.*f %*s", len, declen, kB/(float)1048576, pad, getunit(3)); /* 1024*1024 */
		} else if (kB>=1000) {
			if (type==2) {
				declen=0;
			}
			sprintf(buffer, "%*.*f %*s", len, declen, kB/(float)1024, pad, getunit(2));
		} else {
			sprintf(buffer, "%'*"PRIu64" %*s", len, kB, pad, getunit(1));
		}
#endif
	}
	
	return buffer;
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

char *getunit(int index)
{
	static char *unit[] = { "na", "KiB", "MiB", "GiB", "TiB",
                                   "KB",  "MB",  "GB",  "TB",
                                   "kB",  "MB",  "GB",  "TB" };

	if (index>UNITCOUNT) {
		return unit[0];
	} else {
		return unit[(cfg.unit*UNITCOUNT)+index];
	}
}

char *getbunit(int unit, int index)
{
	static char *bunit[] = { "na", "Kibit", "Mibit", "Gibit", "Tibit",
                                    "Kbit",  "Mbit",  "Gbit",  "Tbit",
                                    "kbit",  "Mbit",  "Gbit",  "Tbit" };

	if (index>UNITCOUNT) {
		return bunit[0];
	} else {
		return bunit[(unit*UNITCOUNT)+index];
	}
}
