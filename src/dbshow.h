#ifndef DBSHOW_H
#define DBSHOW_H

#define DATEBUFFLEN 64
#define HGLINES 15

typedef struct {
	time_t date;
	uint64_t rx, tx;
} HOURDATA;

typedef enum AlertType {
	AT_None = 0,
	AT_Hour,
	AT_Day,
	AT_Month,
	AT_Year
} AlertType;

typedef enum AlertCondition {
	AC_None = 0,
	AC_RX,
	AC_TX,
	AC_Total,
	AC_RX_Estimate,
	AC_TX_Estimate,
	AC_Total_Estimate
} AlertCondition;

void showdb(const char *interface, int qmode, const char *databegin, const char *dataend);
void showsummary(const interfaceinfo *interface, const int shortmode);
void showlist(const interfaceinfo *interface, const char *listname, const char *databegin, const char *dataend);
void showoneline(const interfaceinfo *interface);
void showhours(const interfaceinfo *interface);
int showbar(const uint64_t rx, const uint64_t tx, const uint64_t max, const int len);
void indent(int i);
int showalert(const char *interface, const AlertType type, const AlertCondition condition, const uint64_t limit);

#endif
