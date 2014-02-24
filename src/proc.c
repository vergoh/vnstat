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
		strncpy(inface,"eth0", 32);
	} else {
		strncpy(inface,iface, 32);
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
	uint64_t rx, tx, rxchange=0, txchange=0, btime;   /* rxchange = rx change in MB */
	uint64_t krxchange=0, ktxchange=0;                /* krxchange = rx change in kB */
	time_t current;
	struct tm *d;
	int day, month, year, hour, min, shift;
	int rxkchange=0, txkchange=0;			/* changes in the kB counters */

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
			krxchange=(rx-data.currx)/1024;
			rxkchange=((rx-data.currx)/1024)%1024;
			if (debug)
				printf("rx: %Lu - %Lu = %Lu\n",rx,data.currx,rx-data.currx);
		} else {
			rxchange=(FPOINT-data.currx+rx)/1024/1024;
			krxchange=(FPOINT-data.currx+rx)/1024;
			rxkchange=((FPOINT-data.currx+rx)/1024)%1024;
			if (debug)
				printf("rx: %Lu - %Lu + %Lu = %Lu\n",FPOINT,data.currx,rx,FPOINT-data.currx+rx);
		}
	}
	
	data.currx=rx;	

	addtraffic(&data.totalrx, &data.totalrxk, rxchange, rxkchange);
	

	/* get tx from procline, ugly hack */
	sscanf(procline+7,"%s %s %s %s %s %s %s %s %s",temp,temp,temp,temp,temp,temp,temp,temp,temp);

#ifdef BLIMIT
	tx=strtoull(temp, (char **)NULL, 0);
#else
	tx=strtoul(temp, (char **)NULL, 0);
#endif

	if (newdb!=1) {
		if (data.curtx<=tx) {
			txchange=(tx-data.curtx)/1024/1024;
			ktxchange=(tx-data.curtx)/1024;
			txkchange=((tx-data.curtx)/1024)%1024;
			if (debug)
				printf("tx: %Lu - %Lu = %Lu\n",tx,data.curtx,tx-data.curtx);
		} else {
			txchange=(FPOINT-data.curtx+tx)/1024/1024;
			ktxchange=(FPOINT-data.curtx+tx)/1024;
			txkchange=((FPOINT-data.curtx+tx)/1024)%1024;
			if (debug)
				printf("tx: %Lu - %Lu + %Lu = %Lu\n",FPOINT,data.curtx,tx,FPOINT-data.curtx+tx);
		}
	}

	data.curtx=tx;
	
	addtraffic(&data.totaltx, &data.totaltxk, txchange, txkchange);

	/* update days and months */
	addtraffic(&data.day[0].rx, &data.day[0].rxk, rxchange, rxkchange);
	addtraffic(&data.day[0].tx, &data.day[0].txk, txchange, txkchange);
	addtraffic(&data.month[0].rx, &data.month[0].rxk, rxchange, rxkchange);
	addtraffic(&data.month[0].tx, &data.month[0].txk, txchange, txkchange);	

	/* fill some variables from current date & time */
	d=localtime(&current);
	day=d->tm_mday;
	month=d->tm_mon;
	year=d->tm_year;
	hour=d->tm_hour;
	min=d->tm_min;
	
	/* shift traffic to previous hour when update happens at X:00 */
	if (min==0) {
		shift=hour;
		hour--;
		if (hour<0)
			hour+=24;     /* hour can't be -1 :) */
	} else {
		shift=hour;
	}
	
	/* clean and update hourly */
	cleanhours();
	data.hour[shift].date=current;   /* avoid shifting timestamp */
	data.hour[hour].rx+=krxchange;
	data.hour[hour].tx+=ktxchange;
	
	/* rotate days in database if needed */
	d=localtime(&data.day[0].date);
	if ((d->tm_mday!=day) || (d->tm_mon!=month) || (d->tm_year!=year))
		rotatedays();

	/* rotate months in database if needed */
	d=localtime(&data.month[0].month);
	if ((d->tm_mon!=month) && (day>=atoi(MONTHROTATE)))
		rotatemonths();

}

void trafficmeter(char iface[32], int sampletime)
{
	/* received bytes packets errs drop fifo frame compressed multicast */
	/* transmitted bytes packets errs drop fifo colls carrier compressed */
	uint64_t p1[16], p2[16];
	int i, j, len;
	char temp[64], buffer[256];

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
#ifdef BLIMIT
			p1[j]=strtoull(temp, (char **)NULL, 0);
#else
			p1[j]=strtoul(temp, (char **)NULL, 0);
#endif
			if (debug)
				printf("%8d '%s' -> '%Lu'\n",j,temp,p1[j]);
			j++;
		}
	}

	/* wait sampletime and print some nice dots so that the user thinks
	something is done :) */
	sprintf(buffer,"Sampling %s (%d seconds average)",iface,sampletime);
	printf("%s",buffer);
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
	
	len=strlen(buffer)+3;
	
	for (i=0;i<len;i++)
		printf("\b \b");
	
	/* read those value again... */
	j=0;
	readproc(iface);
	for (i=7;i<strlen(procline);i++) {
		if (procline[i]!=' ') {
			sscanf(procline+i,"%s",temp);
			i+=strlen(temp);
#ifdef BLIMIT
			p2[j]=strtoull(temp, (char **)NULL, 0);
#else
			p2[j]=strtoul(temp, (char **)NULL, 0);
#endif
			if (debug)
				printf("%8d '%s' -> '%Lu'\n",j,temp,p2[j]);
			j++;
		}
	}

	/* show the difference in a readable form */
	printf("%Lu packets sampled in %d seconds\n", (p2[1]-p1[1])+(p2[9]-p1[9]), sampletime);
	printf("Traffic average for %s\n", iface);
	printf("\n      rx     %10.2f kB/s          %5Lu packets/s\n", (p2[0]-p1[0])/(float)sampletime/(float)1024, (uint64_t)((p2[1]-p1[1])/sampletime));
	printf("      tx     %10.2f kB/s          %5Lu packets/s\n\n", (p2[8]-p1[8])/(float)sampletime/(float)1024, (uint64_t)((p2[9]-p1[9])/sampletime));
}

void addtraffic(uint64_t *destmb, int *destkb, uint64_t srcmb, int srckb)
{
	*destmb=*destmb+srcmb;
	*destkb=*destkb+srckb;
	
	while (*destkb>=1024) {
		*destmb=*destmb+1;
		*destkb=*destkb-1024;
	}
}
