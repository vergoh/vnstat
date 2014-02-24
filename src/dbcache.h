#ifndef DBCACHE_H
#define DBCACHE_H

int dataadd(char *iface);
int dataupdate(void);
void datashow(void);
int dataget(char *iface);
void dataflush(char *dirname);
int datacount(void);
uint32_t dbcheck(uint32_t dbhash);
uint32_t simplehash(const char *data, int len);

typedef struct datanode {
	DATA data;
	short filled;
	struct datanode *next;
} datanode;

/* global variables */
datanode *dataptr;

#endif
