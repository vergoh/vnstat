#ifndef MISC_H
#define MISC_H

#define UNITPREFIXCOUNT 7

#include "dbsql.h"

typedef enum RequestType {
	RT_Normal = 1,
	RT_Estimate,
	RT_ImageScale
} RequestType;

typedef enum ListType {
	LT_None = 0,
	LT_5min,
	LT_Hour,
	LT_Day,
	LT_Month,
	LT_Year,
	LT_Top
} ListType;

int spacecheck(const char *path);
void sighandler(int sig);
uint64_t getbtime(void);
char *getvalue(const uint64_t bytes, const int len, const RequestType type);
int getunitspacing(const int len, const int index);
char *gettrafficrate(const uint64_t bytes, const time_t interval, const int len);
const char *getunitprefix(const int index);
const char *getrateunitprefix(const int unitmode, const int index);
uint64_t getunitdivisor(const int unitmode, const int index);
int getunit(void);
char *getratestring(const uint64_t rate, const int len, const int declen);
int getratespacing(const int len, const int unitmode, const int unitindex);
int getpadding(const int len, const char *str);
void cursortocolumn(const int column);
void cursorhide(void);
void cursorshow(void);
void eraseline(void);
int validatedatetime(const char *str);
int issametimeslot(const ListType listtype, const time_t entry, const time_t updated);
uint64_t getperiodseconds(const ListType listtype, const time_t entry, const time_t updated, const time_t created, const short isongoing);
void getestimates(uint64_t *rx, uint64_t *tx, const ListType listtype, const time_t updated, const time_t created, dbdatalist **dbdata);
int ishelprequest(const char *arg);
void indent(int i);

#endif
