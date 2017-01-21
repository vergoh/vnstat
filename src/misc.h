#ifndef MISC_H
#define MISC_H

#define UNITPREFIXCOUNT 5

int kerneltest(void);
int spacecheck(const char *path);
void sighandler(int);
uint64_t getbtime(void);
char *getvalue(const uint64_t b, const int len, const int type);
char *gettrafficrate(const uint64_t bytes, const uint32_t interval, const int len);
uint64_t getscale(const uint64_t input);
char *getunitprefix(const int index);
char *getrateunitprefix(const int unitmode, const int index);
uint64_t getunitdivisor(const int unitmode, const int index);
char *getratestring(const uint64_t rate, const int len, const int declen, const int unitmode);
int getpadding(const int len, const char *str);
void cursortocolumn(const int column);
void cursorhide(void);
void cursorshow(void);
void eraseline(void);

#endif
