#ifndef DBMERGE_H
#define DBMERGE_H

int mergedb(char iface[32], char dirname[512]);
void emptydb(DATA *dat);
int mergewith(DATA *dat);
void cleanmerged(DATA *dat);

#endif
