#ifndef IFINFO_H
#define IFINFO_H

#include "iflist.h"

#if defined(BSD_VNSTAT)
#include <net/if.h>
#endif

int getifinfo(const char *iface);
int getifliststring(char **ifacelist, int showspeed);
int getiflist(iflist **ifl, int getspeed);
#if defined(__linux__) || defined(CHECK_VNSTAT)
int getiflist_linux(iflist **ifl, const int getspeed);
#elif defined(BSD_VNSTAT)
int getiflist_bsd(iflist **ifl, const int getspeed);
#endif
int readproc(const char *iface);
int readsysclassnet(const char *iface);
#if defined(BSD_VNSTAT)
int getifdata(const char *iface, struct if_data *ifd);
int readifaddrs(const char *iface);
#endif
uint32_t getifspeed(const char *iface);
int isifavailable(const char *iface);

#endif
