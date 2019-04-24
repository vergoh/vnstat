#ifndef CFG_H
#define CFG_H

struct cfgsetting {
	const char *name;
	char *locc;
	int32_t *loci;
	short namelen;
	short found;
};

int loadcfg(const char *cfgfile);
void validatebool(const char *cfgname, int32_t *cfgptr, const int32_t defaultvalue);
void validateint(const char *cfgname, int32_t *cfgptr, const int32_t defaultvalue, const int32_t minvalue, const int32_t maxvalue);
void validatecfg(void);
void defaultcfg(void);
int opencfgfile(const char *cfgfile, FILE **fd);
int extractcfgvalue(char *value, const unsigned int valuelen, const char *cfgline, const unsigned int cfglen);
int setcfgvalue(const struct cfgsetting *cset, const char *value, const char *cfgline);
void configlocale(void);

#endif
