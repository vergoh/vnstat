#include "common.h"
#include "dbsql.h"
#include "dbjson.h"
#include "dbshow.h"

void showdb(const char *interface, int qmode, const char *databegin, const char *dataend)
{
	interfaceinfo ifaceinfo;
	char timestamp[22];

	if (!db_getinterfacecountbyname(interface)) {
		exit(EXIT_FAILURE);
	}

	if (!db_getinterfaceinfo(interface, &ifaceinfo)) {
		exit(EXIT_FAILURE);
	}

	if (ifaceinfo.created == ifaceinfo.updated) {
		strftime(timestamp, 22, DATETIMEFORMAT, localtime(&ifaceinfo.updated));
		printf(" %s: No data. Timestamp of last update is same %s as of database creation.\n", interface, timestamp);
		return;
	}

	switch (qmode) {
		case 0:
			showsummary(&ifaceinfo, 0);
			break;
		case 1:
			showlist(&ifaceinfo, "day", databegin, dataend);
			break;
		case 2:
			showlist(&ifaceinfo, "month", databegin, dataend);
			break;
		case 3:
			showlist(&ifaceinfo, "top", databegin, dataend);
			break;
		case 4:
			showsummary(&ifaceinfo, 0);
			break;
		case 5:
			showsummary(&ifaceinfo, 1);
			break;
		case 6:
			showlist(&ifaceinfo, "year", databegin, dataend);
			break;
		case 7:
			showhours(&ifaceinfo);
			break;
		case 9:
			showoneline(&ifaceinfo);
			break;
		case 11:
			showlist(&ifaceinfo, "hour", databegin, dataend);
			break;
		case 12:
			showlist(&ifaceinfo, "fiveminute", databegin, dataend);
			break;
		case 13:
			show95thpercentile(&ifaceinfo);
			break;
		default:
			printf("Error: Not such query mode: %d\n", qmode);
			exit(EXIT_FAILURE);
	}
}

void showsummary(const interfaceinfo *ifaceinfo, const int shortmode)
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

	if (ifaceinfo->updated && !shortmode) {
		strftime(datebuff, DATEBUFFLEN, DATETIMEFORMAT, localtime(&ifaceinfo->updated));
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
	if (strcmp(ifaceinfo->name, ifaceinfo->alias) == 0 || strlen(ifaceinfo->alias) == 0) {
		printf("%s", ifaceinfo->name);
	} else {
		printf("%s (%s)", ifaceinfo->alias, ifaceinfo->name);
	}
	if (ifaceinfo->active == 0) {
		printf(" [disabled]");
	}
	if (shortmode) {
		printf(":\n");
	} else {
		/* get formatted date for creation date */
		d = localtime(&ifaceinfo->created);
		strftime(datebuff, DATEBUFFLEN, cfg.tformat, d);
		printf(" since %s\n\n", datebuff);

		if (cfg.ostyle == 1) {
			indent(3);
		} else {
			indent(10);
		}
		printf("rx:  %s", getvalue(ifaceinfo->rxtotal, 1, RT_Normal));
		indent(3);
		printf("   tx:  %s", getvalue(ifaceinfo->txtotal, 1, RT_Normal));
		indent(3);
		printf("   total:  %s\n\n", getvalue(ifaceinfo->rxtotal + ifaceinfo->txtotal, 1, RT_Normal));

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

	if (!db_getdata(&datalist, &datainfo, ifaceinfo->name, "month", 2)) {
		printf("Error: Failed to fetch month data.\n");
		exit(EXIT_FAILURE);
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
		if (datalist_i->next == NULL && issametimeslot(LT_Month, datalist_i->timestamp, ifaceinfo->updated)) {
			if (datalist_i->rx == 0 || datalist_i->tx == 0 || (ifaceinfo->updated - datalist_i->timestamp) == 0) {
				e_rx = e_tx = 0;
			} else if (cfg.estimatevisible) {
				getestimates(&e_rx, &e_tx, LT_Month, ifaceinfo->updated, ifaceinfo->created, &datalist);
			}
			if (shortmode && cfg.ostyle != 0 && cfg.estimatevisible) {
				printf("%s%s", fieldseparator, getvalue(e_rx + e_tx, 11, RT_Estimate));
			} else if (!shortmode && cfg.ostyle >= 2) {
				printf("%s%s", fieldseparator, gettrafficrate(datalist_i->rx + datalist_i->tx, (time_t)getperiodseconds(LT_Month, datalist_i->timestamp, ifaceinfo->updated, ifaceinfo->created, 1), 14));
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

	if (!db_getdata(&datalist, &datainfo, ifaceinfo->name, "day", 2)) {
		printf("Error: Failed to fetch day data.\n");
		exit(EXIT_FAILURE);
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
		if (datalist_i->next == NULL && issametimeslot(LT_Day, datalist_i->timestamp, ifaceinfo->updated)) {
			d = localtime(&ifaceinfo->updated);
			if (datalist_i->rx == 0 || datalist_i->tx == 0 || (d->tm_hour * 60 + d->tm_min) == 0) {
				e_rx = e_tx = 0;
			} else if (cfg.estimatevisible) {
					e_rx = (uint64_t)((double)datalist_i->rx / (double)(d->tm_hour * 60 + d->tm_min)) * 1440;
					e_tx = (uint64_t)((double)datalist_i->tx / (double)(d->tm_hour * 60 + d->tm_min)) * 1440;
			}
			if (shortmode && cfg.ostyle != 0 && cfg.estimatevisible) {
				printf("%s%s", fieldseparator, getvalue(e_rx + e_tx, 11, RT_Estimate));
			} else if (!shortmode && cfg.ostyle >= 2) {
				printf("%s%s", fieldseparator, gettrafficrate(datalist_i->rx + datalist_i->tx, (time_t)getperiodseconds(LT_Day, datalist_i->timestamp, ifaceinfo->updated, ifaceinfo->created, 1), 14));
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

void showlist(const interfaceinfo *ifaceinfo, const char *listname, const char *databegin, const char *dataend)
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

	if (!db_getdata_range(&datalist, &datainfo, ifaceinfo->name, listname, (uint32_t)limit, databegin, dataend)) {
		printf("Error: Failed to fetch %s data.\n", titlename);
		exit(EXIT_FAILURE);
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
		getestimates(&e_rx, &e_tx, listtype, ifaceinfo->updated, ifaceinfo->created, &datalist);
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
	if (strcmp(ifaceinfo->name, ifaceinfo->alias) == 0 || strlen(ifaceinfo->alias) == 0) {
		printf(" %s", ifaceinfo->name);
	} else {
		printf(" %s (%s)", ifaceinfo->alias, ifaceinfo->name);
	}
	if (ifaceinfo->active == 0) {
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
			if (datalist_i->next == NULL && issametimeslot(listtype, datalist_i->timestamp, ifaceinfo->updated)) {
				e_secs = getperiodseconds(listtype, datalist_i->timestamp, ifaceinfo->updated, ifaceinfo->created, 1);
			} else {
				e_secs = getperiodseconds(listtype, datalist_i->timestamp, ifaceinfo->updated, ifaceinfo->created, 0);
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

void showoneline(const interfaceinfo *ifaceinfo)
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
	if (strcmp(ifaceinfo->name, ifaceinfo->alias) == 0 || strlen(ifaceinfo->alias) == 0) {
		printf("%s", ifaceinfo->name);
	} else {
		printf("%s (%s)", ifaceinfo->alias, ifaceinfo->name);
	}
	if (ifaceinfo->active == 0) {
		printf(" [disabled]");
	}
	printf(";");

	if (!db_getdata(&datalist, &datainfo, ifaceinfo->name, "day", 1)) {
		printf("\nError: Failed to fetch day data.\n");
		exit(EXIT_FAILURE);
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
			div = getperiodseconds(LT_Day, datalist->timestamp, ifaceinfo->updated, ifaceinfo->created, 1);
			if (!div) {
				div = 1;
			}
			printf("%" PRIu64 ";", (datalist->rx + datalist->tx) / div);
		} else {
			printf("%s;", getvalue(datalist->rx, 1, RT_Normal));
			printf("%s;", getvalue(datalist->tx, 1, RT_Normal));
			printf("%s;", getvalue(datalist->rx + datalist->tx, 1, RT_Normal));
			printf("%s;", gettrafficrate(datalist->rx + datalist->tx, (time_t)getperiodseconds(LT_Day, datalist->timestamp, ifaceinfo->updated, ifaceinfo->created, 1), 1));
		}
	} else {
		printf(";;;;;");
	}
	dbdatalistfree(&datalist);

	if (!db_getdata(&datalist, &datainfo, ifaceinfo->name, "month", 1)) {
		printf("\nError: Failed to fetch month data.\n");
		exit(EXIT_FAILURE);
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
			div = getperiodseconds(LT_Month, datalist->timestamp, ifaceinfo->updated, ifaceinfo->created, 1);
			if (!div) {
				div = 1;
			}
			printf("%" PRIu64 ";", (datalist->rx + datalist->tx) / div);
		} else {
			printf("%s;", getvalue(datalist->rx, 1, RT_Normal));
			printf("%s;", getvalue(datalist->tx, 1, RT_Normal));
			printf("%s;", getvalue(datalist->rx + datalist->tx, 1, RT_Normal));
			printf("%s;", gettrafficrate(datalist->rx + datalist->tx, (time_t)getperiodseconds(LT_Month, datalist->timestamp, ifaceinfo->updated, ifaceinfo->created, 1), 1));
		}
	} else {
		printf(";;;;;");
	}
	dbdatalistfree(&datalist);

	/* all time total */
	if (cfg.ostyle == 4) {
		printf("%" PRIu64 ";", ifaceinfo->rxtotal);
		printf("%" PRIu64 ";", ifaceinfo->txtotal);
		printf("%" PRIu64 "\n", ifaceinfo->rxtotal + ifaceinfo->txtotal);
	} else {
		printf("%s;", getvalue(ifaceinfo->rxtotal, 1, RT_Normal));
		printf("%s;", getvalue(ifaceinfo->txtotal, 1, RT_Normal));
		printf("%s\n", getvalue(ifaceinfo->rxtotal + ifaceinfo->txtotal, 1, RT_Normal));
	}
	timeused_debug(__func__, 0);
}

void showhours(const interfaceinfo *ifaceinfo)
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

	if (!db_getdata(&datalist, &datainfo, ifaceinfo->name, "hour", 24)) {
		printf("Error: Failed to fetch hour data.\n");
		exit(EXIT_FAILURE);
	}

	if (datainfo.count == 0) {
		return;
	}

	datalist_i = datalist;

	while (datalist_i != NULL) {
		d = localtime(&datalist_i->timestamp);
		if (hourdata[d->tm_hour].date != 0 || ifaceinfo->updated - datalist_i->timestamp > 86400) {
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

	d = localtime(&ifaceinfo->updated);
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
	if (strcmp(ifaceinfo->name, ifaceinfo->alias) == 0 || strlen(ifaceinfo->alias) == 0) {
		i = snprintf(matrix[0], 81, " %s", ifaceinfo->name);
	} else {
		i = snprintf(matrix[0], 81, " %s (%s)", ifaceinfo->alias, ifaceinfo->name);
	}
	if (ifaceinfo->active == 0) {
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

void show95thpercentile(const interfaceinfo *ifaceinfo)
{
	struct tm *d;
	char datebuff[DATEBUFFLEN];
	percentiledata pdata;

	timeused_debug(__func__, 1);

	if (cfg.fiveminutehours < PERCENTILEENTRYCOUNT) {
		printf("\nWarning: Configuration \"5MinuteHours\" needs to be at least %d for 100%% coverage.\n", PERCENTILEENTRYCOUNT);
		printf("         \"5MinuteHours\" is currently set at %d.\n\n", cfg.fiveminutehours);
	}

	if (!getpercentiledata(&pdata, ifaceinfo->name, 0)) {
		exit(EXIT_FAILURE);
	}

	printf("\n");
	indent(1);
	if (strcmp(ifaceinfo->name, ifaceinfo->alias) == 0 || strlen(ifaceinfo->alias) == 0) {
		printf("%s", ifaceinfo->name);
	} else {
		printf("%s (%s)", ifaceinfo->alias, ifaceinfo->name);
	}
	if (ifaceinfo->active == 0) {
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

	showpercentiledatatable(&pdata, 7, 1);

	timeused_debug(__func__, 0);
}

void showpercentiledatatable(const percentiledata *pdata, const int indentation, const int visible95th)
{
	indent(indentation);
	printf("                   rx       |       tx       |     total\n");
	indent(indentation);
	printf("----------------------------+----------------+---------------\n");
	indent(indentation);
	printf("minimum     %s |", gettrafficrate(pdata->minrx, 300, 15));
	printf("%s |", gettrafficrate(pdata->mintx, 300, 15));
	printf("%s\n", gettrafficrate(pdata->min, 300, 15));
	indent(indentation);
	printf("average     %s |", gettrafficrate(pdata->sumrx, (time_t)(pdata->count * 300), 15));
	printf("%s |", gettrafficrate(pdata->sumtx, (time_t)(pdata->count * 300), 15));
	printf("%s\n", gettrafficrate(pdata->sumrx + pdata->sumtx, (time_t)(pdata->count * 300), 15));
	indent(indentation);
	printf("maximum     %s |", gettrafficrate(pdata->maxrx, 300, 15));
	printf("%s |", gettrafficrate(pdata->maxtx, 300, 15));
	printf("%s\n", gettrafficrate(pdata->max, 300, 15));
	indent(indentation);
	printf("----------------------------+----------------+---------------\n");
	if (visible95th) {
		indent(indentation);
		printf(" 95th %%     %s |", gettrafficrate(pdata->rxpercentile, 300, 15));
		printf("%s |", gettrafficrate(pdata->txpercentile, 300, 15));
		printf("%s\n", gettrafficrate(pdata->sumpercentile, 300, 15));
	}
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

int showalert(const char *interface, const AlertOutput output, const AlertExit aexit, const AlertType type, const AlertCondition condition, const uint64_t limit)
{
	int ret = 0;
	uint64_t e_rx = 0, e_tx = 0;
	alertdata adata;

	timeused_debug(__func__, 1);

	if (!db_getinterfaceinfo(interface, &adata.ifaceinfo)) {
		return 1;
	}

	adata.limitexceeded = 0;
	adata.estimateexceeded = 0;
	adata.ongoing = 1;
	adata.used = 0;
	adata.e_used = 0;
	adata.datalist = NULL;

	switch (type) {
		case AT_None:
			return 0;
		case AT_Hour:
			adata.listtype = LT_Hour;
			snprintf(adata.tablename, 6, "hour");
			snprintf(adata.typeoutput, 8, "hourly");
			break;
		case AT_Day:
			adata.listtype = LT_Day;
			snprintf(adata.tablename, 6, "day");
			snprintf(adata.typeoutput, 8, "daily");
			break;
		case AT_Month:
			adata.listtype = LT_Month;
			snprintf(adata.tablename, 6, "month");
			snprintf(adata.typeoutput, 8, "monthly");
			break;
		case AT_Year:
			adata.listtype = LT_Year;
			snprintf(adata.tablename, 6, "year");
			snprintf(adata.typeoutput, 8, "yearly");
			break;
		case AT_Percentile:
			break;
	}

	if (type != AT_Percentile) {
		if (!db_getdata(&adata.datalist, &adata.datainfo, interface, adata.tablename, 1)) {
			printf("Error: Failed to fetch %s data for interface %s.\n", adata.tablename, interface);
			return 1;
		}

		if (!adata.datalist) {
			printf("Error: No %s data available for interface %s.\n", adata.tablename, interface);
			return 1;
		}
		adata.timestamp = adata.datalist->timestamp;
	} else {
		if (!getpercentiledata(&adata.pdata, adata.ifaceinfo.name, limit)) {
			exit(EXIT_FAILURE);
		}
		adata.timestamp = adata.pdata.dataend;
	}

	// "limit" and "used" are always in either bytes or bytes/second
	switch (condition) {
		case AC_None:
			return 0;
		case AC_RX:
			if (type != AT_Percentile) {
				adata.used = adata.datalist->rx;
			} else {
				adata.used = (uint64_t)(adata.pdata.rxpercentile / (double)300);
			}
			snprintf(adata.conditionname, 16, "rx");
			if (type != AT_Percentile && cfg.estimatevisible) {
				getestimates(&e_rx, &e_tx, adata.listtype, adata.ifaceinfo.updated, adata.ifaceinfo.created, &adata.datalist);
				adata.e_used = e_rx;
			}
			break;
		case AC_TX:
			if (type != AT_Percentile) {
				adata.used = adata.datalist->tx;
			} else {
				adata.used = (uint64_t)(adata.pdata.txpercentile / (double)300);
			}
			snprintf(adata.conditionname, 16, "tx");
			if (type != AT_Percentile && cfg.estimatevisible) {
				getestimates(&e_rx, &e_tx, adata.listtype, adata.ifaceinfo.updated, adata.ifaceinfo.created, &adata.datalist);
				adata.e_used = e_tx;
			}
			break;
		case AC_Total:
			if (type != AT_Percentile) {
				adata.used = adata.datalist->rx + adata.datalist->tx;
			} else {
				adata.used = (uint64_t)(adata.pdata.sumpercentile / (double)300);
			}
			snprintf(adata.conditionname, 16, "total");
			if (type != AT_Percentile && cfg.estimatevisible) {
				getestimates(&e_rx, &e_tx, adata.listtype, adata.ifaceinfo.updated, adata.ifaceinfo.created, &adata.datalist);
				adata.e_used = e_rx + e_tx;
			}
			break;
		case AC_RX_Estimate:
			adata.ongoing = 0;
			getestimates(&e_rx, &e_tx, adata.listtype, adata.ifaceinfo.updated, adata.ifaceinfo.created, &adata.datalist);
			adata.used = e_rx;
			snprintf(adata.conditionname, 16, "rx estimate");
			break;
		case AC_TX_Estimate:
			adata.ongoing = 0;
			getestimates(&e_rx, &e_tx, adata.listtype, adata.ifaceinfo.updated, adata.ifaceinfo.created, &adata.datalist);
			adata.used = e_tx;
			snprintf(adata.conditionname, 16, "tx estimate");
			break;
		case AC_Total_Estimate:
			adata.ongoing = 0;
			getestimates(&e_rx, &e_tx, adata.listtype, adata.ifaceinfo.updated, adata.ifaceinfo.created, &adata.datalist);
			adata.used = e_rx + e_tx;
			snprintf(adata.conditionname, 16, "total estimate");
			break;
	}

	if (debug) {
		printf("used: %" PRIu64 "\nlimit: %" PRIu64 "\n", adata.used, limit);
	}

	if (adata.used > limit) {
		adata.limitexceeded = 1;
	}

	if (adata.ongoing == 1 && adata.e_used > limit) {
		adata.estimateexceeded = 1;
	}

	if (adata.limitexceeded) {
		if (aexit == AE_Exit_1_On_Limit) {
			ret = 1;
		} else if (aexit == AE_Exit_2_On_Limit) {
			ret = 2;
		}
	}

	if (adata.estimateexceeded) {
		if (aexit == AE_Exit_1_On_Estimate) {
			ret = 1;
		} else if (aexit == AE_Exit_2_On_Estimate) {
			ret = 2;
		}
	}

	if (output != AO_No_Output) {
		if (cfg.qmode == 10) { // --json
			jsonalertoutput(&adata, output, type, condition, limit);
		} else {
			alertoutput(&adata, output, type, condition, limit);
		}
	}

	dbdatalistfree(&adata.datalist);

	timeused_debug(__func__, 0);

	return ret;
}

void alertoutput(const alertdata *adata, const AlertOutput output, const AlertType type, const AlertCondition condition, const uint64_t limit)
{
	int i, l;
	uint64_t periodseconds = 0;
	double percentage = 0.0;
	char datebuff[DATEBUFFLEN], linebuffer[128], buffer[32];

	if (output == AO_Always_Output || (output == AO_Output_On_Estimate && adata->estimateexceeded) || ((output == AO_Output_On_Limit || output == AO_Output_On_Estimate) && adata->limitexceeded)) {
		if (type == AT_Percentile && cfg.fiveminutehours < PERCENTILEENTRYCOUNT) {
			printf("\nWarning: Configuration \"5MinuteHours\" needs to be at least %d for 100%% coverage.\n", PERCENTILEENTRYCOUNT);
			printf("         \"5MinuteHours\" is currently set at %d.\n\n", cfg.fiveminutehours);
		}

		if (strlen(adata->ifaceinfo.alias)) {
			printf("\n   %s (%s)", adata->ifaceinfo.alias, adata->ifaceinfo.name);
		} else {
			printf("\n   %s", adata->ifaceinfo.name);
		}
		if (adata->ifaceinfo.updated) {
			strftime(datebuff, DATEBUFFLEN, DATETIMEFORMATWITHOUTSECS, localtime(&adata->ifaceinfo.updated));
			printf(" at %s", datebuff);
		}
		if (adata->timestamp) {
			switch (type) {
				case AT_None:
					break;
				case AT_Percentile:
					strftime(datebuff, DATEBUFFLEN, cfg.mformat, localtime(&adata->timestamp));
					printf(" for 95th percentile of %s", datebuff);
					break;
				case AT_Hour:
					strftime(datebuff, DATEBUFFLEN, "%H", localtime(&adata->timestamp));
					printf(" for %s %s of ", adata->tablename, datebuff);
					strftime(datebuff, DATEBUFFLEN, cfg.dformat, localtime(&adata->timestamp));
					printf("%s", datebuff);
					break;
				case AT_Day:
					strftime(datebuff, DATEBUFFLEN, cfg.dformat, localtime(&adata->timestamp));
					printf(" for %s %s", adata->tablename, datebuff);
					break;
				case AT_Month:
					strftime(datebuff, DATEBUFFLEN, cfg.mformat, localtime(&adata->timestamp));
					printf(" for %s %s", adata->tablename, datebuff);
					break;
				case AT_Year:
					strftime(datebuff, DATEBUFFLEN, "%Y", localtime(&adata->timestamp));
					printf(" for %s %s", adata->tablename, datebuff);
					break;
			}
		}
		printf("\n\n");
	}

	if ((output == AO_Always_Output || output == AO_Output_On_Limit || output == AO_Output_On_Estimate) && adata->limitexceeded) {
		printf("                          Alert limit exceeded!\n\n");
	} else if (output == AO_Always_Output || (output == AO_Output_On_Estimate && adata->estimateexceeded)) {
		if (adata->estimateexceeded) {
			printf("             Warning: Limit will be exceeded at current rate\n\n");
		}
		printf("     [");
		l = (int)lrint((double)adata->used / (double)limit * ALERTUSAGELEN);
		if (l > ALERTUSAGELEN) {
			l = ALERTUSAGELEN;
		}
		for (i = 0; i < l; i++) {
			printf("=");
		}
		if (adata->ongoing && cfg.estimatevisible) {
			if (!adata->estimateexceeded) {
				l = (int)lrint((double)adata->e_used / (double)limit * ALERTUSAGELEN);
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
		printf("]\n");
	}

	if (output == AO_Always_Output || (output == AO_Output_On_Estimate && adata->estimateexceeded) || ((output == AO_Output_On_Limit || output == AO_Output_On_Estimate) && adata->limitexceeded)) {
		percentage = (double)adata->used / (double)limit * 100.0;
		if (type != AT_Percentile) {
			printf("\n      %8s |", adata->typeoutput);
			if (adata->ongoing) {
				printf("   %9s      |", adata->conditionname);
			} else {
				printf("  %14s  |", adata->conditionname);
			}
			printf("   percentage   |   avg. rate\n");
			printf("     ----------+------------------+----------------+--------------\n");
			printf("          used | %16s |", getvalue(adata->used, 16, RT_Normal));
			periodseconds = getperiodseconds(adata->listtype, adata->datalist->timestamp, adata->ifaceinfo.updated, adata->ifaceinfo.created, adata->ongoing);
			if (adata->ongoing && periodseconds == 0) {
				periodseconds = getperiodseconds(adata->listtype, adata->datalist->timestamp, adata->ifaceinfo.updated, adata->ifaceinfo.created, 0);
			}
			if (percentage <= 100000.0) {
				printf(" %13.1f%% | %13s\n", percentage, gettrafficrate(adata->used, (time_t)periodseconds, 13));
			} else {
				printf(" %14s | %13s\n", ">100000%", gettrafficrate(adata->used, (time_t)periodseconds, 13));
			}
			printf("         limit | %16s |", getvalue(limit, 16, RT_Normal));
			printf("                | %13s\n", gettrafficrate(limit, (time_t)getperiodseconds(adata->listtype, adata->datalist->timestamp, adata->ifaceinfo.updated, adata->ifaceinfo.created, 0), 13));

			if (adata->limitexceeded) {
				printf("        excess | %16s |                |\n", getvalue(adata->used - limit, 16, RT_Normal));
				printf("     ----------+------------------+----------------+--------------\n");
			} else {
				printf("     remaining | %16s | %13.1f%% |\n", getvalue(limit - adata->used, 16, RT_Normal), 100.0 - percentage);
				printf("     ----------+------------------+----------------+--------------\n");
			}
			if (adata->ongoing && cfg.estimatevisible && adata->e_used > 0) {
				printf("     %9s | %16s |", cfg.estimatetext, getvalue(adata->e_used, 16, RT_Normal));
				percentage = (double)adata->e_used / (double)limit * 100.0;
				if (percentage <= 100000.0) {
					printf(" %13.1f%%", percentage);
				} else {
					printf(" %14s", ">100000%");
				}
				if (adata->e_used > limit) {
					printf(", +%s\n", getvalue(adata->e_used - limit, 0, RT_Normal));
				} else {
					/* rate for estimated is always to same as for used so "used" intentionally used here instead of "e_used" */
					printf(" | %13s\n", gettrafficrate(adata->used, (time_t)periodseconds, 13));
				}
			}
		} else {
			if (condition == AC_RX) {
				snprintf(linebuffer, 128, "rx - ");
			} else if (condition == AC_TX) {
				snprintf(linebuffer, 128, "tx - ");
			} else if (condition == AC_Total) {
				snprintf(linebuffer, 128, "total - ");
			}
			strncat(linebuffer, gettrafficrate(adata->used, 1, 0), 16);
			if (percentage <= 100000.0) {
				snprintf(buffer, 32, " (%0.1f%%)", percentage);
			} else {
				snprintf(buffer, 32, " (>100000%%)");
			}
			strncat(linebuffer, buffer, 32);
			snprintf(buffer, 32, " of %s limit", gettrafficrate(limit, 1, 0));
			strncat(linebuffer, buffer, 32);
			printf("%*s%s\n\n", (int)(5 + (61 - strlen(linebuffer)) / 2), " ", linebuffer);

			cfg.ostyle = 1;
			showpercentiledatatable(&adata->pdata, 5, 0);

			snprintf(linebuffer, 128, "%" PRIu32 " entries", adata->pdata.count);
			if (condition == AC_RX) {
				snprintf(buffer, 32, ", %" PRIu32 " (%0.1f%%) over limit", adata->pdata.countrxoveruserlimit, (float)adata->pdata.countrxoveruserlimit / (float)adata->pdata.count * 100.0);
			} else if (condition == AC_TX) {
				snprintf(buffer, 32, ", %" PRIu32 " (%0.1f%%) over limit", adata->pdata.counttxoveruserlimit, (float)adata->pdata.counttxoveruserlimit / (float)adata->pdata.count * 100.0);
			} else if (condition == AC_Total) {
				snprintf(buffer, 32, ", %" PRIu32 " (%0.1f%%) over limit", adata->pdata.countsumoveruserlimit, (float)adata->pdata.countsumoveruserlimit / (float)adata->pdata.count * 100.0);
			}
			strncat(linebuffer, buffer, 32);
			if (adata->pdata.count == adata->pdata.countexpectation) {
				snprintf(buffer, 32, ", 100%% coverage");
			} else {
				snprintf(buffer, 32, ", %0.1f%% coverage", (float)adata->pdata.count / (float)adata->pdata.countexpectation * 100.0);
			}
			strncat(linebuffer, buffer, 32);
			printf("%*s%s\n", (int)(5 + (61 - strlen(linebuffer)) / 2), " ", linebuffer);
		}
	}
}
