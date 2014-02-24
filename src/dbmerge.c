#include "common.h"
#include "dbaccess.h"
#include "dbmerge.h"

int mergedb(char iface[], char dirname[])
{
	DATA mergedata;
	char *ifaceptr;

	if (!strstr(iface, "+")) {
		return 0;
	}

	/* create empty database */
	emptydb(&mergedata);
	strncpy(mergedata.interface, iface, 32);
	strncpy(mergedata.nick, mergedata.interface, 32);

	if (debug)
		printf("iface merge: %s\n", iface);

	ifaceptr = strtok(iface, "+");

	/* merge all databases in given string */
	while (ifaceptr != NULL) {
		if (debug)
			printf("merging %s:\n", ifaceptr);

		if (readdb(ifaceptr, dirname)!=0) {
			printf("Merge \"%s\" failed.\n", mergedata.interface);
			return 0;
		}

		if (!mergewith(&mergedata)) {
			printf("Merge \"%s\" failed for interface \"%s\".\n", mergedata.interface, ifaceptr);
			return 0;
		}

		ifaceptr = strtok(NULL, "+");
	}

	/* clean possible glitches */
	cleanmerged(&mergedata);

	/* replace active data with merged */
	if (memcpy(&data, &mergedata, sizeof(data)) != NULL) {
		return 1;	
	} else {
		return 0;
	}
}

void emptydb(DATA *dat)
{
	int i;
	struct tm *d;
	time_t current;

	current = time(NULL);

	dat->version = DBVERSION;
	dat->active = 1;
	dat->totalrx = 0;
	dat->totaltx = 0;
	dat->currx = 0;
	dat->curtx = 0;
	dat->totalrxk = 0;
	dat->totaltxk = 0;
	dat->lastupdated = 0;
	dat->created = current;

	/* days */
	d = localtime(&current);
	for (i=0;i<=29;i++) {
		dat->day[i].rx = 0;
		dat->day[i].tx = 0;
		dat->day[i].rxk = 0;
		dat->day[i].txk = 0;
		dat->day[i].date = mktime(d);
		dat->day[i].used = 1;
		d->tm_mday--;
	}

	/* months */
	d = localtime(&current);
	for (i=0;i<=11;i++) {
		dat->month[i].rx = 0;
		dat->month[i].tx = 0;
		dat->month[i].rxk = 0;
		dat->month[i].txk = 0;
		dat->month[i].month = mktime(d);
		dat->month[i].used = 1;
		d->tm_mon--;
	}

	/* top10 */
	for (i=0;i<=9;i++) {
		dat->top10[i].rx = 0;
		dat->top10[i].tx = 0;
		dat->top10[i].rxk = 0;
		dat->top10[i].txk = 0;
		dat->top10[i].date = 0;
		dat->top10[i].used = 0;
	}

	/* hours */
	for (i=0;i<=23;i++) {
		dat->hour[i].rx = 0;
		dat->hour[i].tx = 0;
		dat->hour[i].date = 0;
	}

	dat->btime = 0;
}

int mergewith(DATA *dat)
{
	int i, j, orig, merged;
	struct tm *d;

	/* merge totals */
	dat->totalrx += data.totalrx;
	dat->totaltx += data.totaltx;
	dat->totalrxk += data.totalrxk;
	dat->totaltxk += data.totaltxk;

	if (data.created < dat->created) {
		dat->created = data.created;
	}

	if (data.lastupdated > dat->lastupdated) {
		dat->lastupdated = data.lastupdated;
	}

	/* clean hours from loaded db */
	cleanhours();

	/* merge hours */
	for (i=0;i<=23;i++) {
		if (data.hour[i].date!=0) {
			dat->hour[i].rx += data.hour[i].rx;
			dat->hour[i].tx += data.hour[i].tx;
			dat->hour[i].date = data.hour[i].date;
		}
	}

	/* merge days */
	for (i=0;i<=29;i++) {
		if (data.day[i].used) {
			d = localtime(&data.day[i].date);
			orig = d->tm_year * 1000 + d->tm_yday;

			for (j=0;j<=29;j++) {
				d = localtime(&dat->day[j].date);
				merged = d->tm_year * 1000 + d->tm_yday;

				if (orig == merged) {
					dat->day[j].rx += data.day[i].rx;
					dat->day[j].tx += data.day[i].tx;
					dat->day[j].rxk += data.day[i].rxk;
					dat->day[j].txk += data.day[i].txk;
					if (dat->day[j].date > data.day[i].date) {
						dat->day[j].date = data.day[i].date;
					}
				} else if (merged < orig) {
					break;
				}
			}

		}
	}

	/* merge months */
	for (i=0;i<=11;i++) {
		if (data.month[i].used) {
			d = localtime(&data.month[i].month);
			orig = d->tm_year * 100 + d->tm_mon;

			for (j=0;j<=11;j++) {
				d = localtime(&dat->month[j].month);
				merged = d->tm_year * 100 + d->tm_mon;

				if (orig == merged) {
					dat->month[j].rx += data.month[i].rx;
					dat->month[j].tx += data.month[i].tx;
					dat->month[j].rxk += data.month[i].rxk;
					dat->month[j].txk += data.month[i].txk;
					if (dat->month[j].month > data.month[i].month) {
						dat->month[j].month = data.month[i].month;
					}
				} else if (merged < orig) {
					break;
				}
			}

		}
	}

	return 1;
}

void cleanmerged(DATA *dat)
{
	int i;

	/* clean days */
	for (i=29;i>=0;i--) {
		if ((dat->day[i].rx == 0) && (dat->day[i].tx == 0) && (dat->day[i].rxk == 0) && (dat->day[i].txk == 0)) {
			dat->day[i].used = 0;
		} else {
			break;
		}
	}

	/* clean days */
	for (i=11;i>=0;i--) {
		if ((dat->month[i].rx == 0) && (dat->month[i].tx == 0) && (dat->month[i].rxk == 0) && (dat->month[i].txk == 0)) {
			dat->month[i].used = 0;
		} else {
			break;
		}
	}
}
