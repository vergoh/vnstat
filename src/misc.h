#ifndef MISC_H
#define MISC_H

#define UNITPREFIXCOUNT 5

int spacecheck(const char *path);
void sighandler(int);
uint64_t getbtime(void);
char *getvalue(const uint64_t b, const int len, const int type);
int getunitspacing(const int len, const int index);
char *gettrafficrate(const uint64_t bytes, const uint32_t interval, const int len);
char *getunitprefix(const int index);
char *getrateunitprefix(const int unitmode, const int index);
uint64_t getunitdivisor(const int unitmode, const int index);
char *getratestring(const uint64_t rate, const int len, const int declen, const int unitmode);
int getratespacing(const int len, const int unitmode, const int unitindex);
int getpadding(const int len, const char *str);
void cursortocolumn(const int column);
void cursorhide(void);
void cursorshow(void);
void eraseline(void);

#endif
