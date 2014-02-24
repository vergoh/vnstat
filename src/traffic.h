#ifndef TRAFFIC_H
#define TRAFFIC_H

void trafficmeter(char iface[32], int sampletime);
void livetrafficmeter(char iface[32]);
void showspeed(float xfer, int len);

#endif
