void readproc(char iface[32]);
void parseproc(int newdb);
void trafficmeter(char iface[32], int sampletime);
void addtraffic(uint64_t *destmb, int *destkb, uint64_t srcmb, int srckb);
