#ifndef IFLIST_H
#define IFLIST_H

typedef struct iflist {
	char interface[MAXIFLEN];
	int64_t id;
	uint32_t bandwidth;
	struct iflist *next;
} iflist;

int iflistadd(iflist **ifl, const char *iface, const int64_t id, const uint32_t bandwidth);
int iflistsearch(iflist **ifl, const char *iface);
void iflistfree(iflist **ifl);

#endif
