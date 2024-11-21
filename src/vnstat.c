#include "ip_geolocation.h"
#include "packet_capture.h"
#include "common.h"
#include "ifinfo.h"
#include "dbsql.h"
#include "misc.h"
#include "cfg.h"
#include "ibw.h"
#include "vnstat_func.h"

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
                // 새로운 -address 플래그 처리
                handle_address();
                return 0;
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
