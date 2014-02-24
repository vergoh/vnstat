void printcfgfile(void);
int loadcfg(void);
void defaultcfg(void);
int getcfgvalue(FILE *fd, char *search);
int ibwadd(char *iface, int limit);
void ibwlist(void);
int ibwget(char *iface);
int ibwcfgread(FILE *fd);
