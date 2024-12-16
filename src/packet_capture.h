#ifndef PACKET_CAPTURE_H
#define PACKET_CAPTURE_H

#include <pcap.h>

// 패킷 처리 콜백 함수
void packet_handler(u_char *user, const struct pcap_pkthdr *header, const u_char *packet);

void capture_packets(const char *device, const char *filter_exp); //창주

void handle_packet_capture(const char *dev); //창주

// 패킷 캡처 시작 함수: 수정된 선언
const char** start_packet_capture(const char *dev, int *out_count);

#endif // PACKET_CAPTURE_H
