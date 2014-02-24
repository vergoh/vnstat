void getifinfo(char iface[32]);
int readproc(char iface[32]);
int readsysclassnet(char iface[32]);
void parseifinfo(int newdb);
void trafficmeter(char iface[32], int sampletime);
void livetrafficmeter(char iface[32]);
uint64_t countercalc(uint64_t a, uint64_t b);
void showspeed(float xfer, int len);
