#ifndef CFG_H
#define CFG_H

struct cfgsetting {
	const char *name;
	char *locc;
	int32_t *loci;
	short namelen;
	short found;
};

void printcfgfile(void);
int loadcfg(const char *cfgfile);
void validatecfg(void);
void defaultcfg(void);
int opencfgfile(const char *cfgfile, FILE **fd);
int extractcfgvalue(char *value, const char *cfgline, int cfglen);
int setcfgvalue(struct cfgsetting *cset, const char *value, const char *cfgline);

#endif
