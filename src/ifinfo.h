#ifndef IFINFO_H
#define IFINFO_H

int getifinfo(char iface[32]);
int getiflist(char **ifacelist);
int readproc(char iface[32]);
int readsysclassnet(char iface[32]);
void parseifinfo(int newdb);
uint64_t countercalc(uint64_t a, uint64_t b);
#if defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__) || defined(__APPLE__)
int readifaddrs(char iface[32]);
#endif

#endif
