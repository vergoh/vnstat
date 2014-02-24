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

	/* btime in /proc/stat seems to vary ±1 second so we use btime-5 just to be safe */
	if (data.btime<btime-5) {
		data.currx=0;
		data.curtx=0;
		if (debug)
			printf("System has been booted.\n");
	}

	data.btime=btime;

	current=time(NULL);

	/* get rx from procline, easy since it's always procline+1 */
	rx=strtoul(procline+7, (char **)NULL, 0);

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
	tx=strtoul(temp, (char **)NULL, 0);

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
