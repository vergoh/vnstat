#include "common.h"
#include "misc.h"
#include "dbaccess.h"
#include "dbsql.h"
#include "cfg.h"
#include "ibw.h"
#include "ifinfo.h"

int getifinfo(const char *iface)
{
	char inface[32];

	ifinfo.filled = 0;
	ifinfo.timestamp = 0;

	if (strcmp(iface, "default")==0) {
		strncpy_nt(inface, cfg.iface, 32);
	} else {
		strncpy_nt(inface, iface, 32);
	}

#if defined(__linux__) || defined(CHECK_VNSTAT)
	/* try getting interface info from /proc */
	if (readproc(inface)==1) {
		ifinfo.timestamp = time(NULL);
		return 1;
	} else {
		if (debug)
			printf("Failed to use %s as source.\n", PROCNETDEV);
	}

	/* try getting interface info from /sys */
	if (readsysclassnet(inface)==1) {
		ifinfo.timestamp = time(NULL);
		return 1;
	}

#elif defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__) || defined(__APPLE__)  || defined(__FreeBSD_kernel__)
	if (readifaddrs(inface)==1) {
		ifinfo.timestamp = time(NULL);
		return 1;
	}
#endif

	snprintf(errorstring, 1024, "Unable to get interface \"%s\" statistics.", inface);
	printe(PT_Error);
	return 0;
}

int getiflist(char **ifacelist, int showspeed)
{
	uint32_t speed;
	char temp[64];
#if defined(__linux__) || defined(CHECK_VNSTAT)
	char interface[32];
	FILE *fp;
	DIR *dp;
	struct dirent *di;
	char procline[512];
#elif defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__) || defined(__APPLE__) || defined(__FreeBSD_kernel__)
	struct ifaddrs *ifap, *ifa;
#endif

	/* initialize list */
	*ifacelist = (char *)malloc(sizeof(char));
	if (*ifacelist == NULL) {
		panicexit(__FILE__, __LINE__);
	}
	*ifacelist[0] = '\0';

#if defined(__linux__) || defined(CHECK_VNSTAT)
	if ((fp=fopen(PROCNETDEV, "r"))!=NULL) {

		/* make list of interfaces */
		while (fgets(procline, 512, fp)!=NULL) {
			sscanf(procline, "%63s", temp);
			if (strlen(temp)>0 && (isdigit(temp[(strlen(temp)-1)]) || temp[(strlen(temp)-1)]==':')) {
				sscanf(temp, "%31[^':']s", interface);
				*ifacelist = (char *)realloc(*ifacelist, ( ( strlen(*ifacelist) + strlen(interface) + 2 ) * sizeof(char)) );
				if (*ifacelist == NULL) {
					panicexit(__FILE__, __LINE__);
				}
				strncat(*ifacelist, interface, strlen(interface));
				strcat(*ifacelist, " ");
				if (!showspeed) {
					continue;
				}
				speed = getifspeed(interface);
				if (speed > 0) {
					snprintf(temp, 64, "(%u Mbit) ", speed);
					*ifacelist = (char *)realloc(*ifacelist, ( ( strlen(*ifacelist) + strlen(temp) + 1 ) * sizeof(char)) );
					if (*ifacelist == NULL) {
						panicexit(__FILE__, __LINE__);
					}
					strncat(*ifacelist, temp, strlen(temp));
				}
			}
		}

		fclose(fp);
		return 1;

	} else {

		if ((dp=opendir(SYSCLASSNET))!=NULL) {

			/* make list of interfaces */
			while ((di=readdir(dp))) {
				if (di->d_name[0] == '.' || strlen(di->d_name) > 31) {
					continue;
				}
				*ifacelist = (char *)realloc(*ifacelist, ( ( strlen(*ifacelist) + strlen(di->d_name) + 2 ) * sizeof(char)) );
				if (*ifacelist == NULL) {
					panicexit(__FILE__, __LINE__);
				}
				strncat(*ifacelist, di->d_name, strlen(di->d_name));
				strcat(*ifacelist, " ");
				if (!showspeed) {
					continue;
				}
				speed = getifspeed(di->d_name);
				if (speed > 0) {
					snprintf(temp, 64, "(%u Mbit) ", speed);
					*ifacelist = (char *)realloc(*ifacelist, ( ( strlen(*ifacelist) + strlen(temp) + 1 ) * sizeof(char)) );
					if (*ifacelist == NULL) {
						panicexit(__FILE__, __LINE__);
					}
					strncat(*ifacelist, temp, strlen(temp));
				}
			}

			closedir(dp);
			return 1;
		}
	}

#elif defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__) || defined(__APPLE__) || defined(__FreeBSD_kernel__)
	if (getifaddrs(&ifap) >= 0) {

		/* make list of interfaces */
		for (ifa = ifap; ifa; ifa = ifa->ifa_next) {
			if (ifa->ifa_addr->sa_family != AF_LINK || strlen(ifa->ifa_name) > 31) {
				continue;
			}
			*ifacelist = realloc(*ifacelist, ( ( strlen(*ifacelist) + strlen(ifa->ifa_name) + 2 ) * sizeof(char)) );
			if (*ifacelist == NULL) {
				panicexit(__FILE__, __LINE__);
			}
			strncat(*ifacelist, ifa->ifa_name, strlen(ifa->ifa_name));
			strcat(*ifacelist, " ");
			if (!showspeed) {
				continue;
			}
			speed = getifspeed(ifa->ifa_name);
			if (speed > 0) {
				snprintf(temp, 64, "(%u Mbit) ", speed);
				*ifacelist = realloc(*ifacelist, ( ( strlen(*ifacelist) + strlen(temp) + 1 ) * sizeof(char)) );
				if (*ifacelist == NULL) {
					panicexit(__FILE__, __LINE__);
				}
				strncat(*ifacelist, temp, strlen(temp));
			}
		}

		freeifaddrs(ifap);
		return 1;
	}

#endif

	return 0;
}

int readproc(const char *iface)
{
	FILE *fp;
	char temp[4][64], procline[512], *proclineptr, ifaceid[33];
	int check;

	if ((fp=fopen(PROCNETDEV, "r"))==NULL) {
		if (debug)
			printf("Error: Unable to read %s: %s\n", PROCNETDEV, strerror(errno));
		return 0;
	}

	strncpy_nt(ifaceid, iface, 32);
	strcat(ifaceid, ":");

	check = 0;
	while (fgets(procline, 512, fp)!=NULL) {
		sscanf(procline, "%63s", temp[0]);
		if (strncmp(ifaceid, temp[0], strlen(ifaceid))==0) {
			/* if (debug)
				printf("\n%s\n", procline); */
			check = 1;
			break;
		}
	}
	fclose(fp);

	if (check==0) {
		if (debug)
			printf("Requested interface \"%s\" not found.\n", iface);
		return 0;
	} else {

		strncpy_nt(ifinfo.name, iface, 32);

		/* get rx and tx from procline */
		proclineptr = strchr(procline, ':');
		sscanf(proclineptr+1, "%63s %63s %*s %*s %*s %*s %*s %*s %63s %63s", temp[0], temp[1], temp[2], temp[3]);

		ifinfo.rx = strtoull(temp[0], (char **)NULL, 0);
		ifinfo.tx = strtoull(temp[2], (char **)NULL, 0);

		/* daemon doesn't need packet data */
		if (!noexit) {
			ifinfo.rxp = strtoull(temp[1], (char **)NULL, 0);
			ifinfo.txp = strtoull(temp[3], (char **)NULL, 0);
		}

		ifinfo.filled = 1;
	}

	return 1;
}

int readsysclassnet(const char *iface)
{
	FILE *fp;
	char path[64], file[76], buffer[64];

	strncpy_nt(ifinfo.name, iface, 32);

	snprintf(path, 64, "%s/%s/statistics", SYSCLASSNET, iface);

	if (debug)
		printf("path: %s\n", path);

	/* rx bytes */
	snprintf(file, 76, "%s/rx_bytes", path);
	if ((fp=fopen(file, "r"))==NULL) {
		if (debug)
			printf("Unable to read: %s - %s\n", file, strerror(errno));
		return 0;
	} else {
		if (fgets(buffer, 64, fp)!=NULL) {
			ifinfo.rx = strtoull(buffer, (char **)NULL, 0);
		} else {
			fclose(fp);
			return 0;
		}
	}
	fclose(fp);

	/* tx bytes */
	snprintf(file, 76, "%s/tx_bytes", path);
	if ((fp=fopen(file, "r"))==NULL) {
		if (debug)
			printf("Unable to read: %s - %s\n", file, strerror(errno));
		return 0;
	} else {
		if (fgets(buffer, 64, fp)!=NULL) {
			ifinfo.tx = strtoull(buffer, (char **)NULL, 0);
		} else {
			fclose(fp);
			return 0;
		}
	}
	fclose(fp);

	/* daemon doesn't need packet data */
	if (!noexit) {

		/* rx packets */
		snprintf(file, 76, "%s/rx_packets", path);
		if ((fp=fopen(file, "r"))==NULL) {
			if (debug)
				printf("Unable to read: %s - %s\n", file, strerror(errno));
			return 0;
		} else {
			if (fgets(buffer, 64, fp)!=NULL) {
				ifinfo.rxp = strtoull(buffer, (char **)NULL, 0);
			} else {
				fclose(fp);
				return 0;
			}
		}
		fclose(fp);

		/* tx packets */
		snprintf(file, 76, "%s/tx_packets", path);
		if ((fp=fopen(file, "r"))==NULL) {
			if (debug)
				printf("Unable to read: %s - %s\n", file, strerror(errno));
			return 0;
		} else {
			if (fgets(buffer, 64, fp)!=NULL) {
				ifinfo.txp = strtoull(buffer, (char **)NULL, 0);
			} else {
				fclose(fp);
				return 0;
			}
		}
		fclose(fp);
	}

	ifinfo.filled = 1;

	return 1;
}

#if defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__) || defined(__APPLE__) || defined(__FreeBSD_kernel__)
int getifdata(const char *iface, struct if_data *ifd)
{
	struct ifaddrs *ifap, *ifa;
	int check = 0;

	if (getifaddrs(&ifap) < 0) {
		if (debug)
			printf("readifaddrs:getifaddrs() failed.\n");
		return 0;
	}
	for (ifa = ifap; ifa; ifa = ifa->ifa_next) {
		if ((strcmp(ifa->ifa_name, iface) == 0) && (ifa->ifa_addr->sa_family == AF_LINK)) {
			if (ifa->ifa_data != NULL) {
				memcpy(ifd, ifa->ifa_data, sizeof(struct if_data));
				check = 1;
			}
			break;
		}
	}
	freeifaddrs(ifap);

	return check;
}

int readifaddrs(const char *iface)
{
	struct if_data ifd;

	if (!getifdata(iface, &ifd)) {
		if (debug)
			printf("Requested interface \"%s\" not found.\n", iface);
		return 0;
	} else {
		strncpy_nt(ifinfo.name, iface, 32);
		ifinfo.rx = ifd.ifi_ibytes;
		ifinfo.tx = ifd.ifi_obytes;
		ifinfo.rxp = ifd.ifi_ipackets;
		ifinfo.txp = ifd.ifi_opackets;
		ifinfo.filled = 1;
	}

	return 1;
}
#endif

uint32_t getifspeed(const char *iface)
{
	uint64_t speed = 0;
#if defined(__linux__)

	FILE *fp;
	char file[64], buffer[64];

	snprintf(file, 64, "%s/%s/speed", SYSCLASSNET, iface);

	if ((fp=fopen(file, "r"))==NULL) {
		if (debug)
			printf("Unable to open: %s - %s\n", file, strerror(errno));
		return 0;
	} else {
		if (fgets(buffer, 64, fp)!=NULL) {
			speed = strtoull(buffer, (char **)NULL, 0);
		} else {
			if (debug)
				printf("Unable to read: %s - %s\n", file, strerror(errno));
			fclose(fp);
			return 0;
		}
	}
	fclose(fp);

#elif defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__) || defined(__APPLE__)  || defined(__FreeBSD_kernel__)

	struct if_data ifd;

	if (!getifdata(iface, &ifd)) {
		if (debug)
			printf("Requested interface \"%s\" not found.\n", iface);
		return 0;
	} else {
		speed = (uint64_t)ifd.ifi_baudrate;
	}

#endif
	if (debug)
		printf("getifspeed: \"%s\": %"PRIu64"\n", iface, speed);

	if (speed > 1000000) {
		speed = 0;
	}
	return (uint32_t)speed;
}

int isifavailable(const char *iface)
{
	int ret = 0, printstatus;

	printstatus = disableprints;
	disableprints = 1;
	ret = getifinfo(iface);
	disableprints = printstatus;

	return ret;
}
