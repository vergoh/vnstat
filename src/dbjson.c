#include "common.h"
#include "dbsql.h"
#include "dbjson.h"

void showjson(const char *interface, const int dbcount, const char mode)
{
	interfaceinfo info;

	timeused(__func__, 1);

	if (!db_getinterfacecountbyname(interface)) {
		return;
	}

	if (!db_getinterfaceinfo(interface, &info)) {
		return;
	}

	if (dbcount) {
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

	printf("\"traffic\":");
	printf("{\"total\":{\"rx\":%"PRIu64",\"tx\":%"PRIu64"},", info.rxtotal, info.txtotal);

	switch (mode) {
		case 'd':
			jsondump(&info, "day", 1);
			break;
		case 'm':
			jsondump(&info, "month", 3);
			break;
		case 't':
			jsondump(&info, "top", 1);
			break;
		case 'h':
			jsondump(&info, "hour", 2);
			break;
		case 'y':
			jsondump(&info, "year", 4);
			break;
		case 'f':
			jsondump(&info, "fiveminute", 2);
			break;
		case 'a':
		default:
			jsondump(&info, "fiveminute", 2);
			printf(",");
			jsondump(&info, "hour", 2);
			printf(",");
			jsondump(&info, "day", 1);
			printf(",");
			jsondump(&info, "month", 3);
			printf(",");
			jsondump(&info, "year", 4);
			printf(",");
			jsondump(&info, "top", 1);
			break;
	}

	printf("}}");

	timeused(__func__, 0);
}

void jsondump(const interfaceinfo *interface, const char *tablename, const int datetype)
{
	int first = 1;
	dbdatalist *datalist = NULL, *datalist_i = NULL;
	dbdatalistinfo datainfo;

	if (!db_getdata(&datalist, &datainfo, interface->name, tablename, (uint32_t)cfg.listjsonxml)) {
		printf("Error: Failed to fetch %s data.\n", tablename);
		return;
	}

	printf("\"%s\":[", tablename);
	datalist_i = datalist;
	while (datalist_i != NULL) {
		if (!first) {
			printf(",");
		} else {
			first = 0;
		}
		printf("{\"id\":%"PRId64",", datalist_i->rowid);
		jsondate(&datalist_i->timestamp, datetype);
		printf(",\"rx\":%"PRIu64",\"tx\":%"PRIu64"}", datalist_i->rx, datalist_i->tx);
		datalist_i = datalist_i->next;
	}
	dbdatalistfree(&datalist);
	printf("]");
}

void jsondate(const time_t *date, const int type)
{
	struct tm *d;

	d = localtime(date);

	switch (type) {
		case 1:
			printf("\"date\":{\"year\":%d,\"month\":%d,\"day\":%d}", \
					1900+d->tm_year, 1+d->tm_mon, d->tm_mday);
			break;
		case 2:
			printf("\"date\":{\"year\":%d,\"month\":%d,\"day\":%d},\"time\":{\"hour\":%d,\"minute\":%d}", \
					1900+d->tm_year, 1+d->tm_mon, d->tm_mday, d->tm_hour, d->tm_min);
			break;
		case 3:
			printf("\"date\":{\"year\":%d,\"month\":%d}", \
					1900+d->tm_year, 1+d->tm_mon);
			break;
		case 4:
			printf("\"date\":{\"year\":%d}", \
					1900+d->tm_year);
			break;
		default:
			break;
	}
}

void jsonheader(void)
{
	printf("{\"vnstatversion\":\"%s\",\"jsonversion\":\"%d\",\"interfaces\":[", getversion(), JSONVERSION);
}

void jsonfooter(void)
{
	printf("]}\n");
}
