#ifndef CFG_H
#define CFG_H

void printcfgfile(void);
int loadcfg(const char *cfgfile);
void validatecfg(void);
void defaultcfg(void);
int ibwadd(const char *iface, int limit);
void ibwlist(void);
int ibwget(const char *iface);
void ibwflush(void);
int ibwcfgread(FILE *fd);

struct cfgsetting {
	const char *name;
	char *locc;
	short *loci;
	short namelen;
	short found;
};

#endif
