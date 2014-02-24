#include "common.h"
#include "ifinfo.h"
#include "dbaccess.h"
#include "dbcache.h"

int dataadd(char *iface)
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
	n->filled = 0;

	if (debug) {
		printf("cache: %s added\n", iface);
	}

	return 1;
}

int dataupdate(void)
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

void datashow(void)
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

int dataget(char *iface)
{
	datanode *p = dataptr;
	
	if (debug) {
		printf("cache search: %s... ", iface);
	}
	
	while (p != NULL) {
		if (strcmp(p->data.interface, iface)==0) {
			if (p->filled) {
				memcpy(&data, &p->data, sizeof(data));
			}
			if (debug) {
				printf("found (%d)\n", p->filled);
			}
			return p->filled;
		}
		p = p->next;
	}

	if (debug) {
		printf("not found\n");
	}
	return 0;
}

/* flush cached data to disk and free all memory allocted for it */
void dataflush(char *dirname)
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

int datacount(void)
{
	datanode *p = dataptr;
	int i = 0;

	while (p != NULL) {
		i++;
		p = p->next;
	}
	
	return i;
}

uint32_t dbcheck(uint32_t dbhash)
{
	char *ifacelist, interface[32];
	datanode *p = dataptr;
	uint32_t newhash;
	int offset, found;

	/* get list of currently visible interfaces */
	if (getiflist(&ifacelist)==0) {
		return 0;
	}

	newhash = simplehash(ifacelist, strlen(ifacelist));

	/* search for changes if hash doesn't match */
	if (newhash!=dbhash) {

		if (debug) {
			printf("ifacelist changed: '%s'    %u <> %u\n", ifacelist, dbhash, newhash);
		}

		while (p != NULL) {

			if (p->filled) {
				found = offset = 0;
			
				while (offset <= strlen(ifacelist)) {
					sscanf(ifacelist+offset, "%32s", interface);
					if (strcmp(p->data.interface, interface)==0) {
						found = 1;
						break;
					}
					offset += strlen(interface)+1;
				}
				
				if (p->data.active==1 && found==0) {
					p->data.active = 0;
					p->data.currx = p->data.curtx = 0;
					snprintf(errorstring, 512, "Interface \"%s\" disabled.", p->data.interface);
					printe(PT_Info);
				} else if (p->data.active==0 && found==1) {
					p->data.active = 1;
					p->data.currx = p->data.curtx = 0;
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
