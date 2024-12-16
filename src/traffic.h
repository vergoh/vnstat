#ifndef TRAFFIC_H
#define TRAFFIC_H

void trafficmeter(const char *iface, unsigned int sampletime);
void livetrafficmeter(const char *iface, const int mode);

void monitor_ip_traffic(const char *ip, unsigned long sent, unsigned long received);//창주
void block_ip(const char *ip); //창주
void list_monitored_ips(); //창주

#endif
