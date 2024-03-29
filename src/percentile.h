#ifndef PERCENTILE_H
#define PERCENTILE_H

typedef struct percentiledata {
	time_t monthbegin, databegin, dataend;
	uint32_t count, countexpectation;
	uint64_t minrx, mintx;
	uint64_t maxrx, maxtx;
	uint64_t min, max;
	uint64_t sumrx, sumtx;
	uint64_t rxpercentile, txpercentile, sumpercentile;
	uint64_t userlimitbytespersecond;
	uint32_t countrxoveruserlimit, counttxoveruserlimit, countsumoveruserlimit;
} percentiledata;

int getpercentiledata(percentiledata *pdata, const char *iface, const uint64_t userlimitbytespersecond);
int compare_uint64_t(const void *a, const void *b);

#endif
