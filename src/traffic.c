#include "common.h"
#include "ifinfo.h"
#include "misc.h"
#include "traffic.h"
#include <arpa/inet.h> //cj
#include <netinet/in.h> //cj
#include <time.h>  //cj
#include <netinet/ip.h> //cj
#include <pcap.h> //cj

#define MAX_IP_GROUP 256
#define DEFAULT_TRAFFIC_THRESHOLD 1024 * 1024 // 1MB
#define DEFAULT_WARNING_LIMIT 3 // 경고 횟수 제한 창주(3)

#include <ifaddrs.h>
#include <string.h>
#include <stdio.h>

void save_blocked_ip(const char *ip) {
    FILE *file = fopen("blocked_ips.txt", "r+"); // 읽기/쓰기 모드로 열기
    char line[INET_ADDRSTRLEN];
    int already_exists = 0;

    if (file) {
        // 파일에서 기존 IP 확인
        while (fgets(line, sizeof(line), file)) {
            line[strcspn(line, "\n")] = '\0'; // 개행 문자 제거
            if (strcmp(line, ip) == 0) {
                already_exists = 1;
                break;
            }
        }
        fclose(file);
    }

    if (!already_exists) {
        file = fopen("blocked_ips.txt", "a");
        if (!file) {
            perror("Error opening file to save blocked IP");
            return;
        }

        fprintf(file, "%s\n", ip);
        fclose(file);
    }
} 

// 네트워크 인터페이스에서 본인의 IP를 감지하는 함수
char *get_self_ip(const char *interface) {
    struct ifaddrs *ifaddr, *ifa;
    char *ip_address = NULL;
    char buffer[INET_ADDRSTRLEN];

    // 네트워크 인터페이스 정보를 가져옴
    if (getifaddrs(&ifaddr) == -1) {
        perror("getifaddrs");
        return NULL;
    }

    // 인터페이스 리스트 순회
    for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr == NULL || ifa->ifa_addr->sa_family != AF_INET) {
            continue; // IPv4 주소만 처리
        }

        // 지정된 인터페이스인지 확인
        if (strcmp(ifa->ifa_name, interface) == 0) {
            struct sockaddr_in *addr = (struct sockaddr_in *)ifa->ifa_addr;
            if (inet_ntop(AF_INET, &addr->sin_addr, buffer, INET_ADDRSTRLEN) != NULL) {
                ip_address = strdup(buffer); // IP 복사
                break;
            }
        }
    }

    freeifaddrs(ifaddr);
    return ip_address;
} 

unsigned long TRAFFIC_THRESHOLD = DEFAULT_TRAFFIC_THRESHOLD;
unsigned int WARNING_LIMIT = DEFAULT_WARNING_LIMIT;

void set_traffic_settings(unsigned long threshold, unsigned int warnings) {
    TRAFFIC_THRESHOLD = threshold;
    WARNING_LIMIT = warnings;
}

typedef struct {
    char ip[INET_ADDRSTRLEN]; // IP 주소
    unsigned long bytes_sent; // 송신된 바이트
    unsigned long bytes_received; // 수신된 바이트
    time_t last_detected; // 마지막 트래픽 감지 시간
    int warning_count; // 경고 횟수
    int is_blocked; // 차단 여부 (0: 정상, 1: 차단됨)
} TrafficInfo;

TrafficInfo monitored_ips[MAX_IP_GROUP];
int monitored_count = 0; //창주(10)

void monitor_ip_traffic(const char *ip, unsigned long sent, unsigned long received) {
    static char *self_ip = NULL;

    if (!self_ip) {
        self_ip = get_self_ip("eth0"); // 본인의 네트워크 인터페이스 이름
        if (!self_ip) {
            fprintf(stderr, "Error: Unable to detect self IP.\n");
            return;
        }
        printf("Self IP detected: %s\n", self_ip);
    }
    
    if (strcmp(ip, self_ip) == 0) {
        return; // 본인의 IP는 처리하지 않고 종료
    }


    int found = 0;

    for (int i = 0; i < monitored_count; i++) {
        if (strcmp(monitored_ips[i].ip, ip) == 0) {
            found = 1;
            monitored_ips[i].bytes_sent += sent;
            monitored_ips[i].bytes_received += received;

            if (monitored_ips[i].is_blocked) {
                printf("Blocked IP tried to access: %s\n", ip);
                return;
            }

            if (monitored_ips[i].bytes_received > TRAFFIC_THRESHOLD || monitored_ips[i].bytes_sent > TRAFFIC_THRESHOLD) {
                monitored_ips[i].warning_count++;
                if (monitored_ips[i].warning_count >= WARNING_LIMIT) {
                    monitored_ips[i].is_blocked = 1;
                    printf("IP blocked due to repeated high traffic: %s\n", ip);
                    save_blocked_ip(ip);
                } else {
                    printf("High traffic from IP: %s. Warning issued.\n", ip);
                }
            }
            return;
        }
    }

    if (!found && monitored_count < MAX_IP_GROUP) {
        strncpy(monitored_ips[monitored_count].ip, ip, INET_ADDRSTRLEN);
        monitored_ips[monitored_count].bytes_sent = sent;
        monitored_ips[monitored_count].bytes_received = received;
        monitored_ips[monitored_count].last_detected = time(NULL);
        monitored_ips[monitored_count].warning_count = 0;
        monitored_ips[monitored_count].is_blocked = 0;
        monitored_count++;
        printf("New IP added to monitoring: %s\n", ip);
    }
}

void list_monitored_ips() {
    printf("Monitored IPs:\n");
    for (int i = 0; i < monitored_count; i++) {
        printf("IP: %s, Sent: %lu bytes, Received: %lu bytes, Warnings: %d, Blocked: %d\n",
               monitored_ips[i].ip,
               monitored_ips[i].bytes_sent,
               monitored_ips[i].bytes_received,
               monitored_ips[i].warning_count,
               monitored_ips[i].is_blocked);
    }
}


void block_ip(const char *ip) {
    char cmd[256];
   #ifdef __APPLE__
       snprintf(cmd, sizeof(cmd), "sudo pfctl -t blocklist -T add %s", ip);
   #else
       snprintf(cmd, sizeof(cmd), "sudo iptables -A INPUT -s %s -j DROP", ip);
   #endif
      system(cmd);
} ////창주 19~117

void trafficmeter(const char *iface, unsigned int sampletime)
{
   /* received bytes packets errs drop fifo frame compressed multicast */
   /* transmitted bytes packets errs drop fifo colls carrier compressed */
   uint64_t rx, tx, rxp, txp;
   int json = 0;
   IFINFO firstinfo;
   char buffer[256];

   if (cfg.qmode == 10) {
      json = 1;
   }

#ifndef CHECK_VNSTAT
   /* less than 2 seconds doesn't produce good results */
   if (sampletime < 2) {
      printf("Error: Time for sampling too short.\n");
      exit(EXIT_FAILURE);
   }
#endif

   /* read interface info and get values to the first list */
   if (!getifinfo(iface)) {
      printf("Error: Interface \"%s\" not available, exiting.\n", iface);
      exit(EXIT_FAILURE);
   }
   firstinfo.rx = ifinfo.rx;
   firstinfo.tx = ifinfo.tx;
   firstinfo.rxp = ifinfo.rxp;
   firstinfo.txp = ifinfo.txp;

   /* wait sampletime and print some nice dots so that the user thinks
   something is done :) */
   if (json || cfg.ostyle == 4) {
      sleep(sampletime);
   } else {
      snprintf(buffer, 256, "Sampling %s (%u seconds average)", iface, sampletime);
      printf("%s", buffer);
      fflush(stdout);
      sleep(sampletime / 3);
      printf(".");
      fflush(stdout);
      sleep(sampletime / 3);
      printf(".");
      fflush(stdout);
      sleep(sampletime / 3);
      printf(".");
      fflush(stdout);
      if ((sampletime / 3) * 3 != sampletime) {
         sleep(sampletime - ((sampletime / 3) * 3));
      }

      cursortocolumn(1);
      eraseline();
   }

#ifdef CHECK_VNSTAT
   sampletime = 1;
#endif

   /* read those values again... */
   if (!getifinfo(iface)) {
      printf("Error: Interface \"%s\" not available, exiting.\n", iface);
      exit(EXIT_FAILURE);
   }

   /* calculate traffic and packets seen between updates */
   rx = countercalc(&firstinfo.rx, &ifinfo.rx, ifinfo.is64bit);
   tx = countercalc(&firstinfo.tx, &ifinfo.tx, ifinfo.is64bit);
   rxp = countercalc(&firstinfo.rxp, &ifinfo.rxp, ifinfo.is64bit);
   txp = countercalc(&firstinfo.txp, &ifinfo.txp, ifinfo.is64bit);

   /* show the difference in a readable format or json */
   if (!json) {
      if (cfg.ostyle != 4) {
         printf("%" PRIu64 " packets sampled in %d seconds\n", rxp + txp, sampletime);
      }
      printf("Traffic average for %s\n\n", iface);

      indent(4);
      printf("  rx");
      indent(5);
      printf("%s   ", gettrafficrate(rx, sampletime, 15));
      indent(6);
      printf("%5" PRIu64 " packets/s\n", (uint64_t)(rxp / sampletime));

      indent(4);
      printf("  tx");
      indent(5);
      printf("%s   ", gettrafficrate(tx, sampletime, 15));
      indent(6);
      printf("%5" PRIu64 " packets/s\n\n", (uint64_t)(txp / sampletime));
   } else {
      printf("{\"jsonversion\":\"%d\",", JSONVERSION_TR);
      printf("\"vnstatversion\":\"%s\",", getversion());
      printf("\"interface\":\"%s\",", iface);
      printf("\"sampletime\":%u,", sampletime);
      printf("\"rx\":{");
      printf("\"ratestring\":\"%s\",", gettrafficrate(rx, sampletime, 0));
      printf("\"bytespersecond\":%" PRIu64 ",", (uint64_t)(rx / sampletime));
      printf("\"packetspersecond\":%" PRIu64 ",", (uint64_t)(rxp / sampletime));
      printf("\"bytes\":%" PRIu64 ",", rx);
      printf("\"packets\":%" PRIu64 "", rxp);
      printf("},");
      printf("\"tx\":{");
      printf("\"ratestring\":\"%s\",", gettrafficrate(tx, sampletime, 0));
      printf("\"bytespersecond\":%" PRIu64 ",", (uint64_t)(tx / sampletime));
      printf("\"packetspersecond\":%" PRIu64 ",", (uint64_t)(txp / sampletime));
      printf("\"bytes\":%" PRIu64 ",", tx);
      printf("\"packets\":%" PRIu64 "", txp);
      printf("}}\n");
   }
}

void livetrafficmeter(const char *iface, const int mode)
{
   /* received bytes packets errs drop fifo frame compressed multicast */
   /* transmitted bytes packets errs drop fifo colls carrier compressed */
   uint64_t rx, tx, rxp, txp, timespent, timeslept;
   uint64_t rxtotal, txtotal, rxptotal, txptotal;
   uint64_t rxpmin, txpmin, rxpmax, txpmax;
   uint64_t rxmin, txmin, rxmax, txmax;
   uint64_t index = 1;
   int ratewidth, ppswidth, paddingwidth, json = 0, spin_i = 0;
   char buffer[256], buffer2[256], spin_c = ' ';
   const char *spinner = "|/-\\";
   IFINFO previnfo;

   if (cfg.qmode == 10) {
      json = 1;
   }

   if (!json) {
      printf("Monitoring %s...    (press CTRL-C to stop)\n\n", iface);
      if (cfg.ostyle != 4) {
         printf("   getting traffic...");
         fflush(stdout);
      }
   }

   /* enable signal trap */
   intsignal = 0;
   if (signal(SIGINT, sighandler) == SIG_ERR) {
      perror("signal");
      exit(EXIT_FAILURE);
   }

   /* set some defaults */
   rxtotal = txtotal = rxptotal = txptotal = rxpmax = txpmax = 0;
   rxpmin = txpmin = rxmin = txmin = MAX64;
   rxmax = txmax = 0;
   timeslept = 0;

   timespent = (uint64_t)time(NULL);

   /* read /proc/net/dev and get values to the first list */
   if (!getifinfo(iface)) {
      printf("Error: Interface \"%s\" not available, exiting.\n", iface);
      exit(EXIT_FAILURE);
   }

   ratewidth = 14;
   ppswidth = 5;
   paddingwidth = 6;

   /* narrow output mode */
   if (cfg.ostyle == 0) {
      ratewidth = 12;
      ppswidth = 3;
      paddingwidth = 4;
   }

   if (!json) {
      cursorhide();
   } else {
      printf("{\"jsonversion\":\"%d\",", JSONVERSION_LIVE);
      printf("\"vnstatversion\":\"%s\",", getversion());
      printf("\"interface\":\"%s\",", iface);
      printf("\"sampletime\":%d}\n", LIVETIME);
   }

   /* loop until user gets bored */
   while (intsignal == 0) {

      timeslept = (uint64_t)time(NULL);

#ifndef CHECK_VNSTAT
      /* wait 2 seconds for more traffic */
      sleep(LIVETIME);
#endif

      timeslept = (uint64_t)time(NULL) - timeslept;

      /* break loop without calculations because sleep was probably interrupted */
      if (intsignal) {
         break;
      }

      /* use values from previous loop if this isn't the first time */
      previnfo.rx = ifinfo.rx;
      previnfo.tx = ifinfo.tx;
      previnfo.rxp = ifinfo.rxp;
      previnfo.txp = ifinfo.txp;

      /* read those values again... */
      if (!getifinfo(iface)) {
         cursorshow();
         printf("Error: Interface \"%s\" not available, exiting.\n", iface);
         exit(EXIT_FAILURE);
      }

      /* calculate traffic and packets seen between updates */
      rx = countercalc(&previnfo.rx, &ifinfo.rx, ifinfo.is64bit);
      tx = countercalc(&previnfo.tx, &ifinfo.tx, ifinfo.is64bit);
      rxp = countercalc(&previnfo.rxp, &ifinfo.rxp, ifinfo.is64bit);
      txp = countercalc(&previnfo.txp, &ifinfo.txp, ifinfo.is64bit);

      /* update totals */
      rxtotal += rx;
      txtotal += tx;
      rxptotal += rxp;
      txptotal += txp;

      /* update min & max */
      if (rxmin > rx) {
         rxmin = rx;
      }
      if (txmin > tx) {
         txmin = tx;
      }
      if (rxmax < rx) {
         rxmax = rx;
      }
      if (txmax < tx) {
         txmax = tx;
      }
      if (rxpmin > rxp) {
         rxpmin = rxp;
      }
      if (txpmin > txp) {
         txpmin = txp;
      }
      if (rxpmax < rxp) {
         rxpmax = rxp;
      }
      if (txpmax < txp) {
         txpmax = txp;
      }

      /* show the difference in a readable format or json */
      if (!json) {
         if (cfg.livespinner && cfg.ostyle != 4) {
            spin_i++;
            if (spin_i >= 4) {
               spin_i = 0;
            }
            spin_c = spinner[spin_i];
         }

         if (mode == 0) {
            /* packets per second visible */
            snprintf(buffer, 128, "%c%*srx: %s %*" PRIu64 " p/s", spin_c, paddingwidth, " ", gettrafficrate(rx, LIVETIME, ratewidth), ppswidth, rxp / LIVETIME);
            snprintf(buffer2, 128, "%*s tx: %s %*" PRIu64 " p/s", paddingwidth, " ", gettrafficrate(tx, LIVETIME, ratewidth), ppswidth, txp / LIVETIME);
         } else {
            /* total transfer amount visible */
            snprintf(buffer, 128, "%c%*srx: %s   %s", spin_c, paddingwidth, " ", gettrafficrate(rx, LIVETIME, ratewidth), getvalue(rxtotal, 1, RT_Normal));
            snprintf(buffer2, 128, "%*stx: %s   %s", paddingwidth, " ", gettrafficrate(tx, LIVETIME, ratewidth), getvalue(txtotal, 1, RT_Normal));
         }
         strcat(buffer, buffer2);

         if (cfg.ostyle != 4 || !debug) {
            cursortocolumn(1);
            eraseline();
         }
         if (cfg.ostyle != 4) {
            printf("%s", buffer);
         } else {
            printf("%s\n", buffer);
         }
      } else {
         printf("{\"index\":%" PRIu64 ",", index);
         printf("\"seconds\":%" PRIu64 ",", (uint64_t)time(NULL) - timespent);
         printf("\"rx\":{");
         printf("\"ratestring\":\"%s\",", gettrafficrate(rx, LIVETIME, 0));
         printf("\"bytespersecond\":%" PRIu64 ",", (uint64_t)(rx / LIVETIME));
         printf("\"packetspersecond\":%" PRIu64 ",", (uint64_t)(rxp / LIVETIME));
         printf("\"bytes\":%" PRIu64 ",", rx);
         printf("\"packets\":%" PRIu64 ",", rxp);
         printf("\"totalbytes\":%" PRIu64 ",", rxtotal);
         printf("\"totalpackets\":%" PRIu64 "", rxptotal);
         printf("},");
         printf("\"tx\":{");
         printf("\"ratestring\":\"%s\",", gettrafficrate(tx, LIVETIME, 0));
         printf("\"bytespersecond\":%" PRIu64 ",", (uint64_t)(tx / LIVETIME));
         printf("\"packetspersecond\":%" PRIu64 ",", (uint64_t)(txp / LIVETIME));
         printf("\"bytes\":%" PRIu64 ",", tx);
         printf("\"packets\":%" PRIu64 ",", txp);
         printf("\"totalbytes\":%" PRIu64 ",", txtotal);
         printf("\"totalpackets\":%" PRIu64 "", txptotal);
         printf("}}\n");
         index++;
      }
      fflush(stdout);
#ifdef CHECK_VNSTAT
      break;
#endif
   }

   timespent = (uint64_t)time(NULL) - timespent - timeslept;

#ifdef CHECK_VNSTAT
   timespent = 10;
#endif

   if (!json) {
      cursorshow();
      printf("\n\n");
   }

   /* print some statistics if enough time did pass */
   if (!json && timespent >= 10) {

      printf("\n %s  /  traffic statistics\n\n", iface);

      printf("                           rx         |       tx\n");
      printf("--------------------------------------+------------------\n");
      printf("  bytes              %s", getvalue(rxtotal, 15, RT_Normal));
      printf("  | %s", getvalue(txtotal, 15, RT_Normal));
      printf("\n");
      printf("--------------------------------------+------------------\n");
      printf("          max        %s", gettrafficrate(rxmax, LIVETIME, 15));
      printf("  | %s\n", gettrafficrate(txmax, LIVETIME, 15));
      printf("      average        %s", gettrafficrate(rxtotal, (time_t)timespent, 15));
      printf("  | %s\n", gettrafficrate(txtotal, (time_t)timespent, 15));
      printf("          min        %s", gettrafficrate(rxmin, LIVETIME, 15));
      printf("  | %s\n", gettrafficrate(txmin, LIVETIME, 15));
      printf("--------------------------------------+------------------\n");
      printf("  packets               %12" PRIu64 "  |    %12" PRIu64 "\n", rxptotal, txptotal);
      printf("--------------------------------------+------------------\n");
      printf("          max          %9" PRIu64 " p/s  |   %9" PRIu64 " p/s\n", rxpmax / LIVETIME, txpmax / LIVETIME);
      printf("      average          %9" PRIu64 " p/s  |   %9" PRIu64 " p/s\n", rxptotal / timespent, txptotal / timespent);
      printf("          min          %9" PRIu64 " p/s  |   %9" PRIu64 " p/s\n", rxpmin / LIVETIME, txpmin / LIVETIME);
      printf("--------------------------------------+------------------\n");

      if (timespent <= 60) {
         printf("  time             %9" PRIu64 " seconds\n", timespent);
      } else {
         printf("  time               %7.2f minutes\n", (double)timespent / (double)60);
      }

      printf("\n");
   } else if (json) {
      printf("{\"seconds\":%" PRIu64 ",", timespent);
      printf("\"rx\":{");
      printf("\"maxratestring\":\"%s\",", gettrafficrate(rxmax, LIVETIME, 0));
      printf("\"averageratestring\":\"%s\",", gettrafficrate(rxtotal, (time_t)timespent, 0));
      printf("\"minratestring\":\"%s\",", gettrafficrate(rxmin, LIVETIME, 0));
      printf("\"totalbytes\":%" PRIu64 ",", rxtotal);
      printf("\"maxbytes\":%" PRIu64 ",", rxmax);
      printf("\"minbytes\":%" PRIu64 ",", rxmin);
      printf("\"totalpackets\":%" PRIu64 ",", rxptotal);
      printf("\"maxpackets\":%" PRIu64 ",", rxpmax);
      printf("\"minpackets\":%" PRIu64 "", rxpmin);
      printf("},");
      printf("\"tx\":{");
      printf("\"maxratestring\":\"%s\",", gettrafficrate(txmax, LIVETIME, 0));
      printf("\"averageratestring\":\"%s\",", gettrafficrate(txtotal, (time_t)timespent, 0));
      printf("\"minratestring\":\"%s\",", gettrafficrate(txmin, LIVETIME, 0));
      printf("\"totalbytes\":%" PRIu64 ",", txtotal);
      printf("\"maxbytes\":%" PRIu64 ",", txmax);
      printf("\"minbytes\":%" PRIu64 ",", txmin);
      printf("\"totalpackets\":%" PRIu64 ",", txptotal);
      printf("\"maxpackets\":%" PRIu64 ",", txpmax);
      printf("\"minpackets\":%" PRIu64 "", txpmin);
      printf("}}\n");
   }
}
