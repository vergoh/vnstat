#ifndef DATACACHE_H
#define DATACACHE_H

typedef struct datacache {
	char interface[32];
	short active;
	short filled;
	short syncneeded;
	uint64_t currx, curtx;
	time_t updated;
	struct xferlog *log;
	struct datacache *next;
} datacache;

typedef struct xferlog {
	time_t timestamp;
	uint64_t rx, tx;
	struct xferlog *next;
} xferlog;

int datacache_add(datacache **dc, const char *interface, const short sync);
int datacache_seek(datacache **dc, const char *interface);
int datacache_remove(datacache **dc, const char *interface);
void datacache_clear(datacache **dc);
int datacache_count(datacache **dc);
int datacache_activecount(datacache **dc);
void datacache_debug(datacache **dc);

int xferlog_add(xferlog **log, const time_t timestamp, const uint64_t rx, const uint64_t tx);
void xferlog_clear(xferlog **log);
void xferlog_debug(xferlog **log, const int newline);

#endif
