#include "common.h"
#include "ifinfo.h"
#include "misc.h"
#include "traffic.h"

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
		printf("Error: Time for sampling too short.\n");
		exit(1);
	}

	/* read interface info and get values to the first list */
	if (!getifinfo(iface)) {
		printf("Error: Interface \"%s\" not available, exiting.\n", iface);
		exit(1);
	}
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
	if (!getifinfo(iface)) {
		printf("Error: Interface \"%s\" not available, exiting.\n", iface);
		exit(1);
	}

	/* calculate traffic and packets seen between updates */
	rx = countercalc(firstinfo.rx, ifinfo.rx);
	tx = countercalc(firstinfo.tx, ifinfo.tx);
	rxp = countercalc(firstinfo.rxp, ifinfo.rxp);
	txp = countercalc(firstinfo.txp, ifinfo.txp);

	/* show the difference in a readable format */
	printf("%"PRIu64" packets sampled in %d seconds\n", rxp+txp, sampletime);
	printf("Traffic average for %s\n", iface);
	printf("\n      rx     %10.2f %s/s         %5"PRIu64" packets/s\n", rx/(float)sampletime/(float)1024, getunit(1), (uint64_t)(rxp/sampletime));
	printf("      tx     %10.2f %s/s         %5"PRIu64" packets/s\n\n", tx/(float)sampletime/(float)1024, getunit(1), (uint64_t)(txp/sampletime));

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
	len=21; /* length of previous print */
	fflush(stdout);

	/* enable signal trap */
	intsignal = 0;
	if (signal(SIGINT, sighandler) == SIG_ERR) {
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
	if (!getifinfo(iface)) {
		printf("Error: Interface \"%s\" not available, exiting.\n", iface);
		exit(1);
	}

	/* loop until user gets bored */
	while (intsignal==0) {

		/* wait 2 seconds for more traffic */
		sleep(LIVETIME);

		/* break loop without calculations because sleep was probably interrupted */
		if (intsignal) {
			break;
		}

		/* use values from previous loop if this isn't the first time */
		previnfo.rx = ifinfo.rx;
		previnfo.tx = ifinfo.tx;
		previnfo.rxp = ifinfo.rxp;
		previnfo.txp = ifinfo.txp;

		/* read those values again... */
		if (!getifinfo(iface)) {
			printf("Error: Interface \"%s\" not available, exiting.\n", iface);
			exit(1);
		}

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
		snprintf(buffer, 256, "   rx: %10.2f %s/s %5"PRIu64" p/s          tx: %10.2f %s/s %5"PRIu64" p/s", rxc, getunit(1), (uint64_t)rxpc, txc, getunit(1), (uint64_t)txpc);
		
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
		printf("  bytes                %s", getvalue(0, rxtotal/1024, 9, -1));
		printf("  | %s", getvalue(0, txtotal/1024, 9, -1));
		printf("\n");
		printf("--------------------------------------+----------------------------------------\n");
		printf("          max          ");
		showspeed(rxmax, 7);
		printf("  | ");
		showspeed(txmax, 7);
		printf("\n");
		printf("      average          ");
		showspeed(rxtotal/(float)timespent/(float)1024, 7);
		printf("  | ");
		showspeed(txtotal/(float)timespent/(float)1024, 7);
		printf("\n");
		printf("          min          ");
		showspeed(rxmin, 7);
		printf("  | ");
		showspeed(txmin, 7);
		printf("\n");
		printf("--------------------------------------+----------------------------------------\n");
		printf("  packets               %12"PRIu64"  |  %12"PRIu64"\n", rxptotal, txptotal);
		printf("--------------------------------------+----------------------------------------\n");
		printf("          max          %9"PRIu64" p/s  | %9"PRIu64" p/s\n", rxpmax, txpmax);
		printf("      average          %9"PRIu64" p/s  | %9"PRIu64" p/s\n", rxptotal/timespent, txptotal/timespent);
		printf("          min          %9"PRIu64" p/s  | %9"PRIu64" p/s\n", rxpmin, txpmin);
		printf("--------------------------------------+----------------------------------------\n");
		
		if (timespent<=60) {
			printf("  time             %9"PRIu64" seconds\n", timespent);
		} else {
			printf("  time               %7.2f minutes\n", timespent/(float)60);
		}

		printf("\n");
	}
	
}

void showspeed(float xfer, int len)
{
	if (cfg.unit>0) {
		len++;
	}

#if !defined(__OpenBSD__)
	if (xfer>=1000) {
		printf("%'*.2f %s/s", len, xfer/(float)1024, getunit(2));
	} else {
		printf("%'*.2f %s/s", len, xfer, getunit(1));
	}
#else
	if (xfer>=1000) {
		printf("%*.2f %s/s", len, xfer/(float)1024, getunit(2));
	} else {
		printf("%*.2f KiB/s", len, xfer, getunit(1));
	}
#endif
}
