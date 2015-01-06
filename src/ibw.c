#include "common.h"
#include "ibw.h"

int ibwloadcfg(const char *cfgfile)
{
	FILE *fd;
	char buffer[512];
	int i, tryhome;

	ifacebw = NULL;

	/* clear buffer */
	for (i=0; i<512; i++) {
		buffer[i] = '\0';
	}

	/* possible config files: 1) --config   2) $HOME/.vnstatrc   3) /etc/vnstat.conf   4) none */

	if (cfgfile[0]!='\0') {

		/* try to open given file */
		if ((fd=fopen(cfgfile, "r"))!=NULL) {
			if (debug)
				printf("IBW Config file: --config\n");
		} else {
			snprintf(errorstring, 512, "Unable to open given ibw config file \"%s\": %s\n", cfgfile, strerror(errno));
			printe(PT_Error);
			return 0;
		}

	} else {

		if (getenv("HOME")) {
			strncpy_nt(buffer, getenv("HOME"), 500);
			strcat(buffer, "/.vnstatrc");
			tryhome = 1;
		} else {
			tryhome = 0;
		}

		/* try to open first available config file */
		if (tryhome && (fd=fopen(buffer, "r"))!=NULL) {
			if (debug)
				printf("IBW Config file: $HOME/.vnstatrc\n");
		} else if ((fd=fopen("/etc/vnstat.conf", "r"))!=NULL) {
			if (debug)
				printf("IBW Config file: /etc/vnstat.conf\n");
		} else if ((fd=fopen("/usr/local/etc/vnstat.conf", "r"))!=NULL) {
			if (debug)
				printf("IBW Config file: /usr/local/etc/vnstat.conf\n");
		} else {
			if (debug)
				printf("IBW Config file: none\n");
			return 1;
		}
	}

	rewind(fd);
	ibwcfgread(fd);
	fclose(fd);

	if (debug)
		ibwlist();

	return 1;
}

int ibwadd(const char *iface, int limit)
{
	ibwnode *n, *p = ifacebw;

	/* add new node if list is empty */
	if (p == NULL) {

		n = malloc(sizeof(ibwnode));

		if (n == NULL) {
			return 0;
		}

		n->next = ifacebw;
		ifacebw = n;
		strncpy_nt(n->interface, iface, 32);
		n->limit = limit;
		n->fallback = limit;
		n->retries = 0;

	} else {

		/* update previous value if already in list */
		while (p != NULL) {
			if (strcmp(p->interface, iface)==0) {
				p->limit = limit;
				p->retries = 0;
				return 1;
			}
			p = p->next;
		}

		/* add new node if not found */
		n = malloc(sizeof(ibwnode));

		if (n == NULL) {
			return 0;
		}

		n->next = ifacebw;
		ifacebw = n;
		strncpy_nt(n->interface, iface, 32);
		n->limit = limit;
		n->fallback = limit;
		n->retries = 0;
	}

	return 1;
}

void ibwlist(void)
{
	int i=1;
	ibwnode *p = ifacebw;

	if (p == NULL) {
		printf("ibw list is empty.\n");
		return;
	}

	printf("ibw:\n");
	while (p != NULL) {
		printf(" %2d: \"%s\" \"%d\"\n", i, p->interface, p->limit);
		p = p->next;
		i++;
	}
}

int ibwget(const char *iface)
{
	ibwnode *p = ifacebw;

	/* search for interface specific limit */
	while (p != NULL) {
		if (strcasecmp(p->interface, iface)==0) {
			if (p->limit>0) {
				return p->limit;
			} else {
				return -1;
			}
		}
		p = p->next;
	}

	/* return default limit if specified */
	if (cfg.maxbw>0) {
		return cfg.maxbw;
	} else {
		return -1;
	}
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
	int i, j, linelen, count = 0, ivalue;

	/* start from value search from first line */
	rewind(fd);

	/* cycle all lines */
	while (!feof(fd)) {

		cfgline[0] = '\0';

		/* get current line */
		if (fgets(cfgline, 512, fd)==NULL) {
			break;
		}

		linelen = (int)strlen(cfgline);

		if (linelen<=8 || cfgline[0]=='#') {
			continue;
		}

		if (strncasecmp(cfgline, "MaxBW", 5)!=0) {
			continue;
		}

		/* clear name and value buffers */
		for (j=0; j<512; j++) {
			name[j]=value[j]='\0';
		}

		/* get interface name */
		j=0;
		for (i=5; i<linelen; i++) {
			if (cfgline[i]==' ' || cfgline[i]=='=' || cfgline[i]=='\t' || cfgline[i]=='\n' || cfgline[i]=='\r') {
				break;
			} else {
				name[j]=cfgline[i];
				j++;
			}
		}

		/* get new line if no usable name was found */
		if (strlen(name)==0) {
			continue;
		}

		/* search value */
		j=0;
		for (i++; i<linelen; i++) {
			if (cfgline[i]=='\n' || cfgline[i]=='\r') {
				break;
			} else if (cfgline[i]=='\"') {
				if (j==0) {
					continue;
				} else {
					break;
				}
			} else {
				if (j==0 && (cfgline[i]==' ' || cfgline[i]=='=' || cfgline[i]=='\t')) {
					continue;
				} else {
					value[j]=cfgline[i];
					j++;
				}
			}
		}

		/* get new line if no usable value was found */
		if ((strlen(value)==0) || (!isdigit(value[0])) ) {
			continue;
		}

		/* add interface and limit to list if value is within limits */
		ivalue = atoi(value);
		if (ivalue<0 || ivalue>BWMAX) {
			snprintf(errorstring, 512, "Invalid value \"%d\" for MaxBW%s, ignoring parameter.", ivalue, name);
			printe(PT_Config);
		} else {
			ibwadd(name, ivalue);
		}
	}

	return count;
}
