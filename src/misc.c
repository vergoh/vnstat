#include "vnstat.h"
#include "misc.h"


void kerneltest(void)
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
	} else if ((bmax-bmin)>cfg.bvar) {
		printf("The current kernel has a boot time variation greater than assumed\n");
		printf("in the vnStat config. That it likely to cause errors in results.\n");
		printf("Set \"BootVariation\" to something greater than \"%d\" and run this\n", (bmax-bmin));
		printf("test again.\n\n");
	} else if ((bmax-bmin)==0) {
		printf("The current kernel doesn't seem to suffer from boot time variation problems.\n\n");
	} else {
		printf("The current kernel is ok.\n\n");
	}

}

int spacecheck(char *path)
{
	struct statfs buf;
	uint64_t free;

	if (statfs(path, &buf)) {
		perror("Free diskspace check");
		exit(0);
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
	char temp[64];

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
