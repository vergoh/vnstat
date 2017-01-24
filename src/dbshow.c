#include "common.h"
#include "dbsql.h"
#include "misc.h"
#include "dbshow.h"

void showdb(const char *interface, int qmode)
{
	interfaceinfo info;

	if (!db_getinterfacecountbyname(interface)) {
		return;
	}

	if (!db_getinterfaceinfo(interface, &info)) {
		return;
	}

	if (info.rxtotal == 0 && info.txtotal == 0 && qmode != 4) {
		printf(" %s: Not enough data available yet.\n", interface);
		return;
	}

	switch(qmode) {
		case 0:
			showsummary(&info, 0);
			break;
		case 1:
			showlist(&info, "day");
			break;
		case 2:
			showlist(&info, "month");
			break;
/*		case 3:
			showtop();
			break;
		case 4:
			exportdb();
			break; */
		case 5:
			showsummary(&info, 1);
			break;
		case 6:
			showlist(&info, "year");
			break;
/*		case 7:
			showhours();
			break;*/
		case 9:
			showoneline(&info);
			break;
		default:
			printf("Error: Not such query mode: %d\n", qmode);
			break;
	}
}

void showsummary(const interfaceinfo *interface, const int shortmode)
{
	struct tm *d;
	char datebuff[DATEBUFFLEN];
	char todaystr[DATEBUFFLEN], yesterdaystr[DATEBUFFLEN];
	char fieldseparator[8];
	uint64_t e_rx, e_tx;
	time_t current, yesterday;
	dbdatalist *datalist = NULL, *datalist_i = NULL;
	dbdatalistinfo datainfo;

	current=time(NULL);
	yesterday=current-86400;

	e_rx=e_tx=0;

	if (interface->updated && !shortmode) {
		printf("Database updated: %s\n",(char*)asctime(localtime(&interface->updated)));
	} else if (!shortmode) {
		printf("\n");
	}

	if (!shortmode) {
		snprintf(fieldseparator, 8, " | ");
		indent(3);
	} else {
		snprintf(fieldseparator, 8, "  / ");
		indent(1);
	}
	if (strcmp(interface->name, interface->alias) == 0 || strlen(interface->alias) == 0) {
		printf("%s", interface->name);
	} else {
		printf("%s (%s)", interface->alias, interface->name);
	}
	if (interface->active == 0) {
		printf(" [disabled]");
	}
	if (shortmode) {
		printf(":\n");
	} else {
		/* get formatted date for creation date */
		d=localtime(&interface->created);
		strftime(datebuff, DATEBUFFLEN, cfg.tformat, d);
		printf(" since %s\n\n", datebuff);

		indent(10);
		printf("rx:  %s", getvalue(interface->rxtotal, 1, 1));
		indent(3);
		printf("   tx:  %s", getvalue(interface->txtotal, 1, 1));
		indent(3);
		printf("   total:  %s\n\n", getvalue(interface->rxtotal+interface->txtotal, 1, 1));

		indent(3);
		printf("monthly\n");
		indent(5);

		if (cfg.ostyle >= 2) {
			printf("                rx      |     tx      |    total    |   avg. rate\n");
			indent(5);
			printf("------------------------+-------------+-------------+---------------\n");
		} else {
			printf("                rx      |     tx      |    total\n");
			indent(5);
			printf("------------------------+-------------+------------\n");
		}
	}

	if (!db_getdata(&datalist, &datainfo, interface->name, "month", 2, 0)) {
		/* TODO: match with other output style */
		printf("Error: failed to fetch monthly data\n");
		return;
	}

	datalist_i = datalist;

	while (datalist_i != NULL) {
		indent(5);
		d = localtime(&datalist_i->timestamp);
		if (strftime(datebuff, DATEBUFFLEN, cfg.mformat, d)<=8) {
			printf("%*s   %s", getpadding(9, datebuff), datebuff, getvalue(datalist_i->rx, 11, 1));
		} else {
			printf("%-*s %s", getpadding(11, datebuff), datebuff, getvalue(datalist_i->rx, 11, 1));
		}
		printf("%s%s", fieldseparator, getvalue(datalist_i->tx, 11, 1));
		printf("%s%s", fieldseparator, getvalue(datalist_i->rx+datalist_i->tx, 11, 1));
		if (cfg.ostyle >= 2) {
			if (datalist_i->next == NULL) {
				if ( datalist_i->rx == 0 || datalist_i->tx == 0 || (interface->updated-datalist_i->timestamp) == 0 ) {
					e_rx = e_tx = 0;
				} else {
					e_rx = (datalist_i->rx/(float)(mosecs(datalist_i->timestamp, interface->updated)))*(dmonth(d->tm_mon)*86400);
					e_tx = (datalist_i->tx/(float)(mosecs(datalist_i->timestamp, interface->updated)))*(dmonth(d->tm_mon)*86400);
				}
				if (shortmode && cfg.ostyle != 0) {
					printf("%s%s", fieldseparator, getvalue(e_rx+e_tx, 11, 1));
				} else if (!shortmode) {
					printf("%s%s", fieldseparator, gettrafficrate(datalist_i->rx+datalist_i->tx, mosecs(datalist_i->timestamp, interface->updated), 14));
				}
			} else if (!shortmode) {
				printf(" | %s", gettrafficrate(datalist_i->rx+datalist_i->tx, dmonth(d->tm_mon)*86400, 14));
			}
		}
		printf("\n");
		datalist_i = datalist_i->next;
	}

	if (!shortmode) {
		indent(5);
		if (cfg.ostyle >= 2) {
			printf("------------------------+-------------+-------------+---------------\n");
		} else {
			printf("------------------------+-------------+------------\n");
		}
		indent(5);
		printf("estimated   %s", getvalue(e_rx, 11, 2));
		printf(" | %s", getvalue(e_tx, 11, 2));
		printf(" | %s", getvalue(e_rx+e_tx, 11, 2));
		if (cfg.ostyle >= 2) {
			printf(" |\n\n");
		} else {
			printf("\n\n");
		}
	}

	dbdatalistfree(&datalist);

	if (!shortmode) {
		indent(3);
		printf("daily\n");
		indent(5);

		if (cfg.ostyle >= 2) {
			printf("                rx      |     tx      |    total    |   avg. rate\n");
			indent(5);
			printf("------------------------+-------------+-------------+---------------\n");
		} else {
			printf("                rx      |     tx      |    total\n");
			indent(5);
			printf("------------------------+-------------+------------\n");
		}
	}

	/* get formatted date for today and yesterday */
	d = localtime(&current);
	strftime(todaystr, DATEBUFFLEN, cfg.dformat, d);
	d = localtime(&yesterday);
	strftime(yesterdaystr, DATEBUFFLEN, cfg.dformat, d);

	if (!db_getdata(&datalist, &datainfo, interface->name, "day", 2, 0)) {
		/* TODO: match with other output style */
		printf("Error: failed to fetch daily data\n");
		return;
	}

	datalist_i = datalist;

	while (datalist_i != NULL) {
		indent(5);
		d = localtime(&datalist_i->timestamp);
		strftime(datebuff, DATEBUFFLEN, cfg.dformat, d);
		if (strcmp(datebuff, todaystr) == 0) {
			snprintf(datebuff, DATEBUFFLEN, "    today");
		}
		if (strcmp(datebuff, yesterdaystr) == 0) {
			snprintf(datebuff, DATEBUFFLEN, "yesterday");
		}
		if (strlen(datebuff) <= 8) {
			printf("%*s   %s", getpadding(9, datebuff), datebuff, getvalue(datalist_i->rx, 11, 1));
		} else {
			printf("%-*s %s", getpadding(11, datebuff), datebuff, getvalue(datalist_i->rx, 11, 1));
		}
		printf("%s%s", fieldseparator, getvalue(datalist_i->tx, 11, 1));
		printf("%s%s", fieldseparator, getvalue(datalist_i->rx+datalist_i->tx, 11, 1));
		if (cfg.ostyle >= 2) {
			if (datalist_i->next == NULL) {
				d = localtime(&interface->updated);
				if ( datalist_i->rx == 0 || datalist_i->tx == 0 || (d->tm_hour*60+d->tm_min) == 0 ) {
					e_rx = e_tx = 0;
				} else {
					e_rx = ((datalist_i->rx)/(float)(d->tm_hour*60+d->tm_min))*1440;
					e_tx = ((datalist_i->tx)/(float)(d->tm_hour*60+d->tm_min))*1440;
				}
				if (shortmode && cfg.ostyle != 0) {
					printf("%s%s", fieldseparator, getvalue(e_rx+e_tx, 11, 2));
				} else if (!shortmode) {
					printf("%s%s", fieldseparator, gettrafficrate(datalist_i->rx+datalist_i->tx, d->tm_sec+(d->tm_min*60)+(d->tm_hour*3600), 14));
				}
			} else if (!shortmode) {
				printf(" | %s", gettrafficrate(datalist_i->rx+datalist_i->tx, 86400, 14));
			}
		}
		printf("\n");
		datalist_i = datalist_i->next;
	}

	if (!shortmode) {
		indent(5);
		if (cfg.ostyle >= 2) {
			printf("------------------------+-------------+-------------+---------------\n");
		} else {
			printf("------------------------+-------------+------------\n");
		}
		indent(5);
		printf("estimated   %s", getvalue(e_rx, 11, 2));
		printf(" | %s", getvalue(e_tx, 11, 2));
		printf(" | %s", getvalue(e_rx+e_tx, 11, 2));
		if (cfg.ostyle >= 2) {
			printf(" |\n");
		} else {
			printf("\n");
		}
	} else {
		printf("\n");
	}

	dbdatalistfree(&datalist);
}

void showlist(const interfaceinfo *interface, const char *listname)
{
	int limit, listtype;
	struct tm *d;
	char datebuff[DATEBUFFLEN], titlename[8], stampformat[64];
	uint64_t e_rx, e_tx, e_secs, div, mult;
	dbdatalist *datalist = NULL, *datalist_i = NULL;
	dbdatalistinfo datainfo;

	if (strcmp(listname, "day") == 0) {
		listtype = 1;
		snprintf(titlename, 8, "daily");
		strncpy_nt(stampformat, cfg.dformat, 64);
		limit = 30;
	} else if (strcmp(listname, "month") == 0) {
		listtype = 2;
		snprintf(titlename, 8, "monthly");
		strncpy_nt(stampformat, cfg.mformat, 64);
		limit = 12;
	} else if (strcmp(listname, "year") == 0) {
		listtype = 3;
		snprintf(titlename, 8, "yearly");
		strncpy_nt(stampformat, "%Y", 64);
		limit = -1;
	} else {
		return;
	}

	e_rx = e_tx = e_secs = 0;

	printf("\n");
	if (strcmp(interface->name, interface->alias) == 0 || strlen(interface->alias) == 0) {
		printf(" %s", interface->name);
	} else {
		printf(" %s (%s)", interface->alias, interface->name);
	}
	if (interface->active == 0) {
		printf(" [disabled]");
	}
	printf("  /  %s\n\n", titlename);

	if (cfg.ostyle == 3) {
		printf("     %8s        rx      |     tx      |    total    |   avg. rate\n", listname);
		printf("     ------------------------+-------------+-------------+---------------\n");
	} else {
		printf(" %8s        rx      |     tx      |    total\n", listname);
		if (cfg.ostyle != 0) {
			printf("-------------------------+-------------+---------------------------------------\n");
		} else {
			printf("-------------------------+-------------+------------\n");
		}
	}

	if (!db_getdata(&datalist, &datainfo, interface->name, listname, limit, 0)) {
		/* TODO: match with other output style */
		printf("Error: failed to fetch %s data\n", titlename);
		return;
	}

	datalist_i = datalist;

	while (datalist_i != NULL) {
		d = localtime(&datalist_i->timestamp);
		strftime(datebuff, DATEBUFFLEN, stampformat, d);
		if (cfg.ostyle == 3) {
			printf("    ");
		}
		if (strlen(datebuff)<=9) {
			printf(" %*s   %s", getpadding(9, datebuff), datebuff, getvalue(datalist_i->rx, 11, 1));
		} else {
			printf(" %-*s %s", getpadding(11, datebuff), datebuff, getvalue(datalist_i->rx, 11, 1));
		}
		printf(" | %s", getvalue(datalist_i->tx, 11, 1));
		printf(" | %s", getvalue(datalist_i->rx+datalist_i->tx, 11, 1));
		if (cfg.ostyle == 3) {
			if (datalist_i->next == NULL) {
				d = localtime(&interface->updated);
				if (listtype == 1) { // day
					e_secs = d->tm_sec+(d->tm_min*60)+(d->tm_hour*3600);
				} else if (listtype == 2) { // month
					e_secs = mosecs(datalist_i->timestamp, interface->updated);
				} else if (listtype == 3) { // year
					e_secs = d->tm_yday*86400+d->tm_sec+(d->tm_min*60)+(d->tm_hour*3600);
				}
			} else {
				if (listtype == 1) { // day
					e_secs = 86400;
				} else if (listtype == 2) { // month
					e_secs = dmonth(d->tm_mon) * 86400;
				} else if (listtype == 3) { // year
					e_secs = (365 + isleapyear(d->tm_year+1900)) * 86400;
				}
			}
			printf(" | %s", gettrafficrate(datalist_i->rx+datalist_i->tx, e_secs, 14));
		} else if (cfg.ostyle != 0) {
			showbar(datalist_i->rx, datalist_i->tx, datainfo.max, 24);
		}
		printf("\n");
		if (datalist_i->next == NULL) {
			break;
		}
		datalist_i = datalist_i->next;
	}
	if (datainfo.count == 0)
		printf("                           no data available\n");
	if (cfg.ostyle == 3) {
		printf("     ------------------------+-------------+-------------+---------------\n");
	} else {
		if (cfg.ostyle != 0) {
			printf("-------------------------+-------------+---------------------------------------\n");
		} else {
			printf("-------------------------+-------------+------------\n");
		}
	}
	if (datainfo.count > 0) {
		/* use database update time for estimates */
		d = localtime(&interface->updated);
		if ( datalist_i->rx==0 || datalist_i->tx==0 ) {
			e_rx = e_tx = 0;
		} else {
			div = 0;
			mult = 0;
			if (listtype == 1) { // day
				div = d->tm_hour * 60 + d->tm_min;
				mult = 1440;
			} else if (listtype == 2) { // month
				div = mosecs(datalist_i->timestamp, interface->updated);
				mult = dmonth(d->tm_mon) * 86400;
			} else if (listtype == 3) { // year
				div = d->tm_yday * 1440 + d->tm_hour * 60 + d->tm_min;
				mult = 1440 * (365 + isleapyear(d->tm_year + 1900));
			}
			if (div > 0) {
				e_rx = ((datalist_i->rx) / (float)div) * mult;
				e_tx = ((datalist_i->tx) / (float)div) * mult;
			} else {
				e_rx = e_tx = 0;
			}
		}
		if (cfg.ostyle == 3) {
			printf("    ");
		}
		printf(" estimated   %s", getvalue(e_rx, 11, 2));
		printf(" | %s", getvalue(e_tx, 11, 2));
		printf(" | %s", getvalue(e_rx + e_tx, 11, 2));
		if (cfg.ostyle == 3) {
			printf(" |");
		}
		printf("\n");
	}
	dbdatalistfree(&datalist);
}

void showoneline(const interfaceinfo *interface)
{
	struct tm *d;
	char daytemp[DATEBUFFLEN];
	dbdatalist *datalist = NULL;
	dbdatalistinfo datainfo;

	/* version string */
	printf("%d;", ONELINEVERSION);

	/* interface name */
	if (strcmp(interface->name, interface->alias) == 0 || strlen(interface->alias) == 0) {
		printf("%s", interface->name);
	} else {
		printf("%s (%s)", interface->alias, interface->name);
	}
	if (interface->active == 0) {
		printf(" [disabled]");
	}
	printf(";");

	if (!db_getdata(&datalist, &datainfo, interface->name, "day", 1, 0)) {
		/* TODO: match with other output style */
		printf("\nError: failed to fetch daily data\n");
		return;
	}

	d = localtime(&datalist->timestamp);
	strftime(daytemp, DATEBUFFLEN, cfg.dformat, d);
	printf("%s;", daytemp);

	d = localtime(&interface->updated);

	/* daily */
	printf("%s;", getvalue(datalist->rx, 1, 1));
	printf("%s;", getvalue(datalist->tx, 1, 1));
	printf("%s;", getvalue(datalist->rx+datalist->tx, 1, 1));
	printf("%s;", gettrafficrate(datalist->rx+datalist->tx, d->tm_sec+(d->tm_min*60)+(d->tm_hour*3600), 1));
	dbdatalistfree(&datalist);

	if (!db_getdata(&datalist, &datainfo, interface->name, "month", 1, 0)) {
		/* TODO: match with other output style */
		printf("\nError: failed to fetch monthly data\n");
		return;
	}

	d = localtime(&datalist->timestamp);
	strftime(daytemp, DATEBUFFLEN, cfg.mformat, d);
	printf("%s;", daytemp);

	/* monthly */
	printf("%s;", getvalue(datalist->rx, 1, 1));
	printf("%s;", getvalue(datalist->tx, 1, 1));
	printf("%s;", getvalue(datalist->rx+datalist->tx, 1, 1));
	printf("%s;", gettrafficrate(datalist->rx+datalist->tx, mosecs(datalist->timestamp, interface->updated), 1));
	dbdatalistfree(&datalist);

	/* all time total */
	printf("%s;", getvalue(interface->rxtotal, 1, 1));
	printf("%s;", getvalue(interface->txtotal, 1, 1));
	printf("%s\n", getvalue(interface->rxtotal+interface->txtotal, 1, 1));
}

int showbar(const uint64_t rx, const uint64_t tx, const uint64_t max, const int len)
{
	int i, l, width = len;

	if ( (rx + tx) < max) {
		width = ( (rx + tx) / (float)max ) * len;
	} else if ((rx + tx) > max) {
		return 0;
	}

	if (len <= 0) {
		return 0;
	}

	printf("  ");

	if (tx > rx) {
		l = rintf((rx/(float)(rx+tx)*width));

		for (i=0; i<l; i++) {
			printf("%c", cfg.rxchar[0]);
		}
		for (i=0; i<(width-l); i++) {
			printf("%c", cfg.txchar[0]);
		}
	} else {
		l = rintf((tx/(float)(rx+tx)*width));

		for (i=0; i<(width-l); i++) {
			printf("%c", cfg.rxchar[0]);
		}
		for (i=0; i<l; i++) {
			printf("%c", cfg.txchar[0]);
		}
	}
	return width;
}

void indent(int i)
{
	if ((cfg.ostyle > 0) && (i > 0)) {
		printf("%*s", i, " ");
	}
}
