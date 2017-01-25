#ifndef DBJSON_H
#define DBJSON_H

void showjson(const char *interface, const int dbcount, const char mode);
void jsondump(const interfaceinfo *interface, const char *tablename, const int datetype);
void jsondate(const time_t *date, const int type);
void jsonheader(void);
void jsonfooter(void);

#endif
