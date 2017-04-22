#ifndef MISC_H
#define MISC_H

#define UNITPREFIXCOUNT 4

int kerneltest(void);
int spacecheck(char *path);
void sighandler(int sig);
uint64_t getbtime(void);
char *getvalue(uint64_t mb, uint64_t kb, int len, int type);
char *getrate(uint64_t mb, uint64_t kb, uint32_t interval, int len);
char *gettrafficrate(uint64_t bytes, uint32_t interval, int len);
uint64_t getscale(uint64_t kb);
char *getunitprefix(int index);
char *getrateunitprefix(int unitmode, int index);
uint64_t getunitdivisor(int unitmode, int index);
char *getratestring(uint64_t rate, int len, int declen, int unitmode);
int getpadding(int len, char *str);
void cursortocolumn(int column);
void cursorhide(void);
void cursorshow(void);
void eraseline(void);

#endif
