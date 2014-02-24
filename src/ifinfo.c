#include "common.h"
#include "misc.h"
#include "dbaccess.h"
#include "cfg.h"
#include "ifinfo.h"

int getifinfo(const char *iface)
{
	char inface[32];

	ifinfo.filled = 0;

	if (strcmp(iface, "default")==0) {
		strncpy(inface, cfg.iface, 32);
	} else {
		strncpy(inface, iface, 32);
	}

#if defined(__linux__)
	/* try getting interface info from /proc */
	if (readproc(inface)!=1) {
		if (debug)
			printf("Failed to use %s as source.\n", PROCNETDEV);

		/* try getting interface info from /sys */
		if (readsysclassnet(inface)!=1) {
			snprintf(errorstring, 512, "Unable to get interface \"%s\" statistics.", inface);
			printe(PT_Error);
			return 0;
		}
	}
#elif defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__) || defined(__APPLE__)  || defined(__FreeBSD_kernel__)
	if (readifaddrs(inface)!=1) {
		snprintf(errorstring, 512, "Unable to get interface \"%s\" statistics.", inface);
		printe(PT_Error);
		return 0;
	}
#else
	return 0;
#endif

	return 1;
}

int getiflist(char **ifacelist)
{
#if defined(__linux__)
	char interface[32];
	FILE *fp;
	DIR *dp;
	struct dirent *di;
	char procline[512], temp[64];
#elif defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__) || defined(__APPLE__) || defined(__FreeBSD_kernel__)
	struct ifaddrs *ifap, *ifa;
#endif

	/* initialize list */
	*ifacelist = malloc(sizeof(char));
	*ifacelist[0] = '\0';

#if defined(__linux__)
	if ((fp=fopen(PROCNETDEV, "r"))!=NULL) {

		/* make list of interfaces */
		while (fgets(procline, 512, fp)!=NULL) {
			sscanf(procline, "%64s", temp);
			if (isdigit(temp[(strlen(temp)-1)]) || temp[(strlen(temp)-1)]==':') {
				sscanf(temp, "%32[^':']s", interface);
				*ifacelist = realloc(*ifacelist, ( ( strlen(*ifacelist) + strlen(interface) + 2 ) * sizeof(char)) );
				strncat(*ifacelist, interface, strlen(interface));
				strcat(*ifacelist, " ");
			}
		}

		fclose(fp);
		return 1;

	} else {

		if ((dp=opendir("/sys/class/net"))!=NULL) {

			/* make list of interfaces */
			while ((di=readdir(dp))) {
				if (di->d_name[0]!='.') {
					*ifacelist = realloc(*ifacelist, ( ( strlen(*ifacelist) + strlen(di->d_name) + 2 ) * sizeof(char)) );
					strncat(*ifacelist, di->d_name, strlen(di->d_name));
					strcat(*ifacelist, " ");
				}
			}

			closedir(dp);
			return 1;

		}	
	}

#elif defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__) || defined(__APPLE__) || defined(__FreeBSD_kernel__)
	if (getifaddrs(&ifap) >= 0) {

		/* make list of interfaces */
		for (ifa = ifap; ifa; ifa = ifa->ifa_next) {
			if (ifa->ifa_addr->sa_family == AF_LINK) {
				*ifacelist = realloc(*ifacelist, ( ( strlen(*ifacelist) + strlen(ifa->ifa_name) + 2 ) * sizeof(char)) );
				strncat(*ifacelist, ifa->ifa_name, strlen(ifa->ifa_name));
				strcat(*ifacelist, " ");
			}
		}

		freeifaddrs(ifap);
		return 1;
	}

#endif

	return 0;
}

int readproc(const char *iface)
{
	FILE *fp;
	char temp[4][64], procline[512], *proclineptr, ifaceid[33];
	int check;

	if ((fp=fopen(PROCNETDEV, "r"))==NULL) {
		if (debug)
			printf("Error: Unable to read %s.\n", PROCNETDEV);
		return 0;
	}

	strncpy(ifaceid, iface, 32);
	strcat(ifaceid, ":");

	check = 0;
	while (fgets(procline, 512, fp)!=NULL) {
		sscanf(procline, "%512s", temp[0]);
		if (strncmp(ifaceid, temp[0], strlen(ifaceid))==0) {
			/* if (debug)
				printf("\n%s\n", procline); */
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
		sscanf(proclineptr+1, "%64s %64s %*s %*s %*s %*s %*s %*s %64s %64s", temp[0], temp[1], temp[2], temp[3]);

		ifinfo.rx = strtoull(temp[0], (char **)NULL, 0);
		ifinfo.tx = strtoull(temp[2], (char **)NULL, 0);

		/* daemon doesn't need packet data */
		if (!noexit) {
			ifinfo.rxp = strtoull(temp[1], (char **)NULL, 0);
			ifinfo.txp = strtoull(temp[3], (char **)NULL, 0);
		}

		ifinfo.filled = 1;
	}

	return 1;
}

int readsysclassnet(const char *iface)
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

	/* daemon doesn't need packet data */
	if (!noexit) {

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
	}

	ifinfo.filled = 1;

	return 1;
}

void parseifinfo(int newdb)
{
	uint64_t rxchange=0, txchange=0, btime, cc;   /* rxchange = rx change in MB */
	uint64_t krxchange=0, ktxchange=0, maxtransfer;   /* krxchange = rx change in kB */
	time_t current, interval;
	struct tm *d;
	int day, month, year, hour, min, shift, maxbw;
	int rxkchange=0, txkchange=0;			          /* changes in the kB counters */

	ifinfo.rxp = ifinfo.txp = 0;
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
			cc = countercalc(data.currx, ifinfo.rx);
			rxchange = cc/1048576;      /* 1024/1024 */
			rxkchange = (cc/1024)%1024;
			krxchange = cc/1024;
			ifinfo.rxp = cc%1024;

			cc = countercalc(data.curtx, ifinfo.tx);
			txchange = cc/1048576;      /* 1024/1024 */
			txkchange = (cc/1024)%1024;
			ktxchange = cc/1024;
			ifinfo.txp = cc%1024;
		}

		/* get bandwidth limit for current interface */
		maxbw = ibwget(data.interface);

		if (maxbw > 0) {

			/* calculate maximum possible transfer since last update based on set maximum rate */
			/* and add 10% in order to be on the safe side */
			maxtransfer = ceil((maxbw/(float)8)*interval*(float)1.1);

			if (debug)
				printf("interval: %"PRIu64"  maxbw: %d  maxrate: %"PRIu64"  rxc: %"PRIu64"  txc: %"PRIu64"\n", (uint64_t)interval, maxbw, maxtransfer, rxchange, txchange); 

			/* sync counters if traffic is greater than set maximum */
			if ( (rxchange > maxtransfer) || (txchange > maxtransfer) ) {
				snprintf(errorstring, 512, "Traffic rate for \"%s\" higher than set maximum %d Mbit (%"PRIu64"->%"PRIu64", r%"PRIu64" t%"PRIu64"), syncing.", data.interface, maxbw, (uint64_t)interval, maxtransfer, rxchange, txchange);
				printe(PT_Info);
				rxchange = krxchange = rxkchange = txchange = ktxchange = txkchange = 0;
				ifinfo.rxp = ifinfo.txp = 0;
			}
		}

	} else {
		if (debug)
			printf("Too much time passed since previous update, syncing. (%"PRIu64" < %d)\n", (uint64_t)interval, 60*MAXUPDATEINTERVAL);
	}


	/* keep btime updated in case it drifts slowly */
	data.btime = btime;

	data.currx = ifinfo.rx - ifinfo.rxp;
	data.curtx = ifinfo.tx - ifinfo.txp;
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
	shift=hour;

	/* add traffic to previous hour when update happens at X:00 */
	/* and previous update was during previous hour */
	d=localtime(&data.lastupdated);
	if ((min==0) && (d->tm_hour!=hour) && ((current-data.lastupdated)<=3600)) {
		hour--;
		if (hour<0) {
			hour=23;
		}
	}

	/* clean and update hourly */
	cleanhours();
	data.hour[shift].date=current;   /* avoid shifting timestamp */
	data.hour[hour].rx+=krxchange;
	data.hour[hour].tx+=ktxchange;

	/* rotate days in database if needed */
	d=localtime(&data.day[0].date);
	if ((d->tm_mday!=day) || (d->tm_mon!=month) || (d->tm_year!=year)) {

		/* make a new entry only if there's something to remember (configuration dependent) */
		if ( (data.day[0].rx==0) && (data.day[0].tx==0) && (data.day[0].rxk==0) && (data.day[0].txk==0) && (cfg.traflessday==0) ) {
			data.day[0].date=current;
		} else {
			rotatedays();
		}
	}

	/* rotate months in database if needed */
	d=localtime(&data.month[0].month);
	if ((d->tm_mon!=month) && (day>=cfg.monthrotate)) {
		rotatemonths();
	}
}

uint64_t countercalc(uint64_t a, uint64_t b)
{
	/* no flip */
	if (b>=a) {
		if (debug)
			printf("cc: %"PRIu64" - %"PRIu64" = %"PRIu64"\n", b, a, b-a);
		return b-a;

	/* flip exists */
	} else {
		/* original counter is 64bit */
		if (a>FP32) {
			if (debug)
				printf("cc64: uint64 - %"PRIu64" + %"PRIu64" = %"PRIu64"\n", a, b, (uint64_t)FP64-a+b);
			return FP64-a+b;

		/* original counter is 32bit */
		} else {
			if (debug)
				printf("cc32: uint32 - %"PRIu64" + %"PRIu64" = %"PRIu64"\n", a, b, (uint64_t)FP32-a+b);
			return FP32-a+b;
		}
	}
}

#if defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__) || defined(__APPLE__) || defined(__FreeBSD_kernel__)
int readifaddrs(const char *iface)
{
	struct ifaddrs *ifap, *ifa;
	struct if_data *ifd = NULL;
	int check = 0;

	if (getifaddrs(&ifap) < 0) {
		if (debug)
			printf("getifaddrs() failed.. exiting.\n");
		return 0;
	}
	for (ifa = ifap; ifa; ifa = ifa->ifa_next) {
		if ((strcmp(ifa->ifa_name, iface) == 0) && (ifa->ifa_addr->sa_family == AF_LINK)) {
			ifd = ifa->ifa_data;
			check = 1;
			break;
		}
	}
	freeifaddrs(ifap);

	if (check == 0) {
		if (debug)
			printf("Requested interface \"%s\" not found.\n", iface);
		return 0;
	} else {
		strncpy(ifinfo.name, iface, 32);
		ifinfo.rx = ifd->ifi_ibytes;
		ifinfo.tx = ifd->ifi_obytes;
		ifinfo.rxp = ifd->ifi_ipackets;
		ifinfo.txp = ifd->ifi_opackets;
		ifinfo.filled = 1;
	}

	return 1;
}
#endif
