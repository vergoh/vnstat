#include "vnstat.h"
#include "misc.h"

void kerneltest(void)
{
	FILE *fp;
	int i=0, check, b1, b2;
	char temp[64];

	if ((fp=fopen("/proc/stat","r"))==NULL) {
		printf("Error:\nUnable to read /proc/net/dev.\n");
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
	
	b1=strtoul(statline+6, (char **)NULL, 0);
	fclose(fp);
	
	printf("This test will take about 20 seconds.\n");
	printf("Testing kernel.");
	fflush(stdout);
	
	for (i=1;i<5;i++) {
		sleep(5);
		printf(".");
		fflush(stdout);
	}
	
	if ((fp=fopen("/proc/stat","r"))==NULL) {
		printf("Error:\nUnable to read /proc/net/dev.\n");
		exit(1);
	}
	
	while (fgets(statline,128,fp)!=NULL) {
		sscanf(statline,"%s",temp);
		if (strcmp(temp,"btime")==0) {
			if (debug)
				printf("\n%s\n",statline);
			break;
		}
	}
	b2=strtoul(statline+6, (char **)NULL, 0);
	fclose(fp);
		
	printf(" done\n\n");
	
	if (b2>b1+5) {
		printf("The current kernel has a broken btime information and vnStat wont work right.\n");
		printf("Upgrading the kernel is likely to solve this problem.\n");
		exit(1);
	} else {
		printf("The current kernel is ok.\n");
		exit(0);
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
		printf("bzise %d\n", buf.f_bsize);
		printf("blocks %lu\n", buf.f_blocks);
		printf("bfree %lu\n", buf.f_bfree);
		printf("bavail %lu\n", buf.f_bavail);
		printf("ffree %lu\n", buf.f_ffree);
		
		printf("%Lu free space left\n", free);
	}	

	/* the database is less than 3kB but let's require */
	/* 1MB to be on the safe side, anyway, the filesystem should */
	/* always have more free space than that */
	if (free<=1024)
		return 0;
	else
		return 1;
}
