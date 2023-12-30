#include "common.h"
#include "dbjson.h"

void showjson(const char *interface, const int ifcount, const char mode, const char *databegin, const char *dataend)
{
	interfaceinfo ifaceinfo;

	timeused_debug(__func__, 1);

	if (!db_getinterfacecountbyname(interface)) {
		exit(EXIT_FAILURE);
	}

	if (!db_getinterfaceinfo(interface, &ifaceinfo)) {
		exit(EXIT_FAILURE);
	}

	if (ifcount) {
		printf(",");
	}

	printf("{");
	jsoninterfaceinfo(&ifaceinfo);

	if (mode == 'p') {
		jsonpercentile(&ifaceinfo);
		printf("}");
		timeused_debug(__func__, 0);
		return;
	}

	printf("\"traffic\":");
	printf("{\"total\":{\"rx\":%" PRIu64 ",\"tx\":%" PRIu64 "},", ifaceinfo.rxtotal, ifaceinfo.txtotal);

	switch (mode) {
		case 'd':
			jsondump(&ifaceinfo, "day", 1, databegin, dataend);
			break;
		case 'm':
			jsondump(&ifaceinfo, "month", 3, databegin, dataend);
			break;
		case 't':
			jsondump(&ifaceinfo, "top", 1, databegin, dataend);
			break;
		case 'h':
			jsondump(&ifaceinfo, "hour", 2, databegin, dataend);
			break;
		case 'y':
			jsondump(&ifaceinfo, "year", 4, databegin, dataend);
			break;
		case 'f':
			jsondump(&ifaceinfo, "fiveminute", 2, databegin, dataend);
			break;
		case 's':
			jsondump(&ifaceinfo, "fiveminute", 2, "", "");
			printf(",");
			jsondump(&ifaceinfo, "hour", 2, "", "");
			printf(",");
			jsondump(&ifaceinfo, "day", 1, "", "");
			printf(",");
			jsondump(&ifaceinfo, "month", 3, "", "");
			printf(",");
			jsondump(&ifaceinfo, "year", 4, "", "");
			break;
		case 'a':
		default:
			jsondump(&ifaceinfo, "fiveminute", 2, databegin, dataend);
			printf(",");
			jsondump(&ifaceinfo, "hour", 2, databegin, dataend);
			printf(",");
			jsondump(&ifaceinfo, "day", 1, databegin, dataend);
			printf(",");
			jsondump(&ifaceinfo, "month", 3, databegin, dataend);
			printf(",");
			jsondump(&ifaceinfo, "year", 4, databegin, dataend);
			printf(",");
			jsondump(&ifaceinfo, "top", 1, databegin, dataend);
			break;
	}

	printf("}}");

	timeused_debug(__func__, 0);
}

void jsondump(const interfaceinfo *ifaceinfo, const char *tablename, const int datetype, const char *databegin, const char *dataend)
{
	int first = 1;
	dbdatalist *datalist = NULL, *datalist_i = NULL;
	dbdatalistinfo datainfo;

	if (!db_getdata_range(&datalist, &datainfo, ifaceinfo->name, tablename, (uint32_t)cfg.listjsonxml, databegin, dataend)) {
		printf("Error: Failed to fetch %s data.\n", tablename);
		exit(EXIT_FAILURE);
	}

	printf("\"%s\":[", tablename);
	datalist_i = datalist;
	while (datalist_i != NULL) {
		if (!first) {
			printf(",");
		} else {
			first = 0;
		}
		printf("{\"id\":%" PRId64 ",", datalist_i->rowid);
		jsondate(&datalist_i->timestamp, datetype);
		printf(",\"rx\":%" PRIu64 ",\"tx\":%" PRIu64 "}", datalist_i->rx, datalist_i->tx);
		datalist_i = datalist_i->next;
	}
	dbdatalistfree(&datalist);
	printf("]");
}

void jsonpercentile(const interfaceinfo *ifaceinfo)
{
	percentiledata pdata;

	if (!getpercentiledata(&pdata, ifaceinfo->name, 0)) {
		exit(EXIT_FAILURE);
	}

	printf("\"bandwidth\":{");

	printf("\"month\":{");
	jsondate(&pdata.monthbegin, 1);
	printf("},");

	printf("\"data_begin\":{");
	jsondate(&pdata.databegin, 2);
	printf("},");

	printf("\"data_end\":{");
	jsondate(&pdata.dataend, 2);
	printf("},");

	printf("\"entries\":{");
	printf("\"seen\":%" PRIu32 ",", pdata.count);
	printf("\"expected\":%" PRIu32 ",", pdata.countexpectation);
	printf("\"missing\":%" PRIu32 ",", pdata.countexpectation - pdata.count);
	printf("\"coverage_percentage\":%0.1f", (float)pdata.count / (float)pdata.countexpectation * 100.0);
	printf("},");

	jsonpercentileminavgmax(&pdata);

	printf("\"95th_percentile\":{");
	printf("\"rx_bytes_per_second\":%" PRIu64 ",", (uint64_t)(pdata.rxpercentile / (double)300));
	printf("\"tx_bytes_per_second\":%" PRIu64 ",", (uint64_t)(pdata.txpercentile / (double)300));
	printf("\"total_bytes_per_second\":%" PRIu64 "", (uint64_t)(pdata.sumpercentile / (double)300));
	printf("}");

	printf("}");
}

void jsonpercentileminavgmax(const percentiledata *pdata)
{
	printf("\"minimum\":{");
	printf("\"rx_bytes_per_second\":%" PRIu64 ",", (uint64_t)(pdata->minrx / (double)300));
	printf("\"tx_bytes_per_second\":%" PRIu64 ",", (uint64_t)(pdata->mintx / (double)300));
	printf("\"total_bytes_per_second\":%" PRIu64 "", (uint64_t)(pdata->min / (double)300));
	printf("},");

	printf("\"average\":{");
	printf("\"rx_bytes_per_second\":%" PRIu64 ",", (uint64_t)(pdata->sumrx / (double)(pdata->count * 300)));
	printf("\"tx_bytes_per_second\":%" PRIu64 ",", (uint64_t)(pdata->sumtx / (double)(pdata->count * 300)));
	printf("\"total_bytes_per_second\":%" PRIu64 "", (uint64_t)((pdata->sumrx + pdata->sumtx) / (double)(pdata->count * 300)));
	printf("},");

	printf("\"maximum\":{");
	printf("\"rx_bytes_per_second\":%" PRIu64 ",", (uint64_t)(pdata->maxrx / (double)300));
	printf("\"tx_bytes_per_second\":%" PRIu64 ",", (uint64_t)(pdata->maxtx / (double)300));
	printf("\"total_bytes_per_second\":%" PRIu64 "", (uint64_t)(pdata->max / (double)300));
	printf("},");
}

void jsonalertoutput(const alertdata *adata, const AlertOutput output, const AlertType type, const AlertCondition condition, const uint64_t limit)
{
	if (output == AO_Always_Output || (output == AO_Output_On_Estimate && adata->estimateexceeded) || ((output == AO_Output_On_Limit || output == AO_Output_On_Estimate) && adata->limitexceeded)) {
		jsonheader(JSONALERT);
		printf("{");
		jsoninterfaceinfo(&adata->ifaceinfo);
		if (type != AT_Percentile) {
			jsonalert(adata, limit);
		} else {
			jsonpercentilealert(adata, condition, limit);
		}
		printf("}");
		jsonfooter();
	}
}

void jsonalert(const alertdata *adata, const uint64_t limit)
{
	double percentage = (double)adata->used / (double)limit * 100.0;

	printf("\"alert\":{");
	printf("\"type\":\"%s\",", adata->tablename);

	printf("\"%s\":{", adata->tablename);
	jsondate(&adata->timestamp, 1);
	printf("},");

	printf("\"limit\":{");
	printf("\"exceeded\":");
	if (adata->limitexceeded) {
		printf("true,");
	} else {
		printf("false,");
	}
	printf("\"condition\":\"%s\",", adata->conditionname);
	printf("\"limit_bytes\":%" PRIu64 ",", limit);
	printf("\"used_bytes\":%" PRIu64 ",", adata->used);
	if (adata->used < limit) {
		printf("\"remaining_bytes\":%" PRIu64 ",", limit - adata->used);
	} else {
		printf("\"remaining_bytes\":0,");
	}
	printf("\"used_percentage\":");
	if (percentage <= 100000.0) {
		printf("%0.1f,", percentage);
	} else {
		printf("100000.0,");
	}
	printf("\"remaining_percentage\":");
	if (percentage >= 100.0) {
		printf("0.0");
	} else {
		printf("%0.1f", 100.0 - percentage);
	}
	printf("}");

	/* show estimate section when condition isn't estimate */
	if (strstr(adata->conditionname, "estimate") == NULL) {
		percentage = (double)adata->e_used / (double)limit * 100.0;

		printf(",\"estimate\":{");
		printf("\"exceeded\":");
		if (adata->estimateexceeded) {
			printf("true,");
		} else {
			printf("false,");
		}
		printf("\"limit_bytes\":%" PRIu64 ",", limit);
		printf("\"used_bytes\":%" PRIu64 ",", adata->e_used);
		if (adata->e_used < limit) {
			printf("\"remaining_bytes\":%" PRIu64 ",", limit - adata->e_used);
		} else {
			printf("\"remaining_bytes\":0,");
		}
		printf("\"used_percentage\":");
		if (percentage <= 100000.0) {
			printf("%0.1f,", percentage);
		} else {
			printf("100000.0,");
		}
		printf("\"remaining_percentage\":");
		if (percentage >= 100.0) {
			printf("0.0");
		} else {
			printf("%0.1f", 100.0 - percentage);
		}
		printf("}");
	}

	printf("}");
}

void jsonpercentilealert(const alertdata *adata, const AlertCondition condition, const uint64_t limit)
{
	double percentage = (double)adata->used / (double)limit * 100.0;

	printf("\"alert\":{");
	printf("\"type\":\"95th percentile\",");

	printf("\"month\":{");
	jsondate(&adata->pdata.monthbegin, 1);
	printf("},");

	printf("\"data_begin\":{");
	jsondate(&adata->pdata.databegin, 2);
	printf("},");

	printf("\"data_end\":{");
	jsondate(&adata->pdata.dataend, 2);
	printf("},");

	printf("\"limit\":{");
	printf("\"exceeded\":");
	if (adata->limitexceeded) {
		printf("true,");
	} else {
		printf("false,");
	}
	printf("\"condition\":\"%s\",", adata->conditionname);
	printf("\"limit_bytes_per_second\":%" PRIu64 ",", limit);
	printf("\"used_bytes_per_second\":%" PRIu64 ",", adata->used);
	printf("\"used_percentage\":");
	if (percentage <= 100000.0) {
		printf("%0.1f,", percentage);
	} else {
		printf("100000.0,");
	}
	printf("\"remaining_percentage\":");
	if (percentage >= 100.0) {
		printf("0.0");
	} else {
		printf("%0.1f", 100.0 - percentage);
	}
	printf("},");

	jsonpercentileminavgmax(&adata->pdata);

	printf("\"entries\":{");
	printf("\"seen\":%" PRIu32 ",", adata->pdata.count);
	printf("\"expected\":%" PRIu32 ",", adata->pdata.countexpectation);
	printf("\"missing\":%" PRIu32 ",", adata->pdata.countexpectation - adata->pdata.count);
	printf("\"coverage_percentage\":%0.1f,", (float)adata->pdata.count / (float)adata->pdata.countexpectation * 100.0);
	if (condition == AC_RX) {
		printf("\"over_limit\":%" PRIu32 ",", adata->pdata.countrxoveruserlimit);
		printf("\"over_limit_percentage\":%0.1f", (float)adata->pdata.countrxoveruserlimit / (float)adata->pdata.count * 100.0);
	} else if (condition == AC_TX) {
		printf("\"over_limit\":%" PRIu32 ",", adata->pdata.counttxoveruserlimit);
		printf("\"over_limit_percentage\":%0.1f", (float)adata->pdata.counttxoveruserlimit / (float)adata->pdata.count * 100.0);
	} else if (condition == AC_Total) {
		printf("\"over_limit\":%" PRIu32 ",", adata->pdata.countsumoveruserlimit);
		printf("\"over_limit_percentage\":%0.1f", (float)adata->pdata.countsumoveruserlimit / (float)adata->pdata.count * 100.0);
	}
	printf("}");

	printf("}");
}

void jsoninterfaceinfo(const interfaceinfo *ifaceinfo)
{
	printf("\"name\":\"%s\",", ifaceinfo->name);
	printf("\"alias\":\"%s\",", ifaceinfo->alias);

	printf("\"created\":{");
	jsondate(&ifaceinfo->created, 1);
	printf("},");
	printf("\"updated\":{");
	jsondate(&ifaceinfo->updated, 2);
	printf("},");
}

void jsondate(const time_t *date, const int type)
{
	struct tm *d;

	d = localtime(date);

	switch (type) {
		case 1:
			printf("\"date\":{\"year\":%d,\"month\":%d,\"day\":%d},",
				   1900 + d->tm_year, 1 + d->tm_mon, d->tm_mday);
			break;
		case 2:
			printf("\"date\":{\"year\":%d,\"month\":%d,\"day\":%d},\"time\":{\"hour\":%d,\"minute\":%d},",
				   1900 + d->tm_year, 1 + d->tm_mon, d->tm_mday, d->tm_hour, d->tm_min);
			break;
		case 3:
			printf("\"date\":{\"year\":%d,\"month\":%d},",
				   1900 + d->tm_year, 1 + d->tm_mon);
			break;
		case 4:
			printf("\"date\":{\"year\":%d},",
				   1900 + d->tm_year);
			break;
		default:
			break;
	}
	printf("\"timestamp\":%" PRId64 "", (uint64_t)*date);
}

void jsonheader(const char *version)
{
	printf("{\"vnstatversion\":\"%s\",\"jsonversion\":\"%s\",\"interfaces\":[", getversion(), version);
}

void jsonfooter(void)
{
	printf("]}\n");
}
