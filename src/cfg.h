#ifndef CFG_H
#define CFG_H

void printcfgfile(void);
int loadcfg(char *cfgfile);
void validatecfg(void);
void defaultcfg(void);
int ibwadd(char *iface, int limit);
void ibwlist(void);
int ibwget(char *iface);
void ibwflush(void);
int ibwcfgread(FILE *fd);

#endif
