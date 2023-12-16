#include "common.h"
#include "dbsql.h"
#include "percentile.h"
#include "dbjson.h"

void showjson(const char *interface, const int ifcount, const char mode, const char *databegin, const char *dataend)
{
	interfaceinfo info;

	timeused_debug(__func__, 1);

	if (!db_getinterfacecountbyname(interface)) {
		exit(EXIT_FAILURE);
	}

	if (!db_getinterfaceinfo(interface, &info)) {
		exit(EXIT_FAILURE);
	}

	if (ifcount) {
		printf(",");
	}
	printf("{");
	printf("\"name\":\"%s\",", info.name);
	printf("\"alias\":\"%s\",", info.alias);

	printf("\"created\":{");
	jsondate(&info.created, 1);
	printf("},");
	printf("\"updated\":{");
	jsondate(&info.updated, 2);
	printf("},");

	if (mode == 'p') {
		jsonpercentile(&info);
		printf("}");
		timeused_debug(__func__, 0);
		return;
	}

	printf("\"traffic\":");
	printf("{\"total\":{\"rx\":%" PRIu64 ",\"tx\":%" PRIu64 "},", info.rxtotal, info.txtotal);

	switch (mode) {
		case 'd':
			jsondump(&info, "day", 1, databegin, dataend);
			break;
		case 'm':
			jsondump(&info, "month", 3, databegin, dataend);
			break;
		case 't':
			jsondump(&info, "top", 1, databegin, dataend);
			break;
		case 'h':
			jsondump(&info, "hour", 2, databegin, dataend);
			break;
		case 'y':
			jsondump(&info, "year", 4, databegin, dataend);
			break;
		case 'f':
			jsondump(&info, "fiveminute", 2, databegin, dataend);
			break;
		case 's':
			jsondump(&info, "fiveminute", 2, "", "");
			printf(",");
			jsondump(&info, "hour", 2, "", "");
			printf(",");
			jsondump(&info, "day", 1, "", "");
			printf(",");
			jsondump(&info, "month", 3, "", "");
			printf(",");
			jsondump(&info, "year", 4, "", "");
			break;
		case 'a':
		default:
			jsondump(&info, "fiveminute", 2, databegin, dataend);
			printf(",");
			jsondump(&info, "hour", 2, databegin, dataend);
			printf(",");
			jsondump(&info, "day", 1, databegin, dataend);
			printf(",");
			jsondump(&info, "month", 3, databegin, dataend);
			printf(",");
			jsondump(&info, "year", 4, databegin, dataend);
			printf(",");
			jsondump(&info, "top", 1, databegin, dataend);
			break;
	}

	printf("}}");

	timeused_debug(__func__, 0);
}

void jsondump(const interfaceinfo *interface, const char *tablename, const int datetype, const char *databegin, const char *dataend)
{
	int first = 1;
	dbdatalist *datalist = NULL, *datalist_i = NULL;
	dbdatalistinfo datainfo;

	if (!db_getdata_range(&datalist, &datainfo, interface->name, tablename, (uint32_t)cfg.listjsonxml, databegin, dataend)) {
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

void jsonpercentile(const interfaceinfo *interface)
{
	percentiledata pdata;

	if (!getpercentiledata(&pdata, interface->name, 0)) {
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

	printf("\"entries\":{\"seen\":%" PRIu32 ",\"expected\":%" PRIu32 ",\"missing\":%" PRIu32 "},", pdata.count, pdata.countexpectation, pdata.countexpectation-pdata.count);

	printf("\"minimum\":{");
	printf("\"rx_bytes_per_second\":%" PRIu64 ",", (uint64_t)(pdata.minrx / (double)300));
	printf("\"tx_bytes_per_second\":%" PRIu64 ",", (uint64_t)(pdata.mintx / (double)300));
	printf("\"total_bytes_per_second\":%" PRIu64 "", (uint64_t)(pdata.min / (double)300));
	printf("},");

	printf("\"average\":{");
	printf("\"rx_bytes_per_second\":%" PRIu64 ",", (uint64_t)(pdata.sumrx / (double)(pdata.count * 300)));
	printf("\"tx_bytes_per_second\":%" PRIu64 ",", (uint64_t)(pdata.sumtx / (double)(pdata.count * 300)));
	printf("\"total_bytes_per_second\":%" PRIu64 "", (uint64_t)(pdata.sumrx + pdata.sumtx / (double)(pdata.count * 300)));
	printf("},");

	printf("\"maximum\":{");
	printf("\"rx_bytes_per_second\":%" PRIu64 ",", (uint64_t)(pdata.maxrx / (double)300));
	printf("\"tx_bytes_per_second\":%" PRIu64 ",", (uint64_t)(pdata.maxtx / (double)300));
	printf("\"total_bytes_per_second\":%" PRIu64 "", (uint64_t)(pdata.max / (double)300));
	printf("},");

	printf("\"95th_percentile\":{");
	printf("\"rx_bytes_per_second\":%" PRIu64 ",", (uint64_t)(pdata.rxpercentile / (double)300));
	printf("\"tx_bytes_per_second\":%" PRIu64 ",", (uint64_t)(pdata.txpercentile / (double)300));
	printf("\"total_bytes_per_second\":%" PRIu64 "", (uint64_t)(pdata.sumpercentile / (double)300));
	printf("}");

	printf("}");
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

void jsonheader(void)
{
	printf("{\"vnstatversion\":\"%s\",\"jsonversion\":\"%d\",\"interfaces\":[", getversion(), JSONVERSION);
}

void jsonfooter(void)
{
	printf("]}\n");
}
