#ifndef DBMERGE_H
#define DBMERGE_H

int mergedb(char iface[], char dirname[]);
void emptydb(DATA *dat);
int mergewith(DATA *dat);
void cleanmerged(DATA *dat);

#endif
