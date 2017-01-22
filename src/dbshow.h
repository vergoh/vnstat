#ifndef DBSHOW_H
#define DBSHOW_H

#define DATEBUFFLEN 64

void showdb(const char *interface, int qmode);
void showsummary(interfaceinfo *interface);
int showbar(uint64_t rx, uint64_t tx, uint64_t max, const int len);
void indent(int i);

#endif
