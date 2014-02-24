#include "vnstat.h"
#include "proc.h"
#include "db.h"

void readproc(char iface[32])
{
	FILE *fp;
	char temp[64], inface[32];
	int check;
	
	if ((fp=fopen("/proc/net/dev","r"))==NULL) {
		printf("Error:\nUnable to read /proc/net/dev.\n");
		exit(1);
	}

	if (strcmp(iface,"default")==0) {
		strcpy(inface,"eth0");
	} else {
		strcpy(inface,iface);
	}

	check=0;
	while (fgets(procline,512,fp)!=NULL) {
		sscanf(procline,"%s",temp);
		if (strncmp(inface,temp,strlen(inface))==0) {
			if (debug)
				printf("\n%s\n",procline);
			check=1;
			break;
		}
	}
	fclose(fp);
	
	if (check==0) {
		printf("Requested interface \"%s\" not found.\n",inface);
		printf("Exiting...\n");
		exit(1);
	}

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
	
	fclose(fp);
	
	if (check==0) {
		printf("Error:\nbtime missing from /proc/stat.\n");
		exit(1);
	}
}

void parseproc(int newdb)
{
	char temp[64];
	uint64_t rx, tx, rxchange=0, txchange=0, btime;
	time_t current;
	struct tm *d;
	int day, month, year;

	btime=strtoul(statline+6, (char **)NULL, 0);

	/* btime in /proc/stat seems to vary ±1 second so we use btime-BVAR just to be safe */
	/* the variation is also slightly different between various kernels... */
	if (data.btime<btime-atoi(BVAR)) {
		data.currx=0;
		data.curtx=0;
		if (debug)
			printf("System has been booted.\n");
	}

	data.btime=btime;

	current=time(NULL);

	/* get rx from procline, easy since it's always procline+7 */

#ifdef BLIMIT
	rx=strtoull(procline+7, (char **)NULL, 0);
#else
	rx=strtoul(procline+7, (char **)NULL, 0);
#endif

	if (newdb!=1) {
		if (data.currx<=rx) {
			rxchange=(rx-data.currx)/1024/1024;
			data.totalrxk+=((rx-data.currx)/1024)%1024;
			if (debug)
				printf("rx: %Lu - %Lu = %Lu\nrxk: %d\n",rx,data.currx,rx-data.currx,data.totalrxk);
		} else {
			rxchange=(FPOINT-data.currx+rx)/1024/1024;
			data.totalrxk+=((FPOINT-data.currx+rx)/1024)%1024;
			if (debug)
				printf("rx: %Lu - %Lu + %Lu = %Lu\nrxk: %d\n",FPOINT,data.currx,rx,FPOINT-data.currx+rx,data.totalrxk);
		}
	}
	
	if (data.totalrxk>=1024) {
		rxchange++;
		data.totalrxk-=1024;
	}
		
	data.currx=rx;
	data.totalrx+=rxchange;

	/* get tx from procline, ugly code but it works */
	sscanf(procline+7,"%s %s %s %s %s %s %s %s %s",temp,temp,temp,temp,temp,temp,temp,temp,temp);

#ifdef BLIMIT
	tx=strtoull(temp, (char **)NULL, 0);
#else
	tx=strtoul(temp, (char **)NULL, 0);
#endif

	if (newdb!=1) {
		if (data.curtx<=tx) {
			txchange=(tx-data.curtx)/1024/1024;
			data.totaltxk+=((tx-data.curtx)/1024)%1024;
			if (debug)
				printf("tx: %Lu - %Lu = %Lu\ntxk: %d\n",tx,data.curtx,tx-data.curtx,data.totaltxk);
		} else {
			txchange=(FPOINT-data.curtx+tx)/1024/1024;
			data.totaltxk+=((FPOINT-data.curtx+tx)/1024)%1024;
			if (debug)
				printf("tx: %Lu - %Lu + %Lu = %Lu\ntxk: %d\n",FPOINT,data.curtx,tx,FPOINT-data.curtx+tx,data.totaltxk);
		}
	}
	
	if (data.totaltxk>=1024) {
		txchange++;
		data.totaltxk-=1024;
	}
	
	data.curtx=tx;
	data.totaltx+=txchange;
	
	data.day[0].rx+=rxchange;
	data.day[0].tx+=txchange;
	data.month[0].rx+=rxchange;
	data.month[0].tx+=txchange;

	d=localtime(&current);
	day=d->tm_mday;
	month=d->tm_mon;
	year=d->tm_year;
	
	d=localtime(&data.day[0].date);
	if ((d->tm_mday!=day) || (d->tm_mon!=month) || (d->tm_year!=year))
		rotatedays();

	d=localtime(&data.month[0].month);
	if ((d->tm_mon!=month) && (day==atoi(MONTHROTATE)))
		rotatemonths();

}

void trafficmeter(char iface[32], int sampletime)
{
	/* received bytes packets errs drop fifo frame compressed multicast */
	/* transmitted bytes packets errs drop fifo colls carrier compressed */
	unsigned long int p1[16], p2[16];
	int i, j;
	char temp[64];

	/* less than 5 seconds if probably too inaccurate */
	if (sampletime<5) {
		printf("Error:\nTime for sampling too short.\n");
		exit(1);
	}

	/* read /proc/net/dev and get those values to the first list */
	j=0;
	readproc(iface);
	for (i=7;i<strlen(procline);i++) {
		if (procline[i]!=' ') {
			sscanf(procline+i,"%s",temp);
			i+=strlen(temp);
			if (debug)
				printf("%8d '%s'\n",j,temp);
#ifdef BLIMIT
			p1[j]=strtoull(temp, (char **)NULL, 0);
#else
			p1[j]=strtoul(temp, (char **)NULL, 0);
#endif
			j++;
		}
	}

	/* wait sampletime and print some nice dots so that the user thinks
	something is done :) */
	printf("Sampling %s (%d seconds average)",iface,sampletime);
	fflush(stdout);
	sleep(sampletime/3);
	printf(".");
	fflush(stdout);
	sleep(sampletime/3);
	printf(".");
	fflush(stdout);
	sleep(sampletime/3);
	printf(".");
	fflush(stdout);
	if ((sampletime/3)*3!=sampletime) {
		sleep(sampletime-((sampletime/3)*3));
	}	
	printf("\n");
		
	/* read those value again... */
	j=0;
	readproc(iface);
	for (i=7;i<strlen(procline);i++) {
		if (procline[i]!=' ') {
			sscanf(procline+i,"%s",temp);
			i+=strlen(temp);
			if (debug)
				printf("%8d '%s'\n",j,temp);
#ifdef BLIMIT
			p2[j]=strtoull(temp, (char **)NULL, 0);
#else
			p2[j]=strtoul(temp, (char **)NULL, 0);
#endif
			j++;
		}
	}

	/* show the difference in a readable form */
	printf("\n      rx     %10.2f kB/s          %5lu packets/s\n", (p2[0]-p1[0])/(float)sampletime/(float)1024, (p2[1]-p1[1])/sampletime);
	printf("      tx     %10.2f kB/s          %5lu packets/s\n\n", (p2[8]-p1[8])/(float)sampletime/(float)1024, (p2[9]-p1[9])/sampletime);

}
