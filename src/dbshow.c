#include "common.h"
#include "dbsql.h"
#include "misc.h"
#include "percentile.h"
#include "dbshow.h"

// TODO: use exit(EXIT_FAILURE); for errors instead of return
void showdb(const char *interface, int qmode, const char *databegin, const char *dataend)
{
	interfaceinfo info;
	char timestamp[22];

	if (!db_getinterfacecountbyname(interface)) {
		return;
	}

	if (!db_getinterfaceinfo(interface, &info)) {
		return;
	}

	if (info.created == info.updated) {
		strftime(timestamp, 22, DATETIMEFORMAT, localtime(&info.updated));
		printf(" %s: No data. Timestamp of last update is same %s as of database creation.\n", interface, timestamp);
		return;
	}

	switch (qmode) {
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
			showlist(&info, "top", databegin, dataend);
			break;
		case 4:
			showsummary(&info, 0);
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
		case 13:
			show95thpercentile(&info);
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

	timeused_debug(__func__, 1);

	current = time(NULL);
	yesterday = current - 86400;

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
		d = localtime(&interface->created);
		strftime(datebuff, DATEBUFFLEN, cfg.tformat, d);
		printf(" since %s\n\n", datebuff);

		if (cfg.ostyle == 1) {
			indent(3);
		} else {
			indent(10);
		}
		printf("rx:  %s", getvalue(interface->rxtotal, 1, RT_Normal));
		indent(3);
		printf("   tx:  %s", getvalue(interface->txtotal, 1, RT_Normal));
		indent(3);
		printf("   total:  %s\n\n", getvalue(interface->rxtotal + interface->txtotal, 1, RT_Normal));

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

	e_rx = e_tx = 0;
	datalist_i = datalist;

	while (datalist_i != NULL) {
		indent(5);
		d = localtime(&datalist_i->timestamp);
		if (strftime(datebuff, DATEBUFFLEN, cfg.mformat, d) <= 8) {
			printf("%*s   %s", getpadding(9, datebuff), datebuff, getvalue(datalist_i->rx, 11, RT_Normal));
		} else {
			printf("%-*s %s", getpadding(11, datebuff), datebuff, getvalue(datalist_i->rx, 11, RT_Normal));
		}
		printf("%s%s", fieldseparator, getvalue(datalist_i->tx, 11, RT_Normal));
		printf("%s%s", fieldseparator, getvalue(datalist_i->rx + datalist_i->tx, 11, RT_Normal));
		if (datalist_i->next == NULL && issametimeslot(LT_Month, datalist_i->timestamp, interface->updated)) {
			if (datalist_i->rx == 0 || datalist_i->tx == 0 || (interface->updated - datalist_i->timestamp) == 0) {
				e_rx = e_tx = 0;
			} else if (cfg.estimatevisible) {
				getestimates(&e_rx, &e_tx, LT_Month, interface->updated, interface->created, &datalist);
			}
			if (shortmode && cfg.ostyle != 0 && cfg.estimatevisible) {
				printf("%s%s", fieldseparator, getvalue(e_rx + e_tx, 11, RT_Estimate));
			} else if (!shortmode && cfg.ostyle >= 2) {
				printf("%s%s", fieldseparator, gettrafficrate(datalist_i->rx + datalist_i->tx, (time_t)getperiodseconds(LT_Month, datalist_i->timestamp, interface->updated, interface->created, 1), 14));
			}
		} else if (!shortmode && cfg.ostyle >= 2) {
			printf(" | %s", gettrafficrate(datalist_i->rx + datalist_i->tx, dmonth(d->tm_mon) * 86400, 14));
		}
		printf("\n");
		datalist_i = datalist_i->next;
	}

	if (!datalist) {
		indent(5 + 27);
		printf("no data available\n");
	}

	if (!shortmode) {
		indent(5);
		if (cfg.ostyle >= 2) {
			printf("------------------------+-------------+-------------+---------------\n");
		} else {
			printf("------------------------+-------------+------------\n");
		}
		if (cfg.estimatevisible) {
			indent(5);
			printf("%9s   %s", cfg.estimatetext, getvalue(e_rx, 11, RT_Estimate));
			printf(" | %s", getvalue(e_tx, 11, RT_Estimate));
			printf(" | %s", getvalue(e_rx + e_tx, 11, RT_Estimate));
			if (cfg.ostyle >= 2) {
				printf(" |\n");
			} else {
				printf("\n");
			}
		}
		printf("\n");
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

	e_rx = e_tx = 0;
	datalist_i = datalist;

	while (datalist_i != NULL) {
		indent(5);
		d = localtime(&datalist_i->timestamp);
		strftime(datebuff, DATEBUFFLEN, cfg.dformat, d);
		if (strcmp(datebuff, todaystr) == 0) {
			snprintf(datebuff, DATEBUFFLEN, "    today");
		} else if (strcmp(datebuff, yesterdaystr) == 0) {
			snprintf(datebuff, DATEBUFFLEN, "yesterday");
		}
		if (strlen(datebuff) <= 8) {
			printf("%*s   %s", getpadding(9, datebuff), datebuff, getvalue(datalist_i->rx, 11, RT_Normal));
		} else {
			printf("%-*s %s", getpadding(11, datebuff), datebuff, getvalue(datalist_i->rx, 11, RT_Normal));
		}
		printf("%s%s", fieldseparator, getvalue(datalist_i->tx, 11, RT_Normal));
		printf("%s%s", fieldseparator, getvalue(datalist_i->rx + datalist_i->tx, 11, RT_Normal));
		if (datalist_i->next == NULL && issametimeslot(LT_Day, datalist_i->timestamp, interface->updated)) {
			d = localtime(&interface->updated);
			if (datalist_i->rx == 0 || datalist_i->tx == 0 || (d->tm_hour * 60 + d->tm_min) == 0) {
				e_rx = e_tx = 0;
			} else if (cfg.estimatevisible) {
					e_rx = (uint64_t)((double)datalist_i->rx / (double)(d->tm_hour * 60 + d->tm_min)) * 1440;
					e_tx = (uint64_t)((double)datalist_i->tx / (double)(d->tm_hour * 60 + d->tm_min)) * 1440;
			}
			if (shortmode && cfg.ostyle != 0 && cfg.estimatevisible) {
				printf("%s%s", fieldseparator, getvalue(e_rx + e_tx, 11, RT_Estimate));
			} else if (!shortmode && cfg.ostyle >= 2) {
				printf("%s%s", fieldseparator, gettrafficrate(datalist_i->rx + datalist_i->tx, (time_t)getperiodseconds(LT_Day, datalist_i->timestamp, interface->updated, interface->created, 1), 14));
			}
		} else if (!shortmode && cfg.ostyle >= 2) {
			printf(" | %s", gettrafficrate(datalist_i->rx + datalist_i->tx, 86400, 14));
		}
		printf("\n");
		datalist_i = datalist_i->next;
	}

	if (!shortmode) {
		if (!datalist) {
			indent(5 + 27);
			printf("no data available\n");
		}
		indent(5);
		if (cfg.ostyle >= 2) {
			printf("------------------------+-------------+-------------+---------------\n");
		} else {
			printf("------------------------+-------------+------------\n");
		}
		if (cfg.estimatevisible) {
			indent(5);
			printf("%9s   %s", cfg.estimatetext, getvalue(e_rx, 11, RT_Estimate));
			printf(" | %s", getvalue(e_tx, 11, RT_Estimate));
			printf(" | %s", getvalue(e_rx + e_tx, 11, RT_Estimate));
			if (cfg.ostyle >= 2) {
				printf(" |\n");
			} else {
				printf("\n");
			}
		}
	} else {
		printf("\n");
	}

	dbdatalistfree(&datalist);
	timeused_debug(__func__, 0);
}

void showlist(const interfaceinfo *interface, const char *listname, const char *databegin, const char *dataend)
{
	int32_t limit, retention;
	ListType listtype = LT_None;
	int offset = 0, i = 1;
	int estimatevisible = 0;
	struct tm *d;
	time_t current;
	char datebuff[DATEBUFFLEN], daybuff[DATEBUFFLEN];
	char titlename[16], colname[8], stampformat[64];
	uint64_t e_rx = 0, e_tx = 0, e_secs = 0;
	dbdatalist *datalist = NULL, *datalist_i = NULL;
	dbdatalistinfo datainfo;

	timeused_debug(__func__, 1);

	if (strcmp(listname, "day") == 0) {
		listtype = LT_Day;
		strncpy_nt(colname, listname, 8);
		snprintf(titlename, 16, "daily");
		strncpy_nt(stampformat, cfg.dformat, 64);
		limit = cfg.listdays;
		retention = cfg.dailydays;
	} else if (strcmp(listname, "month") == 0) {
		listtype = LT_Month;
		strncpy_nt(colname, listname, 8);
		snprintf(titlename, 16, "monthly");
		strncpy_nt(stampformat, cfg.mformat, 64);
		limit = cfg.listmonths;
		retention = cfg.monthlymonths;
	} else if (strcmp(listname, "year") == 0) {
		listtype = LT_Year;
		strncpy_nt(colname, listname, 8);
		snprintf(titlename, 16, "yearly");
		strncpy_nt(stampformat, "%Y", 64);
		limit = cfg.listyears;
		retention = cfg.yearlyyears;
	} else if (strcmp(listname, "top") == 0) {
		listtype = LT_Top;
		snprintf(colname, 8, "day");
		snprintf(titlename, 16, "top days");
		strncpy_nt(stampformat, cfg.tformat, 64);
		limit = cfg.listtop;
		retention = cfg.topdayentries;
		offset = 6;
	} else if (strcmp(listname, "hour") == 0) {
		listtype = LT_Hour;
		strncpy_nt(colname, listname, 8);
		snprintf(titlename, 16, "hourly");
		strncpy_nt(stampformat, "%H:%M", 64);
		limit = cfg.listhours;
		retention = cfg.hourlydays;
	} else if (strcmp(listname, "fiveminute") == 0) {
		listtype = LT_5min;
		strncpy_nt(colname, "time", 8);
		snprintf(titlename, 16, "5 minute");
		strncpy_nt(stampformat, "%H:%M", 64);
		limit = cfg.listfivemins;
		retention = cfg.fiveminutehours;
	} else {
		return;
	}

	if (limit < 0) {
		limit = 0;
	}

	daybuff[0] = '\0';

	if (!db_getdata_range(&datalist, &datainfo, interface->name, listname, (uint32_t)limit, databegin, dataend)) {
		printf("Error: Failed to fetch %s data.\n", titlename);
		return;
	}

	if (!datalist) {
		printf("No %s data available", titlename);
		if (strlen(databegin) || strlen(dataend)) {
			printf(" for given date range");
		}
		if (retention == 0) {
			printf(". Data retention for this data type is disabled in configuration");
		}
		printf(".\n");
		return;
	}

	datalist_i = datalist;

	if (strlen(dataend) == 0 && datainfo.count > 0 && (listtype == LT_Day || listtype == LT_Month || listtype == LT_Year) && cfg.estimatevisible) {
		estimatevisible = 1;
		getestimates(&e_rx, &e_tx, listtype, interface->updated, interface->created, &datalist);
		if (cfg.estimatebarvisible && e_rx + e_tx > datainfo.max) {
			datainfo.max = e_rx + e_tx;
		}
	}

	if (listtype == LT_Top) {
		if (limit > 0 && datainfo.count < (uint32_t)limit) {
			limit = (int32_t)datainfo.count;
		}
		if (limit <= 0 || datainfo.count > 999) {
			snprintf(titlename, 16, "top");
		} else {
			snprintf(titlename, 16, "top %d", limit);
		}
		current = time(NULL);
		d = localtime(&current);
		strftime(daybuff, DATEBUFFLEN, stampformat, d);
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
	printf("  /  %s", titlename);

	if (listtype == LT_Top && (strlen(databegin))) {
		printf("  (%s -", databegin);
		if (strlen(dataend)) {
			printf(" %s)", dataend);
		} else {
			printf(">)");
		}
	}
	printf("\n\n");

	if (cfg.ostyle == 3) {
		if (listtype == LT_Top) {
			printf("    # %8s  ", colname);
		} else {
			indent(5);
			printf("%8s", colname);
		}
		printf("        rx      |     tx      |    total    |   avg. rate\n");
		if (listtype == LT_Top) {
			printf("   -----");
		} else {
			indent(5);
		}
		printf("------------------------+-------------+-------------+---------------\n");
	} else {
		if (listtype == LT_Top) {
			printf("   # %8s  ", colname);
		} else {
			printf(" %8s", colname);
		}
		printf("        rx      |     tx      |    total\n");
		if (listtype == LT_Top) {
			printf("------");
		}
		printf("-------------------------+-------------+------------");
		if (cfg.ostyle != 0) {
			printf("---------------------");
			if (listtype != LT_Top) {
				printf("------");
			}
		}
		printf("\n");
	}

	while (datalist_i != NULL) {
		d = localtime(&datalist_i->timestamp);

		if (listtype == LT_Hour || listtype == LT_5min) {
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
			if (listtype != LT_Top) {
				indent(3);
			}
		}

		if (listtype == LT_Top) {
			if (strcmp(daybuff, datebuff) == 0) {
				printf("> %2d  ", i);
			} else {
				printf("  %2d  ", i);
			}
		}

		if (strlen(datebuff) <= 9 && listtype != LT_Top) {
			printf(" %*s   %s", getpadding(9, datebuff), datebuff, getvalue(datalist_i->rx, 11, RT_Normal));
		} else {
			printf(" %-*s %s", getpadding(11, datebuff), datebuff, getvalue(datalist_i->rx, 11, RT_Normal));
		}
		printf(" | %s", getvalue(datalist_i->tx, 11, RT_Normal));
		printf(" | %s", getvalue(datalist_i->rx + datalist_i->tx, 11, RT_Normal));
		if (cfg.ostyle == 3) {
			if (datalist_i->next == NULL && issametimeslot(listtype, datalist_i->timestamp, interface->updated)) {
				e_secs = getperiodseconds(listtype, datalist_i->timestamp, interface->updated, interface->created, 1);
			} else {
				e_secs = getperiodseconds(listtype, datalist_i->timestamp, interface->updated, interface->created, 0);
			}
			printf(" | %s", gettrafficrate(datalist_i->rx + datalist_i->tx, (time_t)e_secs, 14));
		} else if (cfg.ostyle != 0) {
			showbar(datalist_i->rx, datalist_i->tx, datainfo.max, 24 - offset);
		}
		printf("\n");
		if (datalist_i->next == NULL) {
			break;
		}
		datalist_i = datalist_i->next;
		i++;
	}
	if (datainfo.count == 0) {
		if (cfg.ostyle != 3) {
			printf("                        no data available\n");
		} else {
			printf("                            no data available\n");
		}
	}
	if (cfg.ostyle == 3) {
		if (listtype == LT_Top) {
			printf("   -----");
		} else {
			indent(5);
		}
		printf("------------------------+-------------+-------------+---------------\n");
	} else {
		if (listtype == LT_Top) {
			printf("------");
		}
		printf("-------------------------+-------------+------------");
		if (cfg.ostyle != 0) {
			printf("---------------------");
			if (listtype != LT_Top) {
				printf("------");
			}
		}
		printf("\n");
	}

	/* estimate or sum visible */
	if ( (estimatevisible) ||
	     (strlen(dataend) > 0 && datainfo.count > 1 && listtype != LT_Top) ) {

		if (cfg.ostyle == 3) {
			printf("    ");
		}
		if (strlen(dataend) == 0) {
			if (strlen(datebuff) <= 9) {
				printf(" %9s ", cfg.estimatetext);
			} else {
				printf("  %9s", cfg.estimatetext);
			}
			printf("  %s", getvalue(e_rx, 11, RT_Estimate));
			printf(" | %s", getvalue(e_tx, 11, RT_Estimate));
			printf(" | %s", getvalue(e_rx + e_tx, 11, RT_Estimate));
		} else {
			if (datainfo.count < 100) {
				snprintf(datebuff, DATEBUFFLEN, "sum of %" PRIu32 "", datainfo.count);
			} else {
				snprintf(datebuff, DATEBUFFLEN, "sum");
			}
			printf(" %9s   %s", datebuff, getvalue(datainfo.sumrx, 11, RT_Normal));
			printf(" | %s", getvalue(datainfo.sumtx, 11, RT_Normal));
			printf(" | %s", getvalue(datainfo.sumrx + datainfo.sumtx, 11, RT_Normal));
		}
		if (cfg.ostyle == 3) {
			printf(" |");
		} else if (cfg.ostyle != 0 && estimatevisible && cfg.estimatebarvisible) {
			showbar(e_rx, e_tx, datainfo.max, 24 - offset);
		}
		printf("\n");
	}

	dbdatalistfree(&datalist);
	timeused_debug(__func__, 0);
}

void showoneline(const interfaceinfo *interface)
{
	struct tm *d;
	char daytemp[DATEBUFFLEN];
	uint64_t div;
	dbdatalist *datalist = NULL;
	dbdatalistinfo datainfo;

	timeused_debug(__func__, 1);

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

	if (datainfo.count > 0) {
		d = localtime(&datalist->timestamp);
		strftime(daytemp, DATEBUFFLEN, cfg.dformat, d);
		printf("%s;", daytemp);

		/* daily */
		if (cfg.ostyle == 4) {
			printf("%" PRIu64 ";", datalist->rx);
			printf("%" PRIu64 ";", datalist->tx);
			printf("%" PRIu64 ";", datalist->rx + datalist->tx);
			div = getperiodseconds(LT_Day, datalist->timestamp, interface->updated, interface->created, 1);
			if (!div) {
				div = 1;
			}
			printf("%" PRIu64 ";", (datalist->rx + datalist->tx) / div);
		} else {
			printf("%s;", getvalue(datalist->rx, 1, RT_Normal));
			printf("%s;", getvalue(datalist->tx, 1, RT_Normal));
			printf("%s;", getvalue(datalist->rx + datalist->tx, 1, RT_Normal));
			printf("%s;", gettrafficrate(datalist->rx + datalist->tx, (time_t)getperiodseconds(LT_Day, datalist->timestamp, interface->updated, interface->created, 1), 1));
		}
	} else {
		printf(";;;;;");
	}
	dbdatalistfree(&datalist);

	if (!db_getdata(&datalist, &datainfo, interface->name, "month", 1)) {
		printf("\nError: Failed to fetch month data.\n");
		return;
	}

	if (datainfo.count > 0) {
		d = localtime(&datalist->timestamp);
		strftime(daytemp, DATEBUFFLEN, cfg.mformat, d);
		printf("%s;", daytemp);

		/* monthly */
		if (cfg.ostyle == 4) {
			printf("%" PRIu64 ";", datalist->rx);
			printf("%" PRIu64 ";", datalist->tx);
			printf("%" PRIu64 ";", datalist->rx + datalist->tx);
			div = getperiodseconds(LT_Month, datalist->timestamp, interface->updated, interface->created, 1);
			if (!div) {
				div = 1;
			}
			printf("%" PRIu64 ";", (datalist->rx + datalist->tx) / div);
		} else {
			printf("%s;", getvalue(datalist->rx, 1, RT_Normal));
			printf("%s;", getvalue(datalist->tx, 1, RT_Normal));
			printf("%s;", getvalue(datalist->rx + datalist->tx, 1, RT_Normal));
			printf("%s;", gettrafficrate(datalist->rx + datalist->tx, (time_t)getperiodseconds(LT_Month, datalist->timestamp, interface->updated, interface->created, 1), 1));
		}
	} else {
		printf(";;;;;");
	}
	dbdatalistfree(&datalist);

	/* all time total */
	if (cfg.ostyle == 4) {
		printf("%" PRIu64 ";", interface->rxtotal);
		printf("%" PRIu64 ";", interface->txtotal);
		printf("%" PRIu64 "\n", interface->rxtotal + interface->txtotal);
	} else {
		printf("%s;", getvalue(interface->rxtotal, 1, RT_Normal));
		printf("%s;", getvalue(interface->txtotal, 1, RT_Normal));
		printf("%s\n", getvalue(interface->rxtotal + interface->txtotal, 1, RT_Normal));
	}
	timeused_debug(__func__, 0);
}

void showhours(const interfaceinfo *interface)
{
	int i, s = 0, hour, minute, declen = cfg.hourlydecimals, div = 1;
	unsigned int j, k, tmax = 0, dots = 0;
	uint64_t max = 1;
	char matrix[HGLINES][81]; /* width is one over 80 so that snprintf can write the end char */
	char unit[4];
	struct tm *d;
	dbdatalist *datalist = NULL, *datalist_i = NULL;
	dbdatalistinfo datainfo;
	HOURDATA hourdata[24];

	timeused_debug(__func__, 1);

	for (i = 0; i < 24; i++) {
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
		if (hourdata[d->tm_hour].date != 0 || interface->updated - datalist_i->timestamp > 86400) {
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

	for (i = 0; i < 24; i++) {
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
	for (i = 0; i < HGLINES; i++) {
		for (j = 0; j < 81; j++) {
			matrix[i][j] = ' ';
		}
	}

	/* unit selection */
	while ((double)max / (pow(1024, div)) >= 100 && div < UNITPREFIXCOUNT) {
		div++;
	}
	strncpy_nt(unit, getunitprefix(div), 4);
	div = (int)(pow(1024, div - 1));
	if (div == 1) {
		declen = 0;
	}

	/* structure */
	snprintf(matrix[11], 81, " -+--------------------------------------------------------------------------->");
	for (i = 0; i < 3; i++) {
		snprintf(matrix[14] + (i * 28), 14, " h %*srx (%s)", 1 + cfg.unitmode, " ", unit);
		snprintf(matrix[14] + (i * 28) + 15 + cfg.unitmode, 10, "tx (%s)", unit);
	}

	for (i = 10; i > 1; i--)
		matrix[i][2] = '|';

	matrix[1][2] = '^';
	matrix[12][2] = '|';

	/* title */
	if (strcmp(interface->name, interface->alias) == 0 || strlen(interface->alias) == 0) {
		i = snprintf(matrix[0], 81, " %s", interface->name);
	} else {
		i = snprintf(matrix[0], 81, " %s (%s)", interface->alias, interface->name);
	}
	if (interface->active == 0) {
		snprintf(matrix[0] + i + 1, 81, " [disabled]");
	}

	/* time to the corner */
	snprintf(matrix[0] + 74, 7, "%02d:%02d", hour, minute);

	/* numbers under x-axis and graphics :) */
	k = 5;
	for (i = 23; i >= 0; i--) {
		s = (int)tmax - i;
		if (s < 0)
			s += 24;

		snprintf(matrix[12] + k, 81 - k, "%02d ", s);

		dots = (unsigned int)(10 * ((double)hourdata[s].rx / (double)max));
		for (j = 0; j < dots; j++)
			matrix[10 - j][k] = cfg.rxhourchar[0];

		dots = (unsigned int)(10 * ((double)hourdata[s].tx / (double)max));
		for (j = 0; j < dots; j++)
			matrix[10 - j][k + 1] = cfg.txhourchar[0];

		k = k + 3;
	}

	/* section separators */
	if (cfg.hourlystyle == 1) {
		matrix[14][26] = '|';
		matrix[14][54] = '|';
	} else if (cfg.hourlystyle == 2) {
		matrix[14][25] = ']';
		matrix[14][26] = '[';
		matrix[14][53] = ']';
		matrix[14][54] = '[';
	} else if (cfg.hourlystyle == 3) {
		matrix[14][26] = '[';
		matrix[14][53] = ']';
	}

	/* clean \0 */
	for (i = 0; i < HGLINES; i++) {
		for (j = 0; j < 80; j++) {
			if (matrix[i][j] == '\0') {
				matrix[i][j] = ' ';
			}
		}
	}

	/* show matrix */
	for (i = 0; i < HGLINES; i++) {
		for (j = 0; j < 80; j++) {
			printf("%c", matrix[i][j]);
		}
		printf("\n");
	}

	/* hours and traffic */
	for (i = 0; i <= 7; i++) {
		s = (int)tmax + i + 1;
		for (j = 0; j < 3; j++) {
			printf("%02d %" DECCONV "10.*f %" DECCONV "10.*f", ((unsigned int)s + (j * 8)) % 24,
					 declen, (double)hourdata[((unsigned int)s + (j * 8)) % 24].rx / (double)div,
					 declen, (double)hourdata[((unsigned int)s + (j * 8)) % 24].tx / (double)div);

			if (j < 2) {
				if (cfg.hourlystyle == 1) {
					printf("  | ");
				} else if (cfg.hourlystyle == 2) {
					printf(" ][ ");
				} else if (cfg.hourlystyle == 3) {
					if (j == 0) {
						printf("  [ ");
					} else {
						printf(" ]  ");
					}
				} else {
					printf("    ");
				}
			}
		}
		printf("\n");
	}

	timeused_debug(__func__, 0);
}

// TODO: json? needs new function due to different format
// TODO: xml? needs new function due to different format
// TODO: alert needs new function due to different format
// TODO: image? output below doesn't have much graphics potential, line in 5 minute graph could be one alternative
// TODO: tests
void show95thpercentile(const interfaceinfo *interface)
{
	struct tm *d;
	char datebuff[DATEBUFFLEN];
	percentiledata pdata;

	timeused_debug(__func__, 1);

	if (cfg.fiveminutehours < 744) {
		printf("\nWarning: Configuration \"5MinuteHours\" needs to be at least 744 for 100%% coverage.\n");
		printf("         \"5MinuteHours\" is currently set at %d.\n\n", cfg.fiveminutehours);
	}

	if (!getpercentiledata(&pdata, interface->name)) {
		exit(EXIT_FAILURE);
	}

	printf("\n");
	indent(1);
	if (strcmp(interface->name, interface->alias) == 0 || strlen(interface->alias) == 0) {
		printf("%s", interface->name);
	} else {
		printf("%s (%s)", interface->alias, interface->name);
	}
	if (interface->active == 0) {
		printf(" [disabled]");
	}
	printf("  /  95th percentile\n\n");

	indent(1);
	d = localtime(&pdata.monthbegin);
	strftime(datebuff, DATEBUFFLEN, DATETIMEFORMATWITHOUTSECS, d);
	printf("%s", datebuff);
	d = localtime(&pdata.dataend);
	strftime(datebuff, DATEBUFFLEN, DATETIMEFORMATWITHOUTSECS, d);
	printf(" - %s", datebuff);
	if (cfg.ostyle == 0) {
		printf("\n");
	} else {
		indent(1);
	}
	printf("(%" PRIu32 " entries", pdata.count);
	if (pdata.count == pdata.countexpectation) {
		printf(", 100%% coverage)");
	} else {
		printf(", %0.1f%% coverage)", (float)pdata.count / (float)pdata.countexpectation * 100.0);
	}
	printf("\n\n");

	indent(7);
	printf("                 rx       |       tx       |     total\n");
	indent(7);
	printf("--------------------------+----------------+---------------\n");
	indent(7);
	printf("minimum   %s |", gettrafficrate(pdata.minrx, 300, 15));
	printf("%s |", gettrafficrate(pdata.mintx, 300, 15));
	printf("%s\n", gettrafficrate(pdata.min, 300, 15));
	indent(7);
	printf("average   %s |", gettrafficrate(pdata.sumrx, (time_t)(pdata.count * 300), 15));
	printf("%s |", gettrafficrate(pdata.sumtx, (time_t)(pdata.count * 300), 15));
	printf("%s\n", gettrafficrate(pdata.sumrx + pdata.sumtx, (time_t)(pdata.count * 300), 15));
	indent(7);
	printf("maximum   %s |", gettrafficrate(pdata.maxrx, 300, 15));
	printf("%s |", gettrafficrate(pdata.maxtx, 300, 15));
	printf("%s\n", gettrafficrate(pdata.max, 300, 15));
	indent(7);
	printf("--------------------------+----------------+---------------\n");
	indent(7);
	printf(" 95th %%   %s |", gettrafficrate(pdata.rxpercentile, 300, 15));
	printf("%s |", gettrafficrate(pdata.txpercentile, 300, 15));
	printf("%s\n", gettrafficrate(pdata.sumpercentile, 300, 15));

	timeused_debug(__func__, 0);
}

int showbar(const uint64_t rx, const uint64_t tx, const uint64_t max, const int len)
{
	int i, l, width = len;

	if ((rx + tx) < max) {
		width = (int)(((double)(rx + tx) / (double)max) * len);
	} else if ((rx + tx) > max || max == 0) {
		return 0;
	}

	if (width <= 0) {
		return 0;
	}

	printf("  ");

	if (tx > rx) {
		l = (int)lrint(((double)rx / (double)(rx + tx) * width));

		for (i = 0; i < l; i++) {
			printf("%c", cfg.rxchar[0]);
		}
		for (i = 0; i < (width - l); i++) {
			printf("%c", cfg.txchar[0]);
		}
	} else {
		l = (int)lrint(((double)tx / (double)(rx + tx) * width));

		for (i = 0; i < (width - l); i++) {
			printf("%c", cfg.rxchar[0]);
		}
		for (i = 0; i < l; i++) {
			printf("%c", cfg.txchar[0]);
		}
	}
	return width;
}

int showalert(const char *interface, const AlertOutput output, const AlertExit exit, const AlertType type, const AlertCondition condition, const uint64_t limit)
{
	interfaceinfo ifaceinfo;
	int i, l, ret = 0, limitexceeded = 0, estimateexceeded = 0;
	short ongoing = 1;
	double percentage = 0.0;
	char tablename[6], typeoutput[8], conditionname[16];
	char datebuff[DATEBUFFLEN];
	ListType listtype = LT_None;
	uint64_t bytes = 0, e_rx = 0, e_tx = 0, e_bytes = 0, periodseconds = 0;
	dbdatalist *datalist = NULL;
	dbdatalistinfo datainfo;

	timeused_debug(__func__, 1);

	if (!db_getinterfaceinfo(interface, &ifaceinfo)) {
		return 1;
	}

	switch (type) {
		case AT_None:
			return 0;
		case AT_Hour:
			listtype = LT_Hour;
			snprintf(tablename, 6, "hour");
			snprintf(typeoutput, 8, "hourly");
			break;
		case AT_Day:
			listtype = LT_Day;
			snprintf(tablename, 6, "day");
			snprintf(typeoutput, 8, "daily");
			break;
		case AT_Month:
			listtype = LT_Month;
			snprintf(tablename, 6, "month");
			snprintf(typeoutput, 8, "monthly");
			break;
		case AT_Year:
			listtype = LT_Year;
			snprintf(tablename, 6, "year");
			snprintf(typeoutput, 8, "yearly");
			break;
	}

	if (!db_getdata(&datalist, &datainfo, interface, tablename, 1)) {
		printf("Error: Failed to fetch %s data for interface %s.\n", tablename, interface);
		return 1;
	}

	if (!datalist) {
		printf("Error: No %s data available for interface %s.\n", tablename, interface);
		return 1;
	}

	switch (condition) {
		case AC_None:
			return 0;
		case AC_RX:
			bytes = datalist->rx;
			snprintf(conditionname, 16, "rx");
			if (cfg.estimatevisible) {
				getestimates(&e_rx, &e_tx, listtype, ifaceinfo.updated, ifaceinfo.created, &datalist);
				e_bytes = e_rx;
			}
			break;
		case AC_TX:
			bytes = datalist->tx;
			snprintf(conditionname, 16, "tx");
			if (cfg.estimatevisible) {
				getestimates(&e_rx, &e_tx, listtype, ifaceinfo.updated, ifaceinfo.created, &datalist);
				e_bytes = e_tx;
			}
			break;
		case AC_Total:
			bytes = datalist->rx + datalist->tx;
			snprintf(conditionname, 16, "total");
			if (cfg.estimatevisible) {
				getestimates(&e_rx, &e_tx, listtype, ifaceinfo.updated, ifaceinfo.created, &datalist);
				e_bytes = e_rx + e_tx;
			}
			break;
		case AC_RX_Estimate:
			ongoing = 0;
			getestimates(&e_rx, &e_tx, listtype, ifaceinfo.updated, ifaceinfo.created, &datalist);
			bytes = e_rx;
			snprintf(conditionname, 16, "rx estimate");
			break;
		case AC_TX_Estimate:
			ongoing = 0;
			getestimates(&e_rx, &e_tx, listtype, ifaceinfo.updated, ifaceinfo.created, &datalist);
			bytes = e_tx;
			snprintf(conditionname, 16, "tx estimate");
			break;
		case AC_Total_Estimate:
			ongoing = 0;
			getestimates(&e_rx, &e_tx, listtype, ifaceinfo.updated, ifaceinfo.created, &datalist);
			bytes = e_rx + e_tx;
			snprintf(conditionname, 16, "total estimate");
			break;
	}

	if (bytes > limit) {
		limitexceeded = 1;
	}

	if (ongoing == 1 && e_bytes > limit) {
		estimateexceeded = 1;
	}

	if (limitexceeded && exit == AE_Exit_1_On_Limit) {
		ret = 1;
	} else if (estimateexceeded && exit == AE_Exit_1_On_Estimate) {
		ret = 1;
	}

	if (output != AO_No_Output) {
		if (output == AO_Always_Output || (output == AO_Output_On_Estimate && estimateexceeded) || ((output == AO_Output_On_Limit || output == AO_Output_On_Estimate) && limitexceeded)) {
			if (strlen(ifaceinfo.alias)) {
				printf("\n   %s (%s)", ifaceinfo.alias, ifaceinfo.name);
			} else {
				printf("\n   %s", interface);
			}
			if (ifaceinfo.updated) {
				strftime(datebuff, DATEBUFFLEN, DATETIMEFORMAT, localtime(&ifaceinfo.updated));
				printf(" at %s", datebuff);
			}
			if (datalist->timestamp) {
				printf(" for %s ", tablename);
				switch (type) {
					case AT_None:
						break;
					case AT_Hour:
						strftime(datebuff, DATEBUFFLEN, "%H", localtime(&datalist->timestamp));
						printf("%s of ", datebuff);
						strftime(datebuff, DATEBUFFLEN, cfg.dformat, localtime(&datalist->timestamp));
						printf("%s", datebuff);
						break;
					case AT_Day:
						strftime(datebuff, DATEBUFFLEN, cfg.dformat, localtime(&datalist->timestamp));
						printf("%s", datebuff);
						break;
					case AT_Month:
						strftime(datebuff, DATEBUFFLEN, cfg.mformat, localtime(&datalist->timestamp));
						printf("%s", datebuff);
						break;
					case AT_Year:
						strftime(datebuff, DATEBUFFLEN, "%Y", localtime(&datalist->timestamp));
						printf("%s", datebuff);
						break;
				}
			}
			printf("\n\n");
		}

		if ((output == AO_Always_Output || output == AO_Output_On_Limit || output == AO_Output_On_Estimate) && limitexceeded) {
			printf("                          Alert limit exceeded!\n\n");
		} else if (output == AO_Always_Output || (output == AO_Output_On_Estimate && estimateexceeded)) {
			if (estimateexceeded) {
				printf("             Warning: Limit will be exceeded at current rate\n\n");
			}
			printf("     [");
			l = (int)lrint((double)(bytes) / (double)limit * ALERTUSAGELEN);
			if (l > ALERTUSAGELEN) {
				l = ALERTUSAGELEN;
			}
			for (i = 0; i < l; i++) {
				printf("=");
			}
			if (ongoing && cfg.estimatevisible) {
				if (!estimateexceeded) {
					l = (int)lrint((double)(e_bytes) / (double)limit * ALERTUSAGELEN);
					for (; i < l; i++) {
						printf("-");
					}
				} else {
					for (; i < ALERTUSAGELEN; i++) {
						printf("-");
					}
				}
			}
			for (; i < ALERTUSAGELEN; i++) {
				printf(".");
			}
			printf("]\n\n");
		}

		if (output == AO_Always_Output || (output == AO_Output_On_Estimate && estimateexceeded) || ((output == AO_Output_On_Limit || output == AO_Output_On_Estimate) && limitexceeded)) {
			printf("      %8s |", typeoutput);
			if (ongoing) {
				printf("   %9s      |", conditionname);
			} else {
				printf("  %14s  |", conditionname);
			}
			percentage = (double)(bytes) / (double)limit * 100.0;
			printf("   percentage   |   avg. rate\n");
			printf("     ----------+------------------+----------------+--------------\n");
			printf("          used | %16s |", getvalue(bytes, 16, RT_Normal));
			periodseconds = getperiodseconds(listtype, datalist->timestamp, ifaceinfo.updated, ifaceinfo.created, ongoing);
			if (ongoing && periodseconds == 0) {
				periodseconds = getperiodseconds(listtype, datalist->timestamp, ifaceinfo.updated, ifaceinfo.created, 0);
			}
			if (percentage <= 100000.0) {
				printf(" %13.1f%% | %13s\n", percentage, gettrafficrate(bytes, (time_t)periodseconds, 13));
			} else {
				printf(" %14s | %13s\n", ">100000%", gettrafficrate(bytes, (time_t)periodseconds, 13));
			}
			printf("         limit | %16s |", getvalue(limit, 16, RT_Normal));
			printf("                | %13s\n", gettrafficrate(limit, (time_t)getperiodseconds(listtype, datalist->timestamp, ifaceinfo.updated, ifaceinfo.created, 0), 13));

			if (limitexceeded) {
				printf("        excess | %16s |                |\n", getvalue(bytes - limit, 16, RT_Normal));
				printf("     ----------+------------------+----------------+--------------\n");
			} else {
				printf("     remaining | %16s | %13.1f%% |\n", getvalue(limit - bytes, 16, RT_Normal), 100.0 - percentage);
				printf("     ----------+------------------+----------------+--------------\n");
			}
			if (ongoing && cfg.estimatevisible && e_bytes > 0) {
				printf("     %9s | %16s |", cfg.estimatetext, getvalue(e_bytes, 16, RT_Normal));
				percentage = (double)(e_bytes) / (double)limit * 100.0;
				if (percentage <= 100000.0) {
					printf(" %13.1f%%", percentage);
				} else {
					printf(" %14s", ">100000%");
				}
				if (e_bytes > limit) {
					printf(", +%s\n", getvalue(e_bytes - limit, 0, RT_Normal));
				} else {
					/* rate for estimated is always to same as for used so "bytes" intentionally used here instead of "e_bytes" */
					printf(" | %13s\n", gettrafficrate(bytes, (time_t)periodseconds, 13));
				}
			}
		}
	}

	dbdatalistfree(&datalist);

	timeused_debug(__func__, 0);

	return ret;
}
