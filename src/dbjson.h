#ifndef DBJSON_H
#define DBJSON_H

#include "dbsql.h"
#include "dbshow.h"
#include "percentile.h"

void showjson(const char *interface, const int ifcount, const char mode, const char *databegin, const char *dataend);
void jsondump(const interfaceinfo *ifaceinfo, const char *tablename, const int datetype, const char *databegin, const char *dataend);
void jsonpercentile(const interfaceinfo *ifaceinfo);
void jsonpercentileminavgmax(const percentiledata *pdata);
void jsonalertoutput(const alertdata *adata, const AlertOutput output, const AlertType type, const AlertCondition condition, const uint64_t limit);
void jsonalert(const alertdata *adata, const uint64_t limit);
void jsonpercentilealert(const alertdata *adata, const AlertCondition condition, const uint64_t limit);
void jsoninterfaceinfo(const interfaceinfo *info);
void jsondate(const time_t *date, const int type);
void jsonheader(const char *version);
void jsonfooter(void);

#endif
