#include "common.h"
#include "dbsql.h"
#include "misc.h"
#include "dbshow.h"

void showdb(const char *interface, int qmode, const char *databegin, const char *dataend)
{
	interfaceinfo info;

	if (!db_getinterfacecountbyname(interface)) {
		return;
	}

	if (!db_getinterfaceinfo(interface, &info)) {
		return;
	}

	if (info.rxtotal == 0 && info.txtotal == 0) {
		printf(" %s: Not enough data available yet.\n", interface);
		return;
	}

	switch(qmode) {
		case 0:
			showsummary(&info, 0);
			break;
		case 1:
			showlist(&info, "day", databegin, dataend);
			break;
		case 2:
			showlist(&info, "month", databegin, dataend);
			break;
		case 3:
			/* TODO: should data range selection be supported here? */
			showlist(&info, "top", "", "");
			break;
		case 5:
			showsummary(&info, 1);
			break;
		case 6:
			showlist(&info, "year", databegin, dataend);
			break;
		case 7:
			showhours(&info);
			break;
		case 9:
			showoneline(&info);
			break;
		case 11:
			showlist(&info, "hour", databegin, dataend);
			break;
		case 12:
			showlist(&info, "fiveminute", databegin, dataend);
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

	timeused(__func__, 1);

	current=time(NULL);
	yesterday=current-86400;

	e_rx=e_tx=0;

	if (interface->updated && !shortmode) {
		strftime(datebuff, DATEBUFFLEN, DATETIMEFORMAT, localtime(&interface->updated));
		printf("Database updated: %s\n\n", datebuff);
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

	if (!db_getdata(&datalist, &datainfo, interface->name, "month", 2)) {
		printf("Error: Failed to fetch month data.\n");
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
					e_rx = (uint64_t)(datalist_i->rx/(float)(mosecs(datalist_i->timestamp, interface->updated)))*(uint64_t)(dmonth(d->tm_mon)*86400);
					e_tx = (uint64_t)(datalist_i->tx/(float)(mosecs(datalist_i->timestamp, interface->updated)))*(uint64_t)(dmonth(d->tm_mon)*86400);
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

	if (!db_getdata(&datalist, &datainfo, interface->name, "day", 2)) {
		printf("Error: Failed to fetch day data.\n");
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
					e_rx = (uint64_t)((datalist_i->rx)/(float)(d->tm_hour*60+d->tm_min))*1440;
					e_tx = (uint64_t)((datalist_i->tx)/(float)(d->tm_hour*60+d->tm_min))*1440;
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
	timeused(__func__, 0);
}

void showlist(const interfaceinfo *interface, const char *listname, const char *databegin, const char *dataend)
{
	int32_t limit;
	int listtype, offset = 0, i = 1;
	struct tm *d;
	char datebuff[DATEBUFFLEN], daybuff[DATEBUFFLEN];
	char titlename[16], colname[8], stampformat[64];
	uint64_t e_rx, e_tx, e_secs, div, mult;
	dbdatalist *datalist = NULL, *datalist_i = NULL;
	dbdatalistinfo datainfo;

	timeused(__func__, 1);

	if (strcmp(listname, "day") == 0) {
		listtype = 1;
		strncpy_nt(colname, listname, 8);
		snprintf(titlename, 16, "daily");
		strncpy_nt(stampformat, cfg.dformat, 64);
		limit = cfg.listdays;
	} else if (strcmp(listname, "month") == 0) {
		listtype = 2;
		strncpy_nt(colname, listname, 8);
		snprintf(titlename, 16, "monthly");
		strncpy_nt(stampformat, cfg.mformat, 64);
		limit = cfg.listmonths;
	} else if (strcmp(listname, "year") == 0) {
		listtype = 3;
		strncpy_nt(colname, listname, 8);
		snprintf(titlename, 16, "yearly");
		strncpy_nt(stampformat, "%Y", 64);
		limit = cfg.listyears;
	} else if (strcmp(listname, "top") == 0) {
		listtype = 4;
		snprintf(colname, 8, "day");
		strncpy_nt(stampformat, cfg.tformat, 64);
		limit = cfg.listtop;
		offset = 6;
	} else if (strcmp(listname, "hour") == 0) {
		listtype = 5;
		strncpy_nt(colname, listname, 8);
		snprintf(titlename, 16, "hourly");
		strncpy_nt(stampformat, "%H:%M", 64);
		limit = cfg.listhours;
	} else if (strcmp(listname, "fiveminute") == 0) {
		listtype = 6;
		strncpy_nt(colname, "time", 8);
		snprintf(titlename, 16, "5 minute");
		strncpy_nt(stampformat, "%H:%M", 64);
		limit = cfg.listfivemins;
	} else {
		return;
	}

	if (limit < 0) {
		limit = 0;
	}

	daybuff[0] = '\0';
	e_rx = e_tx = e_secs = 0;

	if (!db_getdata_range(&datalist, &datainfo, interface->name, listname, (uint32_t)limit, databegin, dataend)) {
		printf("Error: Failed to fetch %s data.\n", titlename);
		return;
	}

	if (listtype == 4) {
		if (limit > 0 && datainfo.count < (uint32_t)limit) {
			limit = (int32_t)datainfo.count;
		}
		if (limit <= 0 || datainfo.count > 999) {
			snprintf(titlename, 16, "top");
		} else {
			snprintf(titlename, 16, "top %d", limit);
		}
	}

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
		if (listtype == 4) {
			printf("    # %8s  ", colname);
		} else {
			indent(5);
			printf("%8s", colname);
		}
		printf("        rx      |     tx      |    total    |   avg. rate\n");
		if (listtype == 4) {
			printf("   -----");
		} else {
			indent(5);
		}
		printf("------------------------+-------------+-------------+---------------\n");
	} else {
		if (listtype == 4) {
			printf("   # %8s  ", colname);
		} else {
			printf(" %8s", colname);
		}
		printf("        rx      |     tx      |    total\n");
		if (listtype == 4) {
			printf("------");
		}
		printf("-------------------------+-------------+------------");
		if (cfg.ostyle != 0) {
			printf("---------------------");
			if (listtype != 4) {
				printf("------");
			}
		}
		printf("\n");
	}

	datalist_i = datalist;

	while (datalist_i != NULL) {
		d = localtime(&datalist_i->timestamp);

		if (listtype == 5 || listtype == 6) {
			strftime(datebuff, DATEBUFFLEN, cfg.dformat, d);
			if (strcmp(daybuff, datebuff) != 0) {
				if (cfg.ostyle == 3) {
					indent(4);
				}
				printf(" %s\n", datebuff);
				strcpy(daybuff, datebuff);
			}
		}

		strftime(datebuff, DATEBUFFLEN, stampformat, d);
		if (cfg.ostyle == 3) {
			indent(1);
			if (listtype != 4) {
				indent(3);
			}
		}

		if (listtype == 4) {
			printf("  %2d  ", i);
		}

		if (strlen(datebuff)<=9 && listtype != 4) {
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
					e_secs = (uint64_t)(d->tm_sec+(d->tm_min*60)+(d->tm_hour*3600));
				} else if (listtype == 2) { // month
					e_secs = (uint64_t)mosecs(datalist_i->timestamp, interface->updated);
				} else if (listtype == 3) { // year
					e_secs = (uint64_t)(d->tm_yday*86400+d->tm_sec+(d->tm_min*60)+(d->tm_hour*3600));
				} else if (listtype == 4) { // top
					e_secs = 86400;
				} else if (listtype == 5) { // hour
					e_secs = (uint64_t)(d->tm_sec+(d->tm_min*60));
				} else if (listtype == 6) { // 5min
					e_secs = 300;
				}
			} else {
				if (listtype == 1 || listtype == 4) { // day
					e_secs = 86400;
				} else if (listtype == 2) { // month
					e_secs = (uint64_t)(dmonth(d->tm_mon) * 86400);
				} else if (listtype == 3) { // year
					e_secs = (uint64_t)((365 + isleapyear(d->tm_year+1900)) * 86400);
				} else if (listtype == 5) { // hour
					e_secs = 3600;
				} else if (listtype == 6) { // 5min
					e_secs = 300;
				}
			}
			printf(" | %s", gettrafficrate(datalist_i->rx+datalist_i->tx, (time_t)e_secs, 14));
		} else if (cfg.ostyle != 0) {
			showbar(datalist_i->rx, datalist_i->tx, datainfo.max, 24-offset);
		}
		printf("\n");
		if (datalist_i->next == NULL) {
			break;
		}
		datalist_i = datalist_i->next;
		i++;
	}
	if (datainfo.count == 0)
		printf("                           no data available\n");
	if (cfg.ostyle == 3) {
		if (listtype == 4) {
			printf("   -----");
		} else {
			indent(5);
		}
		printf("------------------------+-------------+-------------+---------------\n");
	} else {
		if (listtype == 4) {
			printf("------");
		}
		printf("-------------------------+-------------+------------");
		if (cfg.ostyle != 0) {
			printf("---------------------");
			if (listtype != 4) {
				printf("------");
			}
		}
		printf("\n");
	}
	if (datainfo.count > 0 && listtype < 4) {
		/* use database update time for estimates */
		d = localtime(&interface->updated);
		if ( datalist_i->rx==0 || datalist_i->tx==0 ) {
			e_rx = e_tx = 0;
		} else {
			div = 0;
			mult = 0;
			if (listtype == 1) { // day
				div = (uint64_t)(d->tm_hour * 60 + d->tm_min);
				mult = 1440;
			} else if (listtype == 2) { // month
				div = (uint64_t)mosecs(datalist_i->timestamp, interface->updated);
				mult = (uint64_t)(dmonth(d->tm_mon) * 86400);
			} else if (listtype == 3) { // year
				div = (uint64_t)(d->tm_yday * 1440 + d->tm_hour * 60 + d->tm_min);
				mult = (uint64_t)(1440 * (365 + isleapyear(d->tm_year + 1900)));
			}
			if (div > 0) {
				e_rx = (uint64_t)((datalist_i->rx) / (float)div) * mult;
				e_tx = (uint64_t)((datalist_i->tx) / (float)div) * mult;
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
	timeused(__func__, 0);
}

void showoneline(const interfaceinfo *interface)
{
	struct tm *d;
	char daytemp[DATEBUFFLEN];
	uint64_t div;
	dbdatalist *datalist = NULL;
	dbdatalistinfo datainfo;

	timeused(__func__, 1);

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

	if (!db_getdata(&datalist, &datainfo, interface->name, "day", 1)) {
		printf("\nError: Failed to fetch day data.\n");
		return;
	}

	d = localtime(&datalist->timestamp);
	strftime(daytemp, DATEBUFFLEN, cfg.dformat, d);
	printf("%s;", daytemp);

	d = localtime(&interface->updated);

	/* daily */
	if (cfg.ostyle == 4) {
		printf("%"PRIu64";", datalist->rx);
		printf("%"PRIu64";", datalist->tx);
		printf("%"PRIu64";", datalist->rx+datalist->tx);
		div = (uint64_t)(d->tm_sec+(d->tm_min*60)+(d->tm_hour*3600));
		if (!div) {
			div = 1;
		}
		printf("%"PRIu64";", (datalist->rx+datalist->tx)/div);
	} else {
		printf("%s;", getvalue(datalist->rx, 1, 1));
		printf("%s;", getvalue(datalist->tx, 1, 1));
		printf("%s;", getvalue(datalist->rx+datalist->tx, 1, 1));
		printf("%s;", gettrafficrate(datalist->rx+datalist->tx, (time_t)(d->tm_sec+(d->tm_min*60)+(d->tm_hour*3600)), 1));
	}
	dbdatalistfree(&datalist);

	if (!db_getdata(&datalist, &datainfo, interface->name, "month", 1)) {
		printf("\nError: Failed to fetch month data.\n");
		return;
	}

	d = localtime(&datalist->timestamp);
	strftime(daytemp, DATEBUFFLEN, cfg.mformat, d);
	printf("%s;", daytemp);

	/* monthly */
	if (cfg.ostyle == 4) {
		printf("%"PRIu64";", datalist->rx);
		printf("%"PRIu64";", datalist->tx);
		printf("%"PRIu64";", datalist->rx+datalist->tx);
		div = (uint64_t)(mosecs(datalist->timestamp, interface->updated));
		if (!div) {
			div = 1;
		}
		printf("%"PRIu64";", (datalist->rx+datalist->tx)/div);
	} else {
		printf("%s;", getvalue(datalist->rx, 1, 1));
		printf("%s;", getvalue(datalist->tx, 1, 1));
		printf("%s;", getvalue(datalist->rx+datalist->tx, 1, 1));
		printf("%s;", gettrafficrate(datalist->rx+datalist->tx, mosecs(datalist->timestamp, interface->updated), 1));
	}
	dbdatalistfree(&datalist);

	/* all time total */
	if (cfg.ostyle == 4) {
		printf("%"PRIu64";", interface->rxtotal);
		printf("%"PRIu64";", interface->txtotal);
		printf("%"PRIu64"\n", interface->rxtotal+interface->txtotal);
	} else {
		printf("%s;", getvalue(interface->rxtotal, 1, 1));
		printf("%s;", getvalue(interface->txtotal, 1, 1));
		printf("%s\n", getvalue(interface->rxtotal+interface->txtotal, 1, 1));
	}
	timeused(__func__, 0);
}

void showhours(const interfaceinfo *interface)
{
	int i, s=0, hour, minute, declen=cfg.hourlydecimals, div=1;
	unsigned int j, k, tmax=0, dots=0;
	uint64_t max=1;
	char matrix[24][81]; /* width is one over 80 so that snprintf can write the end char */
	char unit[4];
	struct tm *d;
	dbdatalist *datalist = NULL, *datalist_i = NULL;
	dbdatalistinfo datainfo;
	HOURDATA hourdata[24];

	timeused(__func__, 1);

	for (i=0; i<24; i++) {
		hourdata[i].rx = hourdata[i].tx = 0;
		hourdata[i].date = 0;
	}

	if (!db_getdata(&datalist, &datainfo, interface->name, "hour", 24)) {
		printf("Error: Failed to fetch hour data.\n");
		return;
	}

	if (datainfo.count == 0) {
		return;
	}

	datalist_i = datalist;

	while (datalist_i != NULL) {
		d = localtime(&datalist_i->timestamp);
		if (hourdata[d->tm_hour].date != 0 || interface->updated-datalist_i->timestamp > 86400) {
			datalist_i = datalist_i->next;
			continue;
		}
		hourdata[d->tm_hour].rx = datalist_i->rx;
		hourdata[d->tm_hour].tx = datalist_i->tx;
		hourdata[d->tm_hour].date = datalist_i->timestamp;
		datalist_i = datalist_i->next;
	}
	dbdatalistfree(&datalist);

	/* tmax = time max = current hour */
	/* max = transfer max */

	d = localtime(&interface->updated);
	hour = d->tm_hour;
	minute = d->tm_min;

	for (i=0; i<24; i++) {
		if (hourdata[i].date >= hourdata[tmax].date) {
			tmax = (unsigned int)i;
		}
		if (hourdata[i].rx >= max) {
			max = hourdata[i].rx;
		}
		if (hourdata[i].tx >= max) {
			max = hourdata[i].tx;
		}
	}

	/* mr. proper */
	for (i=0; i<24; i++) {
		for (j=0; j<81; j++) {
			matrix[i][j] = ' ';
		}
	}

	/* unit selection */
	while (max/(pow(1024, div)) >= 100 && div < UNITPREFIXCOUNT) {
		div++;
	}
	strncpy_nt(unit, getunitprefix(div), 4);
	div = (int)(pow(1024, div-1));
	if (div == 1) {
		declen = 0;
	}

	/* structure */
	snprintf(matrix[11], 81, " -+--------------------------------------------------------------------------->");
	for (i=0; i<3; i++) {
		snprintf(matrix[14]+(i*28), 14, " h %*srx (%s)", 1+cfg.unitmode, " ", unit);
		snprintf(matrix[14]+(i*28)+15+cfg.unitmode, 10, "tx (%s)", unit);
	}

	for (i=10;i>1;i--)
		matrix[i][2]='|';

	matrix[1][2]='^';
	matrix[12][2]='|';

	/* title */
	if (strcmp(interface->name, interface->alias) == 0 || strlen(interface->alias) == 0) {
		i = snprintf(matrix[0], 81, " %s", interface->name);
	} else {
		i = snprintf(matrix[0], 81, " %s (%s)", interface->alias, interface->name);
	}
	if (interface->active == 0) {
		snprintf(matrix[0]+i+1, 81, " [disabled]");
	}

	/* time to the corner */
	snprintf(matrix[0]+74, 7, "%02d:%02d", hour, minute);

	/* numbers under x-axis and graphics :) */
	k = 5;
	for (i=23; i>=0; i--) {
		s = (int)tmax - i;
		if (s < 0)
			s += 24;

		snprintf(matrix[12]+k, 81-k, "%02d ", s);

		dots = (unsigned int)(10 * (hourdata[s].rx / (float)max));
		for (j=0; j<dots; j++)
			matrix[10-j][k] = cfg.rxhourchar[0];

		dots = (unsigned int)(10 * (hourdata[s].tx / (float)max));
		for (j=0; j<dots; j++)
			matrix[10-j][k+1] = cfg.txhourchar[0];

		k = k + 3;
	}

	/* hours and traffic */
	for (i=0; i<=7; i++) {
		s = (int)tmax + i + 1;
		for (j=0; j<3; j++) {
			snprintf(matrix[15+i]+(j*28), 25, "%02d %"DECCONV"10.*f %"DECCONV"10.*f", \
			((unsigned int)s+(j*8))%24, declen, hourdata[((unsigned int)s+(j*8))%24].rx/(double)div, \
										declen, hourdata[((unsigned int)s+(j*8))%24].tx/(double)div);
		}
	}

	/* section separators */
	if (cfg.hourlystyle) {
		for (i=0;i<9;i++) {
			if (cfg.hourlystyle == 1) {
				matrix[14+i][26]='|';
				matrix[14+i][54]='|';
			} else if (cfg.hourlystyle == 2) {
				matrix[14+i][25]=']';
				matrix[14+i][26]='[';
				matrix[14+i][53]=']';
				matrix[14+i][54]='[';
			} else if (cfg.hourlystyle == 3) {
				matrix[14+i][26]='[';
				matrix[14+i][53]=']';
			}
		}
	}

	/* clean \0 */
	for (i=0; i<23; i++) {
		for (j=0; j<80; j++) {
			if (matrix[i][j] == '\0') {
				matrix[i][j] = ' ';
			}
		}
	}

	/* show matrix (yes, the last line isn't shown) */
	for (i=0; i<23; i++) {
		for (j=0; j<80; j++) {
			printf("%c",matrix[i][j]);
		}
		printf("\n");
	}
	timeused(__func__, 0);
}

int showbar(const uint64_t rx, const uint64_t tx, const uint64_t max, const int len)
{
	int i, l, width = len;

	if ( (rx + tx) < max) {
		width = (int)( ((rx + tx) / (float)max) * len );
	} else if ((rx + tx) > max) {
		return 0;
	}

	if (len <= 0) {
		return 0;
	}

	printf("  ");

	if (tx > rx) {
		l = (int)(rintf((rx/(float)(rx+tx)*width)));

		for (i=0; i<l; i++) {
			printf("%c", cfg.rxchar[0]);
		}
		for (i=0; i<(width-l); i++) {
			printf("%c", cfg.txchar[0]);
		}
	} else {
		l = (int)(rintf((tx/(float)(rx+tx)*width)));

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
