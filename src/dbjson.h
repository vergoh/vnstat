#ifndef DBJSON_H
#define DBJSON_H

void showjson(const char *interface, const int ifcount, const char mode, const char *databegin, const char *dataend);
void jsondump(const interfaceinfo *interface, const char *tablename, const int datetype, const char *databegin, const char *dataend);
void jsondate(const time_t *date, const int type);
void jsonheader(void);
void jsonfooter(void);

#endif
