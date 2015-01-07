#ifndef CFG_H
#define CFG_H

void printcfgfile(void);
int loadcfg(const char *cfgfile);
void validatecfg(void);
void defaultcfg(void);
int opencfgfile(const char *cfgfile, FILE **fd);

struct cfgsetting {
	const char *name;
	char *locc;
	short *loci;
	short namelen;
	short found;
};

#endif
