
#include "ip_geolocation.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include <cjson/cJSON.h> // cJSON 헤더 추가
#include <arpa/inet.h>  // for inet_ntop and INET_ADDRSTRLEN //chagju
#include <netinet/ip.h> // for struct ip //chagju

//김태영 created file

//"창주" 주석이 있는것은 창주 lines, 주석이 없으면 태영 lines

// Geolocation 데이터를 가져와 결과를 출력하는 함수
void get_geolocation_with_output(const char *ip, const char *api_key) {
    char country[50] = {0};
    char city[50] = {0};

    printf("Fetching geolocation data for IP: %s\n", ip);

    // Geolocation 데이터 가져오기
    get_geolocation(ip, api_key, country, city);

    // 결과 출력
    if (strlen(country) > 0 && strlen(city) > 0) {
        printf("Geolocation - IP: %s, Country: %s, City: %s\n", ip, country, city);
    } else {
        printf("Geolocation data for IP: %s could not be retrieved.\n", ip);
    }
}

// 구조체로 API 응답 데이터를 저장
struct Memory {
    char *response;
    size_t size;
};

// IP 주소가 특정 범위에 속하는지 확인하는 예시 함수
void filter_ip_address(struct ip *ip_header, const char *api_key) {
    char ip_str[INET_ADDRSTRLEN];
    char country[50] = {0};
    char city[50] = {0};
    inet_ntop(AF_INET, &ip_header->ip_src, ip_str, INET_ADDRSTRLEN);

    get_geolocation(ip_str, api_key, country, city);

    // 예시: IP가 특정 범위에 있을 경우, 지리 정보 확인
    printf("Filtered IP: %s, Country: %s, City: %s\n",
           ip_str,
           strlen(country) > 0 ? country : "Unknown",
           strlen(city) > 0 ? city : "Unknown");
}//창주 37~51

// 콜백 함수: libcurl이 데이터를 수신할 때 호출
static size_t write_callback(void *contents, size_t size, size_t nmemb, void *userp) {
    size_t total_size = size * nmemb;
    struct Memory *mem = (struct Memory *)userp;

    char *ptr = realloc(mem->response, mem->size + total_size + 1);
    if (ptr == NULL) {
        fprintf(stderr, "Not enough memory for API response\n");
        return 0;
    }

    mem->response = ptr;
    memcpy(&(mem->response[mem->size]), contents, total_size);
    mem->size += total_size;
    mem->response[mem->size] = 0;

    return total_size;
}

// Geolocation API를 호출하여 IP의 위치 정보 추출
void get_geolocation(const char* ip, const char* api_key, char *country, char *city) {
    if (!ip || !api_key || !country || !city) {
        fprintf(stderr, "Invalid parameters passed to get_geolocation.\n");
        return;
    }

    CURL *curl;
    CURLcode res;
    char url[256];
    struct Memory chunk = {NULL, 0};

    // 요청 URL 생성
    snprintf(url, sizeof(url), "http://api.ipstack.com/%s?access_key=%s", ip, api_key);

    curl = curl_easy_init();
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);

        // API 요청
        res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
        } else if (chunk.response) {
            // JSON 파싱 시작
            cJSON *json = cJSON_Parse(chunk.response);
            if (json) {
                // country_name 추출
                cJSON *country_json = cJSON_GetObjectItemCaseSensitive(json, "country_name");
                if (cJSON_IsString(country_json) && (country_json->valuestring != NULL)) {
                    strncpy(country, country_json->valuestring, 49);
                } else {
                    strcpy(country, "Unknown"); // 기본값 설정
                }

                // city 추출
                cJSON *city_json = cJSON_GetObjectItemCaseSensitive(json, "city");
                if (cJSON_IsString(city_json) && (city_json->valuestring != NULL)) {
                    strncpy(city, city_json->valuestring, 49);
                } else {
                    strcpy(city, "Unknown"); // 기본값 설정
                }

                // JSON 객체 메모리 해제
                cJSON_Delete(json);
            } else {
                fprintf(stderr, "Failed to parse JSON response: %s\n", chunk.response);
                strcpy(country, "Unknown");
                strcpy(city, "Unknown");
            }
        } else {
            fprintf(stderr, "No response from API or invalid response format.\n");
        }

        // 메모리 해제
        free(chunk.response);
        curl_easy_cleanup(curl);
    } else {
        fprintf(stderr, "Failed to initialize CURL.\n");
    }
}
