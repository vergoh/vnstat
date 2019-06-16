#include "common.h"
#include "iflist.h"

int iflistadd(iflist **ifl, const char *iface, const uint32_t bandwidth)
{
	iflist *newif;

	newif = malloc(sizeof(iflist));
	if (newif == NULL) {
		return 0;
	}

	newif->next = *ifl;
	newif->prev = NULL;

	if (*ifl != NULL) {
		(*ifl)->prev = newif;
	}

	*ifl = newif;

	strncpy_nt(newif->interface, iface, 32);
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
