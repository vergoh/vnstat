#include "common.h"
#include "dbsql.h"
#include "percentile.h"

// TODO: tests
int getpercentiledata(percentiledata *pdata, const char *iface, const uint64_t userlimitbytespersecond)
{
	uint32_t entry = 0, entrylimit;
	uint64_t *rxdata, *txdata, *sumdata;
	struct tm *d;
	char datebuff[DATEBUFFLEN];
	dbdatalist *datalist = NULL, *datalist_i = NULL;
	dbdatalistinfo datainfo;

	pdata->userlimitbytespersecond = userlimitbytespersecond;
	pdata->countrxoveruserlimit = 0;
	pdata->counttxoveruserlimit = 0;
	pdata->countsumoveruserlimit = 0;

	if (!db_getdata_range(&datalist, &datainfo, iface, "month", 1, "", "")) {
		snprintf(errorstring, 1024, "Failed to fetch month data for 95th percentile.");
		printe(PT_Error);
		return 0;
	}

	if (!datalist) {
		snprintf(errorstring, 1024, "No month data for 95th percentile available.");
		printe(PT_Error);
		return 0;
	}

	pdata->monthbegin = datainfo.mintime;
	d = localtime(&pdata->monthbegin);
	strftime(datebuff, DATEBUFFLEN, "%Y-%m-%d", d);

	dbdatalistfree(&datalist);

	/* limit query to a maximum of 8928 entries (31 days * 24 hours * 60 minutes / 5 minutes) */
	if (!db_getdata_range(&datalist, &datainfo, iface, "fiveminute", 8928, datebuff, "")) {
		snprintf(errorstring, 1024, "Failed to fetch 5 minute data for 95th percentile.");
		printe(PT_Error);
		return 0;
	}

	if (!datalist) {
		snprintf(errorstring, 1024, "No 5 minute data for 95th percentile available.");
		printe(PT_Error);
		return 0;
	}

	pdata->databegin = datainfo.mintime;
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
			snprintf(errorstring, 1024, "Database query resulted in more data than expected (%" PRIu32 " >= %" PRIu32 ").", entry, datainfo.count);
			printe(PT_Error);
			free(rxdata);
			free(txdata);
			free(sumdata);
			dbdatalistfree(&datalist);
			return 0;
		}
		rxdata[entry] = datalist_i->rx;
		txdata[entry] = datalist_i->tx;
		sumdata[entry] = datalist_i->rx + datalist_i->tx;

		if (userlimitbytespersecond > 0) {
			if (rxdata[entry] > userlimitbytespersecond * 300) {
				pdata->countrxoveruserlimit++;
			}
			if (txdata[entry] > userlimitbytespersecond * 300) {
				pdata->counttxoveruserlimit++;
			}
			if (sumdata[entry] > userlimitbytespersecond * 300) {
				pdata->countsumoveruserlimit++;
			}
		}

		datalist_i = datalist_i->next;
		entry++;
	}

	if (datainfo.count != entry) {
		snprintf(errorstring, 1024, "Database query data count doesn't match entry count (%" PRIu32 " != %" PRIu32 ").", datainfo.count, entry);
		printe(PT_Error);
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

int compare_uint64_t(const void *a, const void *b)
{
	if (*(uint64_t *)a < *(uint64_t *)b) {
		return -1;
	} else if (*(uint64_t *)a > *(uint64_t *)b) {
		return 1;
	}
	return 0;
}
