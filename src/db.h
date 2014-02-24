int readdb(char iface[32], char file[512]);
void writedb(char file[512], int newdb);
void showdb(int qmode);
void showhours(void);
void cleanhours(void);
void rotatedays(void);
void rotatemonths(void);
void convertdb(FILE *db);
void showint(uint64_t mb, int kb, int len);

/* version 1.0 database format aka db v1 */
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


/* version 1.1-1.2 database format aka db v2 */
typedef struct {
	time_t date;
	uint64_t rx, tx;
	int used;
} DAY12;

typedef struct {
	time_t month;
	uint64_t rx, tx;
	int used;
} MONTH12;

typedef struct {
	int version;
	char interface[32];
	char nick[32];
	int active;
	uint64_t totalrx, totaltx, currx, curtx;
	int totalrxk, totaltxk;
	time_t lastupdated, created;
	DAY12 day[30];
	MONTH12 month[12];
	DAY12 top10[10];
	uint64_t btime;
} DATA12;
