#ifndef DBSHOW_H
#define DBSHOW_H

#define DATEBUFFLEN 64

typedef struct {
	time_t date;
	uint64_t rx, tx;
} HOURDATA;

void showdb(const char *interface, int qmode, const char *databegin, const char *dataend);
void showsummary(const interfaceinfo *interface, const int shortmode);
void showlist(const interfaceinfo *interface, const char *listname, const char *databegin, const char *dataend);
void showoneline(const interfaceinfo *interface);
void showhours(const interfaceinfo *interface);
int showbar(uint64_t rx, uint64_t tx, uint64_t max, const int len);
void indent(int i);

#endif
