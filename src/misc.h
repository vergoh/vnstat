int kerneltest(void);
int spacecheck(char *path);
void intr(int);
int getbtime(void);
void addtraffic(uint64_t *destmb, int *destkb, uint64_t srcmb, int srckb);
char *getvalue(uint64_t mb, uint64_t kb, int len);
void showbar(uint64_t rx, int rxk, uint64_t tx, int txk, uint64_t max, int len);
