#include "vnstat.h"
#include "misc.h"

void kerneltest(void)
{
	FILE *fp;
	int i=0, check, b[5];
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
	
	b[i]=strtoul(statline+6, (char **)NULL, 0);
	
	printf("This test will take about 20 seconds.\n");
	printf("Testing kernel.");
	fflush(stdout);
	
	for (i=1;i<5;i++) {
		sleep(5);
		rewind(fp);
		printf(".");
		fflush(stdout);
		
		while (fgets(statline,128,fp)!=NULL) {
			sscanf(statline,"%s",temp);
			if (strcmp(temp,"btime")==0) {
				if (debug)
					printf("\n%s\n",statline);
				check=1;
				break;
			}
		}
		b[i]=strtoul(statline+6, (char **)NULL, 0);
	}
	
	fclose(fp);
	printf(" done\n\n");
	
	if ((b[0] < b[1]) && (b[1] < b[2]) && (b[2] < b[3]) && (b[3] < b[4])) {
		printf("The current kernel has a broken btime information and vnStat wont work right.\n");
		printf("Upgrading the kernel is likely to solve this problem.\n");
	} else {
		printf("The current kernel is ok.\n");
	}
}

int spacecheck(char *path)
{
	struct statfs buf;
	int free;

	if (statfs(path, &buf)) {
		perror("Free diskspace check");
		exit(0);
	}

	free=(buf.f_bavail/(float)buf.f_blocks)*100;

	if (debug)
		printf("%d%% free space left\n", free);

	if (free<=1)
		return 0;
	else
		return 1;
}
