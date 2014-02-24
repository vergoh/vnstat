#include "vnstat.h"
#include "ifinfo.h"
#include "misc.h"
#include "db.h"
#include "cfg.h"

void getifinfo(char iface[32])
{
	char inface[32];

	ifinfo.filled = 0;

	if (strcmp(iface, "default")==0) {
		strncpy(inface, cfg.iface, 32);
	} else {
		strncpy(inface, iface, 32);
	}

	/* try getting interface info from /proc */
	if (readproc(inface)!=1) {
		if (debug)
			printf("Failed to use /proc/net/dev as source.\n");

		/* try getting interface info from /sys */
		if (readsysclassnet(inface)!=1) {
			printf("Error:\nUnable to get interface statistics.\n");
			exit(1);
		}
	}
}

int readproc(char iface[32])
{
	FILE *fp;
	char temp[4][64], procline[512], *proclineptr;
	int check;
	
	if ((fp=fopen("/proc/net/dev", "r"))==NULL) {
		if (debug)
			printf("Error: Unable to read /proc/net/dev.\n");
		return 0;
	}

	check = 0;
	while (fgets(procline, 512, fp)!=NULL) {
		sscanf(procline, "%s", temp[0]);
		if (strncmp(iface, temp[0], strlen(iface))==0) {
			if (debug)
				printf("\n%s\n", procline);
			check = 1;
			break;
		}
	}
	fclose(fp);
	
	if (check==0) {
		if (debug)
			printf("Requested interface \"%s\" not found.\n", iface);
		return 0;
	} else {

		strncpy(ifinfo.name, iface, 32);

		/* get rx and tx from procline */
		proclineptr = strchr(procline, ':');
		sscanf(proclineptr+1, "%s %s %*s %*s %*s %*s %*s %*s %s %s", temp[0], temp[1], temp[2], temp[3]);

		ifinfo.rx = strtoull(temp[0], (char **)NULL, 0);
		ifinfo.tx = strtoull(temp[2], (char **)NULL, 0);
		ifinfo.rxp = strtoull(temp[1], (char **)NULL, 0);
		ifinfo.txp = strtoull(temp[3], (char **)NULL, 0);

		ifinfo.filled = 1;
	}

	return 1;
}

int readsysclassnet(char iface[32])
{
	FILE *fp;
	char path[64], file[76], buffer[64];

	strncpy(ifinfo.name, iface, 32);

	snprintf(path, 64, "/sys/class/net/%s/statistics", iface);

	if (debug)
		printf("path: %s\n", path);

	/* rx bytes */
	snprintf(file, 76, "%s/rx_bytes", path);
	if ((fp=fopen(file, "r"))==NULL) {
		if (debug)
			printf("Unable to read: %s\n", file);
		return 0;
	} else {
		if (fgets(buffer, 64, fp)!=NULL) {
			ifinfo.rx = strtoull(buffer, (char **)NULL, 0);
		} else {
			return 0;
		}
	}
	fclose(fp);

	/* tx bytes */
	snprintf(file, 76, "%s/tx_bytes", path);
	if ((fp=fopen(file, "r"))==NULL) {
		if (debug)
			printf("Unable to read: %s\n", file);
		return 0;
	} else {
		if (fgets(buffer, 64, fp)!=NULL) {
			ifinfo.tx = strtoull(buffer, (char **)NULL, 0);
		} else {
			return 0;
		}
	}
	fclose(fp);

	/* rx packets */
	snprintf(file, 76, "%s/rx_packets", path);
	if ((fp=fopen(file, "r"))==NULL) {
		if (debug)
			printf("Unable to read: %s\n", file);
		return 0;
	} else {
		if (fgets(buffer, 64, fp)!=NULL) {
			ifinfo.rxp = strtoull(buffer, (char **)NULL, 0);
		} else {
			return 0;
		}
	}
	fclose(fp);

	/* tx packets */
	snprintf(file, 76, "%s/tx_packets", path);
	if ((fp=fopen(file, "r"))==NULL) {
		if (debug)
			printf("Unable to read: %s\n", file);
		return 0;
	} else {
		if (fgets(buffer, 64, fp)!=NULL) {
			ifinfo.txp = strtoull(buffer, (char **)NULL, 0);
		} else {
			return 0;
		}
	}
	fclose(fp);

	ifinfo.filled = 1;

	return 1;
}

void parseifinfo(int newdb)
{
	uint64_t rxchange=0, txchange=0, btime;   /* rxchange = rx change in MB */
	uint64_t krxchange=0, ktxchange=0, maxtransfer;   /* krxchange = rx change in kB */
	time_t current, interval;
	struct tm *d;
	int day, month, year, hour, min, shift, maxbw;
	int rxkchange=0, txkchange=0;			          /* changes in the kB counters */

	current=time(NULL);
	interval=current-data.lastupdated;
	btime=getbtime();

	/* count traffic only if previous update wasn't too long ago */
	if ( interval < (60*MAXUPDATEINTERVAL) ) {

		/* btime in /proc/stat seems to vary ±1 second so we use btime-BVAR just to be safe */
		/* the variation is also slightly different between various kernels... */
		if (data.btime < (btime-cfg.bvar)) {
			data.currx=0;
			data.curtx=0;
			if (debug)
				printf("System has been booted.\n");
		}

		/* process rx & tx */
		if (newdb!=1) {
			rxchange = countercalc(data.currx, ifinfo.rx)/1024/1024;
			krxchange = countercalc(data.currx, ifinfo.rx)/1024;
			rxkchange = (countercalc(data.currx, ifinfo.rx)/1024)%1024;

			txchange = countercalc(data.curtx, ifinfo.tx)/1024/1024;
			ktxchange = countercalc(data.curtx, ifinfo.tx)/1024;
			txkchange = (countercalc(data.curtx, ifinfo.tx)/1024)%1024;
		}

		/* get maximum bandwidth */
		maxbw = ibwget(data.interface);

		if (maxbw > 0) {
		
			/* calculate maximum possible transfer since last update based on set maximum rate */
			/* and add 10% in order to be on the safe side */
			maxtransfer = ceil((maxbw/(float)8)*interval*(float)1.1);

			if (debug)
				printf("interval: %Lu  maxbw: %d  maxrate: %Lu  rxc: %Lu  txc: %Lu\n", (uint64_t)interval, maxbw, maxtransfer, rxchange, txchange); 

			/* sync counters if traffic is greater than set maximum */
			if ( (rxchange > maxtransfer) || (txchange > maxtransfer) ) {
				rxchange=krxchange=rxkchange=txchange=ktxchange=txkchange=0;
				if (debug)
					printf("Traffic is greater than set maximum, counters synced.\n");
			}
		}

	} else {
		if (debug)
			printf("Too much time passed since previous update, syncing. (%Lu < %d)\n", (uint64_t)interval, 60*MAXUPDATEINTERVAL);
	}


	/* keep btime updated in case it drifts slowly */
	data.btime = btime;

	data.currx = ifinfo.rx;
	data.curtx = ifinfo.tx;
	addtraffic(&data.totalrx, &data.totalrxk, rxchange, rxkchange);
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
	if ((d->tm_mon!=month) && (day>=MONTHROTATE))
		rotatemonths();

}

void trafficmeter(char iface[32], int sampletime)
{
	/* received bytes packets errs drop fifo frame compressed multicast */
	/* transmitted bytes packets errs drop fifo colls carrier compressed */
	uint64_t rx, tx, rxp, txp;
	IFINFO firstinfo;
	int i, len;
	char buffer[256];

	/* less than 2 seconds doesn't produce good results */
	if (sampletime<2) {
		printf("Error:\nTime for sampling too short.\n");
		exit(1);
	}

	/* read interface info and get values to the first list */
	getifinfo(iface);
	firstinfo.rx = ifinfo.rx;
	firstinfo.tx = ifinfo.tx;
	firstinfo.rxp = ifinfo.rxp;
	firstinfo.txp = ifinfo.txp;

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
	
	for (i=0;i<len;i++) {
		printf("\b \b");
	}
	
	/* read those values again... */
	getifinfo(iface);

	/* calculate traffic and packets seen between updates */
	rx = countercalc(firstinfo.rx, ifinfo.rx);
	tx = countercalc(firstinfo.tx, ifinfo.tx);
	rxp = countercalc(firstinfo.rxp, ifinfo.rxp);
	txp = countercalc(firstinfo.txp, ifinfo.txp);

	/* show the difference in a readable format */
	printf("%Lu packets sampled in %d seconds\n", rxp+txp, sampletime);
	printf("Traffic average for %s\n", iface);
	printf("\n      rx     %10.2f kB/s          %5Lu packets/s\n", rx/(float)sampletime/(float)1024, (uint64_t)(rxp/sampletime));
	printf("      tx     %10.2f kB/s          %5Lu packets/s\n\n", tx/(float)sampletime/(float)1024, (uint64_t)(txp/sampletime));

}

void livetrafficmeter(char iface[32])
{
	/* received bytes packets errs drop fifo frame compressed multicast */
	/* transmitted bytes packets errs drop fifo colls carrier compressed */
	uint64_t rx, tx, rxp, txp, rxpc, txpc, timespent;
	uint64_t rxtotal, txtotal, rxptotal, txptotal;
	uint64_t rxpmin, txpmin, rxpmax, txpmax;
	float rxmin, txmin, rxmax, txmax, rxc, txc;
	int i, len;
	char buffer[256];
	IFINFO previnfo;

	printf("Monitoring %s...    (press CTRL-C to stop)\n\n", iface);
	printf("   getting traffic...");
	len=21;
	fflush(stdout);

	/* enable signal trap */
	if (signal(SIGINT, intr) == SIG_ERR) {
		perror("signal");
		exit(1);
	}

	/* set some defaults */
	rxtotal=txtotal=rxptotal=txptotal=rxpmax=txpmax=0;
	rxpmin=txpmin=-1;
	rxmax=txmax=0.0;
	rxmin=txmin=-1.0;
	
	timespent = (uint64_t)time(NULL);

	/* read /proc/net/dev and get values to the first list */
	getifinfo(iface);

	/* loop until user gets bored */
	while (intsignal==0) {

		/* wait 2 seconds for more traffic */
		sleep(LIVETIME);

		/* break loop without calculations because sleep was probably interrupted */
		if (intsignal) {
			break;
		}

		/* use values from previous loop if this isn't the first time
		if (len!=21) {
			for (i=0;i<16;i++) {
				p1[i]=p2[i];
			}
		} */

		previnfo.rx = ifinfo.rx;
		previnfo.tx = ifinfo.tx;
		previnfo.rxp = ifinfo.rxp;
		previnfo.txp = ifinfo.txp;

		/* read those values again... */
		getifinfo(iface);

		/* calculate traffic and packets seen between updates */
		rx = countercalc(previnfo.rx, ifinfo.rx);
		tx = countercalc(previnfo.tx, ifinfo.tx);
		rxp = countercalc(previnfo.rxp, ifinfo.rxp);
		txp = countercalc(previnfo.txp, ifinfo.txp);

		/* update totals */
		rxtotal += rx;
		txtotal += tx;
		rxptotal += rxp;
		txptotal += txp;

		rxc = rx/(float)LIVETIME/(float)1024;
		txc = tx/(float)LIVETIME/(float)1024;
		rxpc = rxp/LIVETIME;
		txpc = txp/LIVETIME;		

		/* update min & max */
		if ((rxmin==-1.0) || (rxmin>rxc)) {
			rxmin = rxc;
		}
		if ((txmin==-1.0) || (txmin>txc)) {
			txmin = txc;
		}
		if (rxmax<rxc) {
			rxmax = rxc;
		}
		if (txmax<txc) {
			txmax = txc;
		}
		
		if ((rxpmin==-1) || (rxpmin>rxpc)) {
			rxpmin = rxpc;
		}
		if ((txpmin==-1) || (txpmin>txpc)) {
			txpmin = txpc;
		}
		if (rxpmax<rxpc) {
			rxpmax = rxpc;
		}
		if (txpmax<txpc) {
			txpmax = txpc;
		}

		/* show the difference in a readable format */
		snprintf(buffer, 256, "   rx: %10.2f kB/s %5Lu p/s            tx: %10.2f kB/s %5Lu p/s", rxc, (uint64_t)rxpc, txc, (uint64_t)txpc);
		
		if (len>1) {
			if (debug) {
				printf("\nlinelen: %d\n", len);
			} else {
				for (i=0;i<len;i++) {
					printf("\b \b");
				}
				fflush(stdout);
			}
		}
		printf("%s", buffer);
		fflush(stdout);
		len=strlen(buffer);
	
	}

	timespent = (uint64_t)time(NULL) - timespent;

	printf("\n\n");

	/* print some statistics if enough time did pass */
	if (timespent>10) {

		printf("\n %s  /  traffic statistics\n\n", iface);

		printf("                             rx       |       tx\n");
		printf("--------------------------------------+----------------------------------------\n");
		printf("  bytes                %s", getvalue(0, rxtotal/1024, 10));
		printf("  | %s", getvalue(0, txtotal/1024, 10));
		printf("\n");
		printf("--------------------------------------+----------------------------------------\n");
		printf("          max          ");
		showspeed(rxmax, 8);
		printf("  | ");
		showspeed(txmax, 8);
		printf("\n");
		printf("      average          ");
		showspeed(rxtotal/(float)timespent/(float)1024, 8);
		printf("  | ");
		showspeed(txtotal/(float)timespent/(float)1024, 8);
		printf("\n");
		printf("          min          ");
		showspeed(rxmin, 8);
		printf("  | ");
		showspeed(txmin, 8);
		printf("\n");
		printf("--------------------------------------+----------------------------------------\n");
		printf("  packets               %12Lu  |  %12Lu\n", rxptotal, txptotal);
		printf("--------------------------------------+----------------------------------------\n");
		printf("          max          %9Lu p/s  | %9Lu p/s\n", rxpmax, txpmax);
		printf("      average          %9Lu p/s  | %9Lu p/s\n", rxptotal/timespent, txptotal/timespent);
		printf("          min          %9Lu p/s  | %9Lu p/s\n", rxpmin, txpmin);
		printf("--------------------------------------+----------------------------------------\n");
		
		if (timespent<=60) {
			printf("  time             %9Lu seconds\n", timespent);
		} else {
			printf("  time               %7.2f minutes\n", timespent/(float)60);
		}

		printf("\n");
	}
	
}

uint64_t countercalc(uint64_t a, uint64_t b)
{
	/* no flip */
	if (b>=a) {
		if (debug)
			printf("cc: %Lu - %Lu = %Lu\n", b, a, b-a);
		return b-a;

	/* flip exists */
	} else {
		/* original counter is 64bit */
		if (a>FP32) {
			if (debug)
				printf("cc64: uint64 - %Lu + %Lu = %Lu\n", a, b, FP64-a+b);
			return FP64-a+b;

		/* original counter is 32bit */
		} else {
			if (debug)
				printf("cc32: uint32 - %Lu + %Lu = %Lu\n", a, b, FP32-a+b);
			return FP32-a+b;
		}
	}
}

void showspeed(float xfer, int len)
{
	if (xfer>=1000) {
		printf("%'*.2f MB/s", len, xfer/(float)1024);
	} else {
		printf("%'*.2f kB/s", len, xfer);
	}
}
