#include "common.h"
#include "dbsql.h"
#include "percentile.h"

int getpercentiledata(percentiledata *pdata, const char *iface)
{
	uint32_t entry = 0, entrylimit;
	uint64_t *rxdata, *txdata, *sumdata;
	struct tm *d;
	char datebuff[DATEBUFFLEN];
	dbdatalist *datalist = NULL, *datalist_i = NULL;
	dbdatalistinfo datainfo;

	if (!db_getdata_range(&datalist, &datainfo, iface, "month", 1, "", "")) {
		printf("Error: Failed to fetch month data for 95th percentile.\n");
		return 0;
	}

	if (!datalist) {
		printf("No month data for 95th percentile available.\n");
		return 0;
	}

	pdata->monthbegin = datainfo.mintime;
	d = localtime(&pdata->monthbegin);
	strftime(datebuff, DATEBUFFLEN, "%Y-%m-%d", d);

	dbdatalistfree(&datalist);

	// limit query to a maximum of 8228 entries (31 days * 24 hours * 60 minutes / 5 minutes)
	if (!db_getdata_range(&datalist, &datainfo, iface, "fiveminute", 8928, datebuff, "")) {
		printf("Error: Failed to fetch 5 minute data for 95th percentile.\n");
		return 0;
	}

	if (!datalist) {
		printf("No 5 minute data for 95th percentile available.\n");
		return 0;
	}

	pdata->dataend = datainfo.maxtime;
	pdata->count = datainfo.count;
	pdata->countexpectation = (uint32_t)((pdata->dataend - pdata->monthbegin) / 300 + 1);
	pdata->minrx = datainfo.minrx;
	pdata->mintx = datainfo.mintx;
	pdata->min = datainfo.min;
	pdata->sumrx = datainfo.sumrx;
	pdata->sumtx = datainfo.sumtx;
	pdata->maxrx = datainfo.maxrx;
	pdata->maxtx = datainfo.maxtx;
	pdata->max = datainfo.max;

	entrylimit = lrint(datainfo.count * (float)0.95) - 1;

	if (debug) {
		printf("Entry expectation: %" PRIu32 "\n", pdata->countexpectation);
		printf("Entry count: %" PRIu32 "\n", pdata->count);
		printf("95th: %" PRIu32 "\n", entrylimit);
	}

	rxdata = (uint64_t *)malloc(datainfo.count * sizeof(uint64_t));
	txdata = (uint64_t *)malloc(datainfo.count * sizeof(uint64_t));
	sumdata = (uint64_t *)malloc(datainfo.count * sizeof(uint64_t));
	if (rxdata == NULL || txdata == NULL || sumdata == NULL) {
		panicexit(__FILE__, __LINE__);
	}

	datalist_i = datalist;
	while (datalist_i != NULL) {
		if (entry >= datainfo.count) {
			printf("Error: Database query resulted in more data than expected (%" PRIu32 " >= %" PRIu32 ").\n", entry, datainfo.count);
			free(rxdata);
			free(txdata);
			free(sumdata);
			dbdatalistfree(&datalist);
			return 0;
		}
		rxdata[entry] = datalist_i->rx;
		txdata[entry] = datalist_i->tx;
		sumdata[entry] = datalist_i->rx + datalist_i->tx;
		datalist_i = datalist_i->next;
		entry++;
	}

	if (datainfo.count != entry) {
		printf("Error: Database query data count doesn't match entry count (%" PRIu32 " != %" PRIu32 ").\n", datainfo.count, entry);
		free(rxdata);
		free(txdata);
		free(sumdata);
		dbdatalistfree(&datalist);
		return 0;
	}

	qsort((void *)rxdata, datainfo.count, sizeof(uint64_t), compare_uint64_t);
	qsort((void *)txdata, datainfo.count, sizeof(uint64_t), compare_uint64_t);
	qsort((void *)sumdata, datainfo.count, sizeof(uint64_t), compare_uint64_t);

	pdata->rxpercentile = rxdata[entrylimit];
	pdata->txpercentile = txdata[entrylimit];
	pdata->sumpercentile = sumdata[entrylimit];

	free(rxdata);
	free(txdata);
	free(sumdata);
	dbdatalistfree(&datalist);

	return 1;
}

// TODO: tests
int compare_uint64_t(const void *a, const void *b)
{
	if (*(uint64_t *)a < *(uint64_t *)b) {
		return -1;
	} else if (*(uint64_t *)a > *(uint64_t *)b) {
		return 1;
	}
	return 0;
}
