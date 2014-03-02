#ifndef MISC_H
#define MISC_H

#define UNITCOUNT 4

int kerneltest(void);
int spacecheck(char *path);
void sighandler(int);
int getbtime(void);
char *getvalue(uint64_t mb, uint64_t kb, int len, int type);
char *getrate(uint64_t mb, uint64_t kb, uint32_t interval, int len);
char *gettrafficrate(uint64_t bytes, uint32_t interval, int len);
uint64_t getscale(uint64_t kb);
char *getunit(int index);
char *getrateunit(int unit, int index);
uint32_t getunitdivider(int unit, int index);
char *getratestring(float rate, int len, int declen, int unit);

#endif
