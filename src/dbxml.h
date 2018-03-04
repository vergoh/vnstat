#ifndef DBXML_H
#define DBXML_H

void showxml(const char *interface, const char mode);
void xmldump(const interfaceinfo *interface, const char *tablename, const int datetype);
void xmldate(const time_t *date, const int type);
void xmlheader(void);
void xmlfooter(void);

#endif
