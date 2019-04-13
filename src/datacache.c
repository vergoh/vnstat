#include "common.h"
#include "datacache.h"

int datacache_add(datacache **dc, const char *interface, const short sync)
{
	datacache *newdc;

	newdc = malloc(sizeof(datacache));
	if (newdc == NULL) {
		return 0;
	}
	newdc->next = *dc;
	*dc = newdc;

	strncpy_nt((*dc)->interface, interface, 32);
	(*dc)->active = 1;
	(*dc)->filled = 0;
	(*dc)->syncneeded = sync;
	(*dc)->currx = 0;
	(*dc)->curtx = 0;
	(*dc)->updated = time(NULL);
	(*dc)->log = NULL;

	return 1;
}

int datacache_seek(datacache **dc, const char *interface)
{
	while (*dc != NULL) {
		if (strcmp((*dc)->interface, interface) == 0) {
			return 1;
		}
		*dc = (*dc)->next;
	}
	return 0;
}

int datacache_remove(datacache **dc, const char *interface)
{
	int ret = 0;
	datacache *dc_prev, *dc_head;

	dc_head = *dc;
	dc_prev = *dc;

	if (*dc == NULL) {
		return ret;
	}

	/* handle list head remove */
	if (strcmp((*dc)->interface, interface) == 0) {
		*dc = (*dc)->next;
		xferlog_clear(&dc_prev->log);
		free(dc_prev);
		return 1;
	}

	*dc = (*dc)->next;

	/* handle other locations */
	while (*dc != NULL) {
		if (strcmp((*dc)->interface, interface) == 0) {
			dc_prev->next = (*dc)->next;
			xferlog_clear(&(*dc)->log);
			free(*dc);
			ret = 1;
			break;
		}
		dc_prev = *dc;
		*dc = (*dc)->next;
	}

	*dc = dc_head;
	return ret;
}

void datacache_clear(datacache **dc)
{
	datacache *dc_prev;

	while (*dc != NULL) {
		dc_prev = *dc;
		*dc = (*dc)->next;
		xferlog_clear(&dc_prev->log);
		free(dc_prev);
	}
}

int datacache_count(datacache **dc)
{
	int count = 0;
	datacache *cacheiterator = *dc;

	while (cacheiterator != NULL) {
		count++;
		cacheiterator = cacheiterator->next;
	}
	return count;
}

int datacache_activecount(datacache **dc)
{
	int count = 0;
	datacache *cacheiterator = *dc;

	while (cacheiterator != NULL) {
		if (cacheiterator->active) {
			count++;
		}
		cacheiterator = cacheiterator->next;
	}
	return count;
}

void datacache_debug(datacache **dc)
{
	int i = 1;
	datacache *cacheiterator = *dc;

	if (cacheiterator == NULL) {
		printf("cache: empty\n");
		return;
	}

	printf("cache: ");
	while (cacheiterator != NULL) {
		printf(" %d: \"%s\" (", i, cacheiterator->interface);
		xferlog_debug(&cacheiterator->log, 0);
		printf(")  ");
		cacheiterator = cacheiterator->next;
		i++;
	}
	printf("\n");
}

int xferlog_add(xferlog **log, const time_t timestamp, const uint64_t rx, const uint64_t tx)
{
	xferlog *newlog;

	if (*log == NULL || (*log)->timestamp != timestamp) {
		newlog = malloc(sizeof(xferlog));
		if (newlog == NULL) {
			return 0;
		}
		newlog->next = *log;
		*log = newlog;

		newlog->timestamp = timestamp;
		newlog->rx = 0;
		newlog->tx = 0;
	}

	(*log)->rx += rx;
	(*log)->tx += tx;

	return 1;
}

void xferlog_clear(xferlog **log)
{
	xferlog *log_prev;

	while (*log != NULL) {
		log_prev = *log;
		*log = (*log)->next;
		free(log_prev);
	}
}

void xferlog_debug(xferlog **log, const int newline)
{
	int i = 1;
	xferlog *logiterator = *log;

	if (newline && logiterator == NULL) {
		printf("  xferlog: empty\n");
		return;
	}

	if (newline) {
		printf("  xferlog: ");
	}
	while (logiterator != NULL) {
		printf("%d: %" PRIu64 " - %" PRIu64 " / %" PRIu64 "", i, (uint64_t)logiterator->timestamp, logiterator->rx, logiterator->tx);
		if (logiterator->next != NULL) {
			printf(", ");
		}
		logiterator = logiterator->next;
		i++;
	}
	if (newline) {
		printf("\n");
	}
}
