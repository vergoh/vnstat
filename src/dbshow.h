#ifndef DBSHOW_H
#define DBSHOW_H

void showdb(int qmode);
void showsummary(void);
void showshort(void);
void showdays(void);
void showmonths(void);
void showtop(void);
void showweeks(void);
void showhours(void);
void showoneline(void);
void dumpdb(void);
void showbar(uint64_t rx, int rxk, uint64_t tx, int txk, uint64_t max, int len);
void indent(int i);

#endif
