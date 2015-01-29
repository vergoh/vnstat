#include "common.h"
#include "dbjson.h"

void showjson(int dbcount, char mode)
{
	if (dbcount) {
		printf(",");
	}
	printf("{");
	printf("\"id\":\"%s\",", data.interface);
	printf("\"nick\":\"%s\",", data.nick);

	printf("\"created\":{");
	jsondate(&data.created, 1);
	printf("},");
	printf("\"updated\":{");
	jsondate(&data.lastupdated, 2);
	printf("},");

	printf("\"traffic\":");
	printf("{\"total\":{\"rx\":%"PRIu64",\"tx\":%"PRIu64"},", (data.totalrx*1024)+data.totalrxk, (data.totaltx*1024)+data.totaltxk);

	switch (mode) {
		case 'd':
			jsondays();
			break;
		case 'm':
			jsonmonths();
			break;
		case 't':
			jsontops();
			break;
		case 'h':
			jsonhours();
			break;
		case 'a':
		default:
			jsondays();
			printf(",");
			jsonmonths();
			printf(",");
			jsontops();
			printf(",");
			jsonhours();
			break;
	}

	printf("}}");
}

void jsondays(void)
{
	int i, first;

	printf("\"days\":[");
	for (i=0,first=1;i<=29;i++) {
		if (!data.day[i].used) {
			continue;
		}
		if (!first) {
			printf(",");
		}
		printf("{\"id\":%d,", i);
		jsondate(&data.day[i].date, 1);
		printf(",\"rx\":%"PRIu64",\"tx\":%"PRIu64"}", (data.day[i].rx*1024)+data.day[i].rxk, (data.day[i].tx*1024)+data.day[i].txk);
		first = 0;
	}
	printf("]");
}

void jsonmonths(void)
{
	int i, first;

	printf("\"months\":[");
	for (i=0,first=1;i<=11;i++) {
		if (!data.month[i].used) {
			continue;
		}
		if (!first) {
			printf(",");
		}
		printf("{\"id\":%d,", i);
		jsondate(&data.month[i].month, 3);
		printf(",\"rx\":%"PRIu64",\"tx\":%"PRIu64"}", (data.month[i].rx*1024)+data.month[i].rxk, (data.month[i].tx*1024)+data.month[i].txk);
		first = 0;
	}
	printf("]");
}

void jsontops(void)
{
	int i, first;

	printf("\"tops\":[");
	for (i=0,first=1;i<=9;i++) {
		if (!data.top10[i].used) {
			continue;
		}
		if (!first) {
			printf(",");
		}
		printf("{\"id\":%d,", i);
		jsondate(&data.top10[i].date, 2);
		printf(",\"rx\":%"PRIu64",\"tx\":%"PRIu64"}", (data.top10[i].rx*1024)+data.top10[i].rxk, (data.top10[i].tx*1024)+data.top10[i].txk);
		first = 0;
	}
	printf("]");
}

void jsonhours(void)
{
	int i, first;

	printf("\"hours\":[");
	for (i=0,first=1;i<=23;i++) {
		if (data.hour[i].date==0) {
			continue;
		}
		if (!first) {
			printf(",");
		}
		printf("{\"id\":%d,", i);
		jsondate(&data.hour[i].date, 1);
		printf(",\"rx\":%"PRIu64",\"tx\":%"PRIu64"}", data.hour[i].rx, data.hour[i].tx);
		first = 0;
	}
	printf("]");
}

void jsondate(time_t *date, int type)
{
	struct tm *d;
	char *type1 = "\"date\":{\"year\":%d,\"month\":%d,\"day\":%d}";
	char *type2 = "\"date\":{\"year\":%d,\"month\":%d,\"day\":%d},\"time\":{\"hour\":%d,\"minutes\":%d}";
	char *type3 = "\"date\":{\"year\":%d,\"month\":%d}";

	d = localtime(date);

	if (type == 1) {
		printf(type1, 1900+d->tm_year, 1+d->tm_mon, d->tm_mday);
	} else if (type == 2) {
		printf(type2, 1900+d->tm_year, 1+d->tm_mon, d->tm_mday, d->tm_hour, d->tm_min);
	} else if (type == 3) {
		printf(type3, 1900+d->tm_year, 1+d->tm_mon);
	}
}

void jsonheader(void)
{
	printf("{\"vnstatversion\":\"%s\",\"jsonversion\":\"%d\",\"interfaces\":[", VNSTATVERSION, JSONVERSION);
}

void jsonfooter(void)
{
	printf("]}\n");
}
