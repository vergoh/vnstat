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
int extractcfgvalue(char *value, const unsigned int valuelen, const char *cfgline, const unsigned int cfglen);
int setcfgvalue(const struct cfgsetting *cset, const char *value, const char *cfgline);
void configlocale(void);

#endif
