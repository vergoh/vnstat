#include "common.h"
#include "dbsql.h"
#include "percentile.h"
#include "dbxml.h"

void showxml(const char *interface, const char mode, const char *databegin, const char *dataend)
{
	interfaceinfo ifaceinfo;

	timeused_debug(__func__, 1);

	if (!db_getinterfacecountbyname(interface)) {
		exit(EXIT_FAILURE);
	}

	if (!db_getinterfaceinfo(interface, &ifaceinfo)) {
		exit(EXIT_FAILURE);
	}

	printf(" <interface name=\"%s\">\n", ifaceinfo.name);

	printf("  <name>%s</name>\n", ifaceinfo.name);
	printf("  <alias>%s</alias>\n", ifaceinfo.alias);

	printf("  <created>");
	xmldate(&ifaceinfo.created, 1);
	printf("</created>\n");
	printf("  <updated>");
	xmldate(&ifaceinfo.updated, 2);
	printf("</updated>\n");

	if (mode == 'p') {
		xmlpercentile(&ifaceinfo);
		printf(" </interface>\n");
		timeused_debug(__func__, 0);
		return;
	}

	printf("  <traffic>\n");
	printf("   <total><rx>%" PRIu64 "</rx><tx>%" PRIu64 "</tx></total>\n", ifaceinfo.rxtotal, ifaceinfo.txtotal);

	switch (mode) {
		case 'd':
			xmldump(&ifaceinfo, "day", 1, databegin, dataend);
			break;
		case 'm':
			xmldump(&ifaceinfo, "month", 3, databegin, dataend);
			break;
		case 't':
			xmldump(&ifaceinfo, "top", 1, databegin, dataend);
			break;
		case 'h':
			xmldump(&ifaceinfo, "hour", 2, databegin, dataend);
			break;
		case 'y':
			xmldump(&ifaceinfo, "year", 4, databegin, dataend);
			break;
		case 'f':
			xmldump(&ifaceinfo, "fiveminute", 2, databegin, dataend);
			break;
		case 's':
			xmldump(&ifaceinfo, "fiveminute", 2, "", "");
			xmldump(&ifaceinfo, "hour", 2, "", "");
			xmldump(&ifaceinfo, "day", 1, "", "");
			xmldump(&ifaceinfo, "month", 3, "", "");
			xmldump(&ifaceinfo, "year", 2, "", "");
			break;
		case 'a':
		default:
			xmldump(&ifaceinfo, "fiveminute", 2, databegin, dataend);
			xmldump(&ifaceinfo, "hour", 2, databegin, dataend);
			xmldump(&ifaceinfo, "day", 1, databegin, dataend);
			xmldump(&ifaceinfo, "month", 3, databegin, dataend);
			xmldump(&ifaceinfo, "year", 2, databegin, dataend);
			xmldump(&ifaceinfo, "top", 1, databegin, dataend);
			break;
	}

	printf("  </traffic>\n");
	printf(" </interface>\n");

	timeused_debug(__func__, 0);
}

void xmldump(const interfaceinfo *ifaceinfo, const char *tablename, const int datetype, const char *databegin, const char *dataend)
{
	dbdatalist *datalist = NULL, *datalist_i = NULL;
	dbdatalistinfo datainfo;

	if (!db_getdata_range(&datalist, &datainfo, ifaceinfo->name, tablename, (uint32_t)cfg.listjsonxml, databegin, dataend)) {
		printf("Error: Failed to fetch %s data.\n", tablename);
		exit(EXIT_FAILURE);
	}

	printf("   <%ss>\n", tablename);
	datalist_i = datalist;
	while (datalist_i != NULL) {
		printf("    <%s id=\"%" PRId64 "\">", tablename, datalist_i->rowid);
		xmldate(&datalist_i->timestamp, datetype);
		printf("<rx>%" PRIu64 "</rx><tx>%" PRIu64 "</tx></%s>\n", datalist_i->rx, datalist_i->tx, tablename);
		datalist_i = datalist_i->next;
	}
	dbdatalistfree(&datalist);
	printf("   </%ss>\n", tablename);
}

void xmlpercentile(const interfaceinfo *ifaceinfo)
{
	percentiledata pdata;

	if (!getpercentiledata(&pdata, ifaceinfo->name, 0)) {
		exit(EXIT_FAILURE);
	}

	printf("  <bandwidth>\n");

	printf("   <month>");
	xmldate(&pdata.monthbegin, 1);
	printf("</month>\n");

	printf("   <data_begin>");
	xmldate(&pdata.databegin, 2);
	printf("</data_begin>\n");

	printf("   <data_end>");
	xmldate(&pdata.dataend, 2);
	printf("</data_end>\n");

	printf("   <entries><seen>%" PRIu32 "</seen><expected>%" PRIu32 "</expected><missing>%" PRIu32 "</missing></entries>\n", pdata.count, pdata.countexpectation, pdata.countexpectation-pdata.count);

	printf("   <minimum>");
	printf("<rx_bytes_per_second>%" PRIu64 "</rx_bytes_per_second>", (uint64_t)(pdata.minrx / (double)300));
	printf("<tx_bytes_per_second>%" PRIu64 "</tx_bytes_per_second>", (uint64_t)(pdata.mintx / (double)300));
	printf("<total_bytes_per_second>%" PRIu64 "</total_bytes_per_second>", (uint64_t)(pdata.min / (double)300));
	printf("</minimum>\n");

	printf("   <average>");
	printf("<rx_bytes_per_second>%" PRIu64 "</rx_bytes_per_second>", (uint64_t)(pdata.sumrx / (double)(pdata.count * 300)));
	printf("<tx_bytes_per_second>%" PRIu64 "</tx_bytes_per_second>", (uint64_t)(pdata.sumtx / (double)(pdata.count * 300)));
	printf("<total_bytes_per_second>%" PRIu64 "</total_bytes_per_second>", (uint64_t)(pdata.sumrx + pdata.sumtx / (double)(pdata.count * 300)));
	printf("</average>\n");

	printf("   <maximum>");
	printf("<rx_bytes_per_second>%" PRIu64 "</rx_bytes_per_second>", (uint64_t)(pdata.maxrx / (double)300));
	printf("<tx_bytes_per_second>%" PRIu64 "</tx_bytes_per_second>", (uint64_t)(pdata.maxtx / (double)300));
	printf("<total_bytes_per_second>%" PRIu64 "</total_bytes_per_second>", (uint64_t)(pdata.max / (double)300));
	printf("</maximum>\n");

	printf("   <95th_percentile>");
	printf("<rx_bytes_per_second>%" PRIu64 "</rx_bytes_per_second>", (uint64_t)(pdata.rxpercentile / (double)300));
	printf("<tx_bytes_per_second>%" PRIu64 "</tx_bytes_per_second>", (uint64_t)(pdata.txpercentile / (double)300));
	printf("<total_bytes_per_second>%" PRIu64 "</total_bytes_per_second>", (uint64_t)(pdata.sumpercentile / (double)300));
	printf("</95th_percentile>\n");

	printf("  </bandwidth>\n");
}

void xmldate(const time_t *date, const int type)
{
	struct tm *d;

	d = localtime(date);

	switch (type) {
		case 1:
			printf("<date><year>%d</year><month>%02d</month><day>%02d</day></date>",
				   1900 + d->tm_year, 1 + d->tm_mon, d->tm_mday);
			break;
		case 2:
			printf("<date><year>%d</year><month>%02d</month><day>%02d</day></date><time><hour>%02d</hour><minute>%02d</minute></time>",
				   1900 + d->tm_year, 1 + d->tm_mon, d->tm_mday, d->tm_hour, d->tm_min);
			break;
		case 3:
			printf("<date><year>%d</year><month>%02d</month></date>",
				   1900 + d->tm_year, 1 + d->tm_mon);
			break;
		case 4:
			printf("<date><year>%d</year></date>",
				   1900 + d->tm_year);
			break;
		default:
			break;
	}
	printf("<timestamp>%" PRId64 "</timestamp>", (uint64_t)*date);
}

void xmlheader(void)
{
	printf("<vnstat version=\"%s\" xmlversion=\"%d\">\n", getversion(), XMLVERSION);
}

void xmlfooter(void)
{
	printf("</vnstat>\n");
}
