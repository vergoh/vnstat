#ifndef DBMERGE_H
#define DBMERGE_H

int dbmerge(const char *srcdbfile, const char *srciface, const char *dstdbfile, const char *dstiface, const int execute);
int mergeinterface(sqlite3 *srcdb, const char *srciface, sqlite3 *dstdb, const char *dstiface);
int mergedata(const char *table, const sqlite3_int64 ifaceid, const uint64_t rx, const uint64_t tx, const time_t timestamp);

#endif
