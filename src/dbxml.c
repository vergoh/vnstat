#include "common.h"
#include "dbsql.h"
#include "dbxml.h"

void showxml(const char *interface, const char mode, const char *databegin, const char *dataend)
{
	interfaceinfo info;

	timeused(__func__, 1);

	if (!db_getinterfacecountbyname(interface)) {
		return;
	}

	if (!db_getinterfaceinfo(interface, &info)) {
		return;
	}

	printf(" <interface name=\"%s\">\n", info.name);

	printf("  <name>%s</name>\n", info.name);
	printf("  <alias>%s</alias>\n", info.alias);

	printf("  <created>");
	xmldate(&info.created, 1);
	printf("</created>\n");
	printf("  <updated>");
	xmldate(&info.updated, 2);
	printf("</updated>\n");

	printf("  <traffic>\n");
	printf("   <total><rx>%" PRIu64 "</rx><tx>%" PRIu64 "</tx></total>\n", info.rxtotal, info.txtotal);

	switch (mode) {
		case 'd':
			xmldump(&info, "day", 1, databegin, dataend);
			break;
		case 'm':
			xmldump(&info, "month", 3, databegin, dataend);
			break;
		case 't':
			xmldump(&info, "top", 1, databegin, dataend);
			break;
		case 'h':
			xmldump(&info, "hour", 2, databegin, dataend);
			break;
		case 'y':
			xmldump(&info, "year", 4, databegin, dataend);
			break;
		case 'f':
			xmldump(&info, "fiveminute", 2, databegin, dataend);
			break;
		case 'a':
		default:
			xmldump(&info, "fiveminute", 2, databegin, dataend);
			xmldump(&info, "hour", 2, databegin, dataend);
			xmldump(&info, "day", 1, databegin, dataend);
			xmldump(&info, "month", 3, databegin, dataend);
			xmldump(&info, "year", 2, databegin, dataend);
			xmldump(&info, "top", 1, databegin, dataend);
			break;
	}

	printf("  </traffic>\n");
	printf(" </interface>\n");

	timeused(__func__, 0);
}

void xmldump(const interfaceinfo *interface, const char *tablename, const int datetype, const char *databegin, const char *dataend)
{
	dbdatalist *datalist = NULL, *datalist_i = NULL;
	dbdatalistinfo datainfo;

	if (!db_getdata_range(&datalist, &datainfo, interface->name, tablename, (uint32_t)cfg.listjsonxml, databegin, dataend)) {
		printf("Error: Failed to fetch %s data.\n", tablename);
		return;
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
}

void xmlheader(void)
{
	printf("<vnstat version=\"%s\" xmlversion=\"%d\">\n", getversion(), XMLVERSION);
}

void xmlfooter(void)
{
	printf("</vnstat>\n");
}
