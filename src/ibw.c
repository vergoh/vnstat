#include "common.h"
#include "cfg.h"
#include "ifinfo.h"
#include "ibw.h"

int ibwloadcfg(const char *cfgfile)
{
	FILE *fd;
	int ret;

	ifacebw = NULL;

	ret = opencfgfile(cfgfile, &fd);
	if (ret != 2)
		return ret;

	rewind(fd);
	ibwcfgread(fd);
	fclose(fd);

	if (debug)
		ibwlist();

	return 1;
}

int ibwadd(const char *iface, const uint32_t limit)
{
	ibwnode *n, *p = ifacebw;

	/* add new node if list is empty */
	if (p == NULL) {

		n = (ibwnode *)malloc(sizeof(ibwnode));

		if (n == NULL) {
			return 0;
		}

		n->next = ifacebw;
		ifacebw = n;
		strncpy_nt(n->interface, iface, MAXIFLEN);
		n->limit = limit;
		n->fallback = limit;
		n->retries = 0;
		n->detected = 0;

	} else {

		/* update previous value if already in list */
		while (p != NULL) {
			if (strcmp(p->interface, iface) == 0) {
				p->limit = limit;
				return 1;
			}
			p = p->next;
		}

		/* add new node if not found */
		n = (ibwnode *)malloc(sizeof(ibwnode));

		if (n == NULL) {
			return 0;
		}

		n->next = ifacebw;
		ifacebw = n;
		strncpy_nt(n->interface, iface, MAXIFLEN);
		n->limit = limit;
		n->fallback = limit;
		n->retries = 0;
		n->detected = 0;
	}

	return 1;
}

void ibwlist(void)
{
	int i = 1;
	ibwnode *p = ifacebw;

	if (p == NULL) {
		printf("ibw list is empty.\n");
		return;
	}

	printf("ibw: ");
	while (p != NULL) {
		printf("%2d: i\"%s\" l%u f%u r%d d%u  ", i, p->interface, p->limit, p->fallback, p->retries, (unsigned int)p->detected);
		p = p->next;
		i++;
	}
	printf("\n");
}

int ibwget(const char *iface, uint32_t *limit)
{
	ibwnode *p = ifacebw;
	time_t current;
	uint32_t speed;

	*limit = 0;
	current = time(NULL);

	/* search for interface specific limit */
	while (p != NULL) {
		if (strcasecmp(p->interface, iface) == 0) {

			/* never override manually configured limits */
			if (p->detected == 0 && p->limit > 0) {
				*limit = p->limit;
				return 1;
			}

			if (!istun(iface) && cfg.bwdetection && p->retries < 5) {
				if (cfg.bwdetectioninterval > 0 && (current - p->detected) > (cfg.bwdetectioninterval * 60)) {
					speed = getifspeed(iface);
					if (speed > 0) {
						if (p->detected > 0 && speed != p->limit) {
							snprintf(errorstring, 1024, "Detected bandwidth limit for \"%s\" changed from %" PRIu32 " Mbit to %" PRIu32 " Mbit.", iface, p->limit, speed);
							printe(PT_Info);
						}
						p->limit = speed;
						p->retries = 0;
						p->detected = current;
						*limit = speed;
						return 1;
					}
					p->retries++;
				}
			}

			if (p->limit > 0) {
				*limit = p->limit;
				return 1;
			} else {
				return 0;
			}
		}
		p = p->next;
	}

	if (!istun(iface) && cfg.bwdetection) {
		if (ibwadd(iface, (uint32_t)cfg.maxbw)) {
			p = ibwgetnode(iface);
			if (p != NULL) {
				speed = getifspeed(iface);
				if (speed > 0) {
					p->limit = speed;
					p->retries = 0;
					p->detected = current;
					*limit = speed;
					return 1;
				}
				p->retries++;
			}
		}
	}

	/* return default limit if specified */
	if (cfg.maxbw > 0) {
		*limit = (uint32_t)cfg.maxbw;
		return 1;
	}
	return 0;
}

ibwnode *ibwgetnode(const char *iface)
{
	ibwnode *p = ifacebw;

	while (p != NULL) {
		if (strcasecmp(p->interface, iface) == 0) {
			return p;
		}
		p = p->next;
	}
	return NULL;
}

void ibwflush(void)
{
	ibwnode *f, *p = ifacebw;

	while (p != NULL) {
		f = p;
		p = p->next;
		free(f);
	}

	ifacebw = NULL;
}

int ibwcfgread(FILE *fd)
{
	char cfgline[512], name[512], value[512];
	int i, j, linelen, count = 0;
	long ivalue;

	/* start from value search from first line */
	rewind(fd);

	/* cycle all lines */
	while (!feof(fd)) {

		cfgline[0] = '\0';

		/* get current line */
		if (fgets(cfgline, 512, fd) == NULL) {
			break;
		}

		linelen = (int)strlen(cfgline);

		if (linelen <= 8 || cfgline[0] == '#') {
			continue;
		}

		if (strncasecmp(cfgline, "MaxBW", 5) != 0) {
			continue;
		}

		/* clear name and value buffers */
		for (j = 0; j < 512; j++) {
			name[j] = value[j] = '\0';
		}

		/* get interface name */
		j = 0;
		for (i = 5; i < linelen; i++) {
			if (cfgline[i] == ' ' || cfgline[i] == '=' || cfgline[i] == '\t' || cfgline[i] == '\n' || cfgline[i] == '\r') {
				break;
			} else {
				name[j] = cfgline[i];
				j++;
			}
		}

		/* get new line if no usable name was found */
		if (strlen(name) == 0) {
			continue;
		}

		/* search value */
		j = 0;
		for (i++; i < linelen; i++) {
			if (cfgline[i] == '\n' || cfgline[i] == '\r') {
				break;
			} else if (cfgline[i] == '\"') {
				if (j == 0) {
					continue;
				} else {
					break;
				}
			} else {
				if (j == 0 && (cfgline[i] == ' ' || cfgline[i] == '=' || cfgline[i] == '\t')) {
					continue;
				} else {
					value[j] = cfgline[i];
					j++;
				}
			}
		}

		/* get new line if no usable value was found */
		if ((strlen(value) == 0) || (!isdigit(value[0]))) {
			continue;
		}

		/* add interface and limit to list if value is within limits */
		ivalue = strtol(value, (char **)NULL, 0);
		if (ivalue < 0 || ivalue > BWMAX) {
			snprintf(errorstring, 1024, "Invalid value \"%ld\" for MaxBW%s, ignoring parameter.", ivalue, name);
			printe(PT_Config);
		} else {
			ibwadd(name, (uint32_t)ivalue);
		}
	}

	return count;
}
