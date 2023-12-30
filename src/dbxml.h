#ifndef DBXML_H
#define DBXML_H

void showxml(const char *interface, const char mode, const char *databegin, const char *dataend);
void xmldump(const interfaceinfo *ifaceinfo, const char *tablename, const int datetype, const char *databegin, const char *dataend);
void xmlpercentile(const interfaceinfo *ifaceinfo);
void xmldate(const time_t *date, const int type);
void xmlheader(void);
void xmlfooter(void);

#endif
