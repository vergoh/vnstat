void readproc(char iface[32]);
void parseproc(int newdb);
void trafficmeter(char iface[32], int sampletime);
void livetrafficmeter(char iface[32]);
void addtraffic(uint64_t *destmb, int *destkb, uint64_t srcmb, int srckb);
uint64_t countercalc(uint64_t a, uint64_t b);
void showspeed(float xfer, int len);
