#ifndef CFG_H
#define CFG_H

struct cfgsetting {
	const char *name;
	char *locc;
	int32_t *loci;
	short namelen;
	short found;
};

typedef enum ConfigType {
	CT_All = 0,
	CT_CLI,
	CT_Daemon,
	CT_Image
} ConfigType;

int loadcfg(const char *cfgfile, const ConfigType type);
void validatebool(const char *cfgname, int32_t *cfgptr, const int32_t defaultvalue);
void validateint(const char *cfgname, int32_t *cfgptr, const int32_t defaultvalue, const int32_t minvalue, const int32_t maxvalue);
void validatecfg(const ConfigType type);
void defaultcfg(void);
int opencfgfile(const char *cfgfile, FILE **fd);
int extractcfgvalue(char *value, const unsigned int valuelen, const char *cfgline, const unsigned int cfglen);
int setcfgvalue(const struct cfgsetting *cset, const char *value, const char *cfgline);
void configlocale(void);

#endif
