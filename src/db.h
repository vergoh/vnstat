int readdb(char iface[32], char file[512]);
void writedb(char file[512], int newdb);
void showdb(int qmode);
void rotatedays(void);
void rotatemonths(void);
void convertdb(FILE *db);

/* version 1.0 database format */
typedef struct {
	char date[11];
	uint64_t rx, tx;
} DAY10;

typedef struct {
	char month[4];
	uint64_t rx, tx;
} MONTH10;

typedef struct {
	int version;
	char interface[32];
	uint64_t totalrx, totaltx, currx, curtx;
	int totalrxk, totaltxk;
	time_t lastupdated, created;
	DAY10 day[30];
	MONTH10 month[12];
	DAY10 top10[10];
	uint64_t btime;
} DATA10;
