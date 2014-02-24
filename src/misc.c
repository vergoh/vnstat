#include "vnstat.h"
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
	struct statfs buf;
	uint64_t free;

	if (statfs(path, &buf)) {
		perror("Free diskspace check");
		exit(1);
	}

	free=(buf.f_bavail/(float)1024)*buf.f_bsize;

	if (debug) {
		printf("bsize %d\n", buf.f_bsize);
		printf("blocks %lu\n", buf.f_blocks);
		printf("bfree %lu\n", buf.f_bfree);
		printf("bavail %lu\n", buf.f_bavail);
		printf("ffree %lu\n", buf.f_ffree);
		
		printf("%Lu free space left\n", free);
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

void intr(int sig)
{
	intsignal=1;
	if (debug)
		printf("Got signal: %d\n", sig);	
}

int getbtime(void)
{
	FILE *fp;
	int check, result=0;
	char temp[64], statline[128];

	if ((fp=fopen("/proc/stat","r"))==NULL) {
		printf("Error:\nUnable to read /proc/stat.\n");
		exit(1);
	}
	
	check=0;
	while (fgets(statline,128,fp)!=NULL) {
		sscanf(statline,"%s",temp);
		if (strcmp(temp,"btime")==0) {
			if (debug)
				printf("\n%s\n",statline);
			check=1;
			break;
		}
	}
	
	if (check==0) {
		printf("Error:\nbtime missing from /proc/stat.\n");
		exit(1);
	}
	
	result = strtoul(statline+6, (char **)NULL, 0);
	fclose(fp);

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

char *getvalue(uint64_t mb, uint64_t kb, int len)
{
	static char buffer[64];
	int declen=2;
	uint64_t kB;

	/* don't show decimals .00 if len was negative (set that way for estimates) */
	if (len<0) {
		declen=0;
		len=abs(len);
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


	if ( (declen==0) && (kB==0) ){
		sprintf(buffer, "%*s   ", len, "--");
	} else {
		/* try to figure out what unit to use */
		if (kB>=(1024*1024*1000)) {
			sprintf(buffer, "%'*.2f TB", len, kB/(float)1024/(float)1024/(float)1024);
		} else if (kB>=(1024*1000)) {
			sprintf(buffer, "%'*.2f GB", len, kB/(float)1024/(float)1024);
		} else if (kB>=1000) {
			sprintf(buffer, "%'*.*f MB", len, declen, kB/(float)1024);
		} else {
			sprintf(buffer, "%'*Lu kB", len, kB);
		}
	}
	
	return buffer;
}


void showbar(uint64_t rx, int rxk, uint64_t tx, int txk, uint64_t max, int len)
{
	int i, l;
	
	if (rxk>=1024) {
		rx+=rxk/1024;
		rxk-=(rxk/1024)*1024;
	}

	if (txk>=1024) {
		tx+=txk/1024;
		txk-=(txk/1024)*1024;
	}

	rx=(rx*1024)+rxk;
	tx=(tx*1024)+txk;
	
	if ((rx+tx)!=max) {
		len=((rx+tx)/(float)max)*len;
	}


	if (len!=0) {
		printf("   ");

		if (tx>rx) {
			l=rint((rx/(float)(rx+tx)*len));

			for (i=0;i<l;i++) {
				printf("%c", cfg.rxchar[0]);
			}
			for (i=0;i<(len-l);i++) {
				printf("%c", cfg.txchar[0]);
			}
		} else {
			l=rint((tx/(float)(rx+tx)*len));
			
			for (i=0;i<(len-l);i++) {
				printf("%c", cfg.rxchar[0]);
			}
			for (i=0;i<l;i++) {
				printf("%c", cfg.txchar[0]);
			}		
		}
		
	}
	
}
