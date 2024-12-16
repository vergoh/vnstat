#include "ip_geolocation.h"
#include "packet_capture.h"
#include "common.h"
#include "ifinfo.h"
#include "dbsql.h"
#include "misc.h"
#include "cfg.h"
#include "ibw.h"
#include "vnstat_func.h"
#include <pcap.h>
#include <string.h>
#include <netinet/ip.h> // for struct ip
#include <arpa/inet.h> // for inet_ntop and INET_ADDRSTRLEN


// 트래픽 데이터를 수집하는 함수
void get_traffic_data(const char *ip, const char *dev, unsigned long *bytes_sent, unsigned long *bytes_received) {
    // pcap 핸들
    pcap_t *handle;
    char errbuf[PCAP_ERRBUF_SIZE];

    // 네트워크 인터페이스 열기
    handle = pcap_open_live(dev, BUFSIZ, 1, 1000, errbuf);
    if (handle == NULL) {
        fprintf(stderr, "Couldn't open device %s: %s\n", dev, errbuf);
        return;
    }

    struct pcap_pkthdr header;
    const u_char *packet;

    // 패킷 캡처 루프
    while ((packet = pcap_next(handle, &header)) != NULL) {
        // 패킷에서 IP 헤더를 파싱하고 대상 IP 확인
        struct ip *ip_header = (struct ip *)(packet + 14);  // Ethernet 헤더 길이는 14바이트
        char src_ip[INET_ADDRSTRLEN];
        char dst_ip[INET_ADDRSTRLEN];

        inet_ntop(AF_INET, &ip_header->ip_src, src_ip, INET_ADDRSTRLEN);
        inet_ntop(AF_INET, &ip_header->ip_dst, dst_ip, INET_ADDRSTRLEN);

        // 송신/수신 바이트 계산
        if (strcmp(src_ip, ip) == 0) {
            *bytes_sent += header.len;  // 송신 트래픽
        }
        if (strcmp(dst_ip, ip) == 0) {
            *bytes_received += header.len;  // 수신 트래픽
        }
    }

    pcap_close(handle);
}//창주


void handle_address() {
    const char *api_key = "64fbc6778aa7dd9fd832966450af25df";  // ipstack API 키
    const char *dev = "en0";  // 활성 네트워크 인터페이스
    char country[50] = {0};
    char city[50] = {0};

    int ip_count = 0;
    const char **captured_ips = start_packet_capture(dev, &ip_count);

    if (captured_ips) {
        for (int i = 0; i < ip_count; i++) {
            if (captured_ips[i]) {
                // 비정상적인 IP 검증
                if (strcmp(captured_ips[i], "0.0.0.0") == 0) {
                    continue;  // Invalid IP는 무시
                }

                // IP 기반 지리적 정보 조회
                get_geolocation(captured_ips[i], api_key, country, city);

                // 간결한 결과 출력
                printf("IP: %s, Country: %s, City: %s\n",
                       captured_ips[i],
                       strlen(country) > 0 ? country : "Unknown",
                       strlen(city) > 0 ? city : "Unknown");
            }
        }

        // 메모리 해제
        for (int i = 0; i < ip_count; i++) {
            free((void *)captured_ips[i]);  // 개별 IP 메모리 해제
        }
        free(captured_ips);  // IP 배열 메모리 해제
    } else {
        printf("No IPs captured.\n");
    }
}

void handle_filter(const char *filter_ip) {
    const char *api_key = "64fbc6778aa7dd9fd832966450af25df";  // ipstack API 키
    const char *dev = "en0";  // 활성 네트워크 인터페이스
    unsigned long bytes_sent = 0;     // 특정 IP로 전송된 바이트
    unsigned long bytes_received = 0; // 특정 IP로 수신된 바이트
    char country[50] = {0};  // 국가 정보를 저장할 버퍼
    char city[50] = {0};     // 도시 정보를 저장할 버퍼
    int matched = 0;         // 필터 조건에 맞는지 여부

    int ip_count = 0;
    const char **captured_ips = start_packet_capture(dev, &ip_count);

    if (captured_ips) {
        for (int i = 0; i < ip_count; i++) {
            if (captured_ips[i] && strcmp(captured_ips[i], filter_ip) == 0) {
                // 필터 조건에 맞는 경우
                matched = 1;

                // 트래픽 데이터 수집
                get_traffic_data(filter_ip, dev, &bytes_sent, &bytes_received); //창주

                // 지리적 정보 조회
                get_geolocation(filter_ip, api_key, country, city);

                // 결과 출력
                printf("Filtered IP: %s\n", captured_ips[i]);
                printf("Country: %s, City: %s\n",
                       strlen(country) > 0 ? country : "Unknown",
                       strlen(city) > 0 ? city : "Unknown");
                printf("Traffic - Sent: %lu bytes, Received: %lu bytes\n",
                       bytes_sent, bytes_received);
                break;  // 필터 조건에 맞는 IP를 찾으면 루프 종료
            }
        }

        if (!matched) {
            // 필터 조건에 맞는 IP가 없는 경우
            printf("No traffic found for IP: %s\n", filter_ip);
        }

        // 메모리 해제
        for (int i = 0; i < ip_count; i++) {
            free((void *)captured_ips[i]);  // 개별 IP 메모리 해제
        }
        free(captured_ips);  // IP 배열 메모리 해제
    } else {
        printf("No IPs captured.\n");
    }
}
//창주


int main(int argc, char *argv[])
{
    int currentarg;
    DIR *dir = NULL;
    PARAMS p;

    initparams(&p);

    /* 명령어 파싱 */
    if (argc > 1) {
        for (currentarg = 1; currentarg < argc; currentarg++) {
            if (strcmp(argv[currentarg], "-D") == 0 || strcmp(argv[currentarg], "--debug") == 0) {
                debug = 1;
                printf("Debug enabled, vnstat %s, SQLite %s\n", VERSION, sqlite3_libversion());
            } else if (strcmp(argv[currentarg], "--config") == 0) {
                if (currentarg + 1 < argc) {
                    strncpy_nt(p.cfgfile, argv[currentarg + 1], 512);
                    if (debug)
                        printf("Loading config file: %s\n", p.cfgfile);
                    if (!loadcfg(p.cfgfile, CT_CLI)) {
                        return 1;
                    }
                    if (!ibwloadcfg(p.cfgfile)) {
                        return 1;
                    }
                    currentarg++;
                } else {
                    printf("Error: File for --config missing.\n");
                    return 1;
                }
            } else if (strcmp(argv[currentarg], "-address") == 0) {
                handle_address();  // 네트워크에 연결된 IP를 검색
                return 0;
            } else if (strcmp(argv[currentarg], "-filter") == 0) {
                if (currentarg + 1 < argc) {
                    handle_filter(argv[currentarg + 1]);  // 특정 IP를 필터링 창주
                    return 0;
                } else {
                    printf("Error: IP address missing for -filter.\n");
                    return 1;
                }
            } else if (strcmp(argv[currentarg], "-monitor") == 0) { //ip 차단 창주
                if (currentarg + 1 < argc) {
                    handle_packet_capture(argv[currentarg + 1]); // 네트워크 인터페이스 지정
                    return 0;
                } else {
                    printf("Error: Interface name missing for -monitor.\n");
                    return 1;
                }
            }
        }
    }


    /* 기본 설정 및 기존 동작 */
    if (p.cfgfile[0] == '\0') {
        if (!loadcfg(p.cfgfile, CT_CLI)) {
            return 1;
        }
        if (!ibwloadcfg(p.cfgfile)) {
            return 1;
        }
    }

    configlocale();
    strncpy_nt(p.interface, "default", MAXIFPARAMLEN);
    strncpy_nt(p.definterface, cfg.iface, MAXIFPARAMLEN);
    strncpy_nt(p.alias, "none", 32);

    parseargs(&p, argc, argv);

    /* 데이터베이스 열기 */
    if (!p.traffic && !p.livetraffic) {
        if ((dir = opendir(cfg.dbdir)) != NULL) {
            if (debug)
                printf("Dir OK\n");
            closedir(dir);
            if (!db_open_ro()) {
                printf("Error: Failed to open database \"%s/%s\" in read-only mode.\n", cfg.dbdir, DATABASEFILE);
                if (errno == ENOENT) {
                    printf("The vnStat daemon should have created the database when started.\n");
                    printf("Check that it is configured and running. See also \"man vnstatd\".\n");
                }
                return 1;
            }
            p.dbifcount = db_getinterfacecount();
            if (debug)
                printf("%" PRIu64 " interface(s) found\n", p.dbifcount);

            if (p.dbifcount > 1) {
                strncpy_nt(p.definterface, cfg.iface, MAXIFPARAMLEN);
            }
        } else {
            printf("Error: Unable to open database directory \"%s\": %s\n", cfg.dbdir, strerror(errno));
            if (errno == ENOENT) {
                printf("The vnStat daemon should have created this directory when started.\n");
                printf("Check that it is configured and running. See also \"man vnstatd\".\n");
            } else {
                printf("Make sure it is at least read enabled for current user.\n");
                printf("Use --help for help.\n");
            }
            return 1;
        }
    }

    /* 인터페이스 처리 및 기본 동작 */
    handleifselection(&p);
    handleshowalert(&p);
    handleremoveinterface(&p);
    handlerenameinterface(&p);
    handleaddinterface(&p);
    handlesetalias(&p);
    handleshowdata(&p);
    handletrafficmeters(&p);

    /* 추가 정보가 없을 경우 출력 */
    if (!p.query && !p.traffic && !p.livetraffic) {
        if (p.dbifcount == 0) {
            getifliststring(&p.ifacelist, 1);
            printf("No interfaces found in the database, nothing to do. Use --help for help.\n\n");
            printf("Interfaces can be added to the database with the following command:\n");
            printf("    %s --add -i eth0\n\n", argv[0]);
            printf("Replace 'eth0' with the interface that should be monitored.\n\n");
            if (strlen(cfg.cfgfile)) {
                printf("The default interface can be changed by updating the \"Interface\" keyword\n");
                printf("value in the configuration file \"%s\".\n\n", cfg.cfgfile);
            }
            printf("The following interfaces are currently available:\n    %s\n", p.ifacelist);
            free(p.ifacelist);
        } else {
            printf("Nothing to do. Use --help for help.\n");
        }
    }

    /* 정리 및 종료 */
    db_close();
    ibwflush();

    return 0;
}
