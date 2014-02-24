#include "common.h"
#include "ifinfo.h"
#include "dbaccess.h"
#include "dbcache.h"

int cacheadd(const char *iface, int sync)
{
	datanode *p, *n;

	p = dataptr;

	/* skip if already in list */
	while (p != NULL) {
		if (strcmp(p->data.interface, iface)==0) {
			if (debug) {
				printf("cache: %s already cached\n", iface);
			}
			return 1;
		}
		p = p->next;
	}

	/* add new node if not in list */
	n = malloc(sizeof(datanode));

	if (n == NULL) {
		return 0;
	}

	n->next = dataptr;
	dataptr = n;
	strncpy(n->data.interface, iface, 32);
	n->data.interface[31] = '\0';
	n->data.active = 1;
	n->filled = 0;
	n->sync = sync;

	if (debug) {
		printf("cache: %s added\n", iface);
	}

	return 1;
}

datanode *cacheremove(const char *iface)
{
	datanode *p, *o;

	p = dataptr;

	if (p == NULL) {
		return NULL;
	} else {

		/* handle list head remove */
		if (strcmp(p->data.interface, iface)==0) {
			dataptr = p->next;
			if (debug) {
				printf("cache: h %s removed\n", iface);
			}
			free(p);
			return dataptr;
		}

		o = p;
		p = p->next;

		/* handle other locations */
		while (p != NULL) {

			if (strcmp(p->data.interface, iface)==0) {
				o->next = p->next;
				if (debug) {
					printf("cache: %s removed\n", iface);
				}
				free(p);
				return o->next;
			}

			o = p;
			p = p->next;
		}
	}

	return NULL;
}

int cacheupdate(void)
{
	datanode *p, *n;

	p = dataptr;

	/* update if already in list */
	while (p != NULL) {
		if (strcmp(p->data.interface, data.interface)==0) {
			if (memcpy(&p->data, &data, sizeof(p->data))!=NULL) {
				p->filled = 1;
			} else {
				p->filled = 0;
			}
			if (debug) {
				printf("cache: %s updated (%d)\n", p->data.interface, p->filled);
			}
			return p->filled;
		}
		p = p->next;
	}

	/* add new node if not in list */
	n = malloc(sizeof(datanode));

	if (n == NULL) {
		return 0;
	}

	n->next = dataptr;
	dataptr = n;
	if (memcpy(&n->data, &data, sizeof(n->data))!=NULL) {
		n->filled = 1;
	} else {
		n->filled = 0;
	}

	if (debug) {
		printf("cache: %s added and updated (%d)\n", p->data.interface, p->filled);
	}

	return n->filled;
}

void cacheshow(void)
{
	int i = 1;
	datanode *p = dataptr;

	if (p == NULL) {
		printf("cache: empty.\n");

	} else {

		printf("cache:");
		while (p != NULL) {
			printf(" %d. \"%s\"  ", i, p->data.interface);
			p = p->next;
			i++;
		}
		printf("\n");
	}
}

void cachestatus(void)
{
	char buffer[512];
	int b = 13, count = 0;
	datanode *p = dataptr;

	snprintf(buffer, b, "Monitoring: ");

	if (p != NULL) {

		while (p != NULL) {
			if ((b+strlen(p->data.interface)+1) < 508) {
				strncat(buffer, p->data.interface, strlen(p->data.interface));
				strcat(buffer, " ");
				b = b+strlen(p->data.interface)+1;
			} else {
				strcat(buffer, "...");
				break;
			}
			count++;
			p = p->next;
		}
	}

	if (count) {
		strncpy(errorstring, buffer, 512);
		errorstring[511] = '\0';
	} else {
		snprintf(errorstring, 512, "Nothing to monitor");
	}
	printe(PT_Info);
}

int cacheget(datanode *dn)
{
	if (dn->filled) {
		memcpy(&data, &dn->data, sizeof(data));

		/* do simple data validation */
		if (data.version != DBVERSION ||
			data.created == 0 ||
			data.lastupdated == 0 ||
			data.interface[0] == '\0' ||
			data.active > 1 ||
			data.active < 0) {

			if (debug)
				printf("cache get: validation failed (%d/%u/%u/%d/%d)\n", data.version, (unsigned int)data.created, (unsigned int)data.lastupdated, data.interface[0], data.active);

			/* force reading of database file */
			dn->filled = 0;
		}
	}

	if (debug) {
		printf("cache get: %s (%d/%d)\n", dn->data.interface, dn->filled, dn->data.active);
	}

	return dn->filled;
}

/* flush cached data to disk and free all memory allocted for it */
void cacheflush(const char *dirname)
{
	datanode *f, *p = dataptr;

	while (p != NULL) {
		f = p;
		p = p->next;

		/* write data to file if needed */
		if (f->filled && dirname!=NULL) {
			memcpy(&data, &f->data, sizeof(data));
			writedb(f->data.interface, dirname, 0);
		}

		free(f);
	}

	dataptr = NULL;
}

int cachecount(void)
{
	datanode *p = dataptr;
	int c = 0;

	while (p != NULL) {
		c++;
		p = p->next;
	}

	return c;
}

int cacheactivecount(void)
{
	datanode *p = dataptr;
	int c = 0;

	while (p != NULL) {
		if (p->data.active) {
			c++;
		}
		p = p->next;
	}

	return c;
}

uint32_t dbcheck(uint32_t dbhash, int *forcesave)
{
	char *ifacelist, interface[32];
	datanode *p = dataptr;
	uint32_t newhash;
	int offset, found;

	/* get list of currently visible interfaces */
	if (getiflist(&ifacelist)==0) {
		return 0;
	}

	newhash = simplehash(ifacelist, (int)strlen(ifacelist));

	/* search for changes if hash doesn't match */
	if (newhash!=dbhash) {

		if (debug) {
			printf("ifacelist changed: '%s'    %u <> %u\n", ifacelist, dbhash, newhash);
		}

		while (p != NULL) {

			if (p->filled) {
				found = offset = 0;

				while (offset <= (int)strlen(ifacelist)) {
					sscanf(ifacelist+offset, "%32s", interface);
					if (strcmp(p->data.interface, interface)==0) {
						found = 1;
						break;
					}
					offset += (int)strlen(interface)+1;
				}

				if (p->data.active==1 && found==0) {
					p->data.active = 0;
					p->data.currx = p->data.curtx = 0;
					if (cfg.savestatus) {
						*forcesave = 1;
					}
					snprintf(errorstring, 512, "Interface \"%s\" disabled.", p->data.interface);
					printe(PT_Info);
				} else if (p->data.active==0 && found==1) {
					p->data.active = 1;
					p->data.currx = p->data.curtx = 0;
					if (cfg.savestatus) {
						*forcesave = 1;
					}
					snprintf(errorstring, 512, "Interface \"%s\" enabled.", p->data.interface);
					printe(PT_Info);
				}
			}
			p = p->next;
		}
	}

	free(ifacelist);

	return newhash;
}

uint32_t simplehash(const char *data, int len)
{
	uint32_t hash = len;

	if (len <= 0 || data == NULL) {
		return 0;
	}

	for (len--; len >= 0; len--) {
		if (len > 0) {
			hash += (int)data[len] * len;
		} else {
			hash += (int)data[len];
		}
	}

	return hash;
}
