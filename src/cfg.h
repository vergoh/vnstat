void printcfgfile(void);
int loadcfg(char *cfgfile);
void defaultcfg(void);
int ibwadd(char *iface, int limit);
void ibwlist(void);
int ibwget(char *iface);
int ibwcfgread(FILE *fd);
