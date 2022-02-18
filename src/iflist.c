#include "common.h"
#include "iflist.h"

int iflistadd(iflist **ifl, const char *iface, const uint32_t bandwidth)
{
	iflist *newif = NULL, *ifl_iterator = *ifl;

	newif = malloc(sizeof(iflist));
	if (newif == NULL) {
		return 0;
	}

	newif->next = NULL;

	if (*ifl != NULL) {
		while (ifl_iterator->next != NULL) {
			ifl_iterator = ifl_iterator->next;
		}
		ifl_iterator->next = newif;
	} else {
		*ifl = newif;
	}

	strncpy_nt(newif->interface, iface, MAXIFLEN);
	newif->bandwidth = bandwidth;

	return 1;
}

int iflistsearch(iflist **ifl, const char *iface)
{
	iflist *ifl_iterator = *ifl;

	while (ifl_iterator != NULL) {
		if (strcmp(iface, ifl_iterator->interface) == 0) {
			return 1;
		}
		ifl_iterator = ifl_iterator->next;
	}
	return 0;
}

void iflistfree(iflist **ifl)
{
	iflist *ifl_prev;

	while (*ifl != NULL) {
		ifl_prev = *ifl;
		*ifl = (*ifl)->next;
		free(ifl_prev);
	}
}
