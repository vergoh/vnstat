#ifndef MISC_H
#define MISC_H

#define UNITCOUNT 4

int kerneltest(void);
int spacecheck(char *path);
void sighandler(int);
int getbtime(void);
void addtraffic(uint64_t *destmb, int *destkb, uint64_t srcmb, int srckb);
char *getvalue(uint64_t mb, uint64_t kb, int len, int type);
uint64_t getscale(uint64_t kb);
char *getunit(int index);
char *getbunit(int unit, int index);

#endif
