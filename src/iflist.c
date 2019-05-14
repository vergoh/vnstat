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

void iflistfree(iflist **ifl)
{
	iflist *ifl_prev;

	while (*ifl != NULL) {
		ifl_prev = *ifl;
		*ifl = (*ifl)->next;
		free(ifl_prev);
	}
}
