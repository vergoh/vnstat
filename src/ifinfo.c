#include "common.h"
#include "ibw.h"
#include "ifinfo.h"

int getifinfo(const char *iface)
{
	char inface[MAXIFLEN];

	ifinfo.filled = 0;
	ifinfo.timestamp = 0;

#if defined(__linux__) && HAVE_LINUX_RTNETLINK_H
	if (cfg.is64bit == -2) {
#if HAVE_DECL_IFLA_STATS64
		ifinfo.is64bit = 1;
#else
		ifinfo.is64bit = 0;
#endif
	} else {
		ifinfo.is64bit = (short)cfg.is64bit;
	}
#else
	if (cfg.is64bit < 0) {
		ifinfo.is64bit = -1;
	} else {
		ifinfo.is64bit = (short)cfg.is64bit;
	}
#endif

	if (strcmp(iface, "default") == 0) {
		strncpy_nt(inface, cfg.iface, MAXIFLEN);
	} else {
		strncpy_nt(inface, iface, MAXIFLEN);
	}

#if defined(__linux__) || defined(CHECK_VNSTAT)
	/* try getting interface info from /proc */
	if (readproc(inface) == 1) {
		ifinfo.timestamp = time(NULL);
		return 1;
	} else {
		if (debug)
			printf("Failed to use %s as source.\n", PROCNETDEV);
	}

	/* try getting interface info from /sys */
	if (readsysclassnet(inface) == 1) {
		ifinfo.timestamp = time(NULL);
		return 1;
	}

#elif defined(BSD_VNSTAT)
	if (readifaddrs(inface) == 1) {
		ifinfo.timestamp = time(NULL);
		return 1;
	}
#endif

	snprintf(errorstring, 1024, "Unable to get interface \"%s\" statistics.", inface);
	printe(PT_Error);
	return 0;
}

int getifliststring(char **ifacelist, int showspeed)
{
	char temp[64];
	iflist *ifl = NULL, *ifl_iterator = NULL;

	/* initialize list string */
	*ifacelist = (char *)malloc(sizeof(char));
	if (*ifacelist == NULL) {
		panicexit(__FILE__, __LINE__);
	}
	*ifacelist[0] = '\0';

	if (getiflist(&ifl, showspeed, 1) > 0) {

		ifl_iterator = ifl;

		while (ifl_iterator != NULL) {
			*ifacelist = (char *)realloc(*ifacelist, ((strlen(*ifacelist) + strlen(ifl_iterator->interface) + 2) * sizeof(char)));
			if (*ifacelist == NULL) {
				panicexit(__FILE__, __LINE__);
			}
			strcat(*ifacelist, ifl_iterator->interface);
			strcat(*ifacelist, " ");

			if (showspeed && ifl_iterator->bandwidth > 0) {
				snprintf(temp, 64, "(%u Mbit) ", ifl_iterator->bandwidth);
				*ifacelist = (char *)realloc(*ifacelist, ((strlen(*ifacelist) + strlen(temp) + 1) * sizeof(char)));
				if (*ifacelist == NULL) {
					panicexit(__FILE__, __LINE__);
				}
				strcat(*ifacelist, temp);
			}

			ifl_iterator = ifl_iterator->next;
		}

		iflistfree(&ifl);
		return 1;
	}

	iflistfree(&ifl);
	return 0;
}

int getiflist(iflist **ifl, const int getspeed, const int validate)
{
	int result = 0;
	int32_t maxbw = cfg.maxbw;

	/* disable MaxBandwidth during list creation to avoid having it being set as the default value when nothing is detected */
	cfg.maxbw = 0;
#if defined(__linux__) || defined(CHECK_VNSTAT)
	result = getiflist_linux(ifl, getspeed, validate);
#elif defined(BSD_VNSTAT)
	result = getiflist_bsd(ifl, getspeed, validate);
#else
#error vnStat only supports Linux and BSD like systems
#endif
	cfg.maxbw = maxbw;
	return result;
}

#if defined(__linux__) || defined(CHECK_VNSTAT)
int getiflist_linux(iflist **ifl, const int getspeed, const int validate)
{
	char temp[64];
	char interface[MAXIFLEN];
	FILE *fp;
	DIR *dp;
	struct dirent *di;
	char procline[512];
	uint32_t bwlimit = 0;

	if ((fp = fopen(PROCNETDEV, "r")) != NULL) {

		/* make list of interfaces */
		while (fgets(procline, 512, fp) != NULL) {
			sscanf(procline, "%63s", temp);
			if (strlen(temp) > 0 && (isdigit(temp[(strlen(temp) - 1)]) || temp[(strlen(temp) - 1)] == ':')) {
				sscanf(temp, "%31[^':']s", interface);
				if (validate && !isifvalid(interface)) {
					continue;
				}
				bwlimit = 0;
				if (getspeed && ibwget(interface, &bwlimit)) {
					iflistadd(ifl, interface, bwlimit);
				} else {
					iflistadd(ifl, interface, 0);
				}
			}
		}

		fclose(fp);
		return 1;

	} else {

		if ((dp = opendir(SYSCLASSNET)) != NULL) {

			/* make list of interfaces */
			while ((di = readdir(dp))) {
				if (di->d_name[0] == '.' || strlen(di->d_name) > MAXIFLEN - 1) {
					continue;
				}
				if (validate && !isifvalid(di->d_name)) {
					continue;
				}
				bwlimit = 0;
				if (getspeed && ibwget(di->d_name, &bwlimit)) {
					iflistadd(ifl, di->d_name, bwlimit);
				} else {
					iflistadd(ifl, di->d_name, 0);
				}
			}

			closedir(dp);
			return 1;
		}
	}

	return 0;
}
#elif defined(BSD_VNSTAT)
int getiflist_bsd(iflist **ifl, const int getspeed, const int validate)
{
	struct ifaddrs *ifap, *ifa;
	uint32_t bwlimit = 0;

	if (getifaddrs(&ifap) >= 0) {

		/* make list of interfaces */
		for (ifa = ifap; ifa; ifa = ifa->ifa_next) {
			if (ifa->ifa_addr->sa_family != AF_LINK || strlen(ifa->ifa_name) > MAXIFLEN - 1) {
				continue;
			}
			if (validate && !isifvalid(ifa->ifa_name)) {
				continue;
			}
			bwlimit = 0;
			if (getspeed && ibwget(ifa->ifa_name, &bwlimit)) {
				iflistadd(ifl, ifa->ifa_name, getifspeed(ifa->ifa_name));
			} else {
				iflistadd(ifl, ifa->ifa_name, 0);
			}
		}

		freeifaddrs(ifap);
		return 1;
	}

	return 0;
}
#endif

int readproc(const char *iface)
{
	FILE *fp;
	char temp[4][64], procline[512], *proclineptr, ifaceid[MAXIFLEN + 1];
	int check;

	if ((fp = fopen(PROCNETDEV, "r")) == NULL) {
		if (debug)
			printf("Error (debug): Unable to read %s: %s\n", PROCNETDEV, strerror(errno));
		return 0;
	}

	strncpy_nt(ifaceid, iface, MAXIFLEN);
	strcat(ifaceid, ":");

	check = 0;
	while (fgets(procline, 512, fp) != NULL) {
		sscanf(procline, "%63s", temp[0]);
		if (strncmp(ifaceid, temp[0], strlen(ifaceid)) == 0) {
			/* if (debug)
				printf("\n%s\n", procline); */
			check = 1;
			break;
		}
	}
	fclose(fp);

	if (check == 0) {
		if (debug)
			printf("Requested interface \"%s\" not found.\n", iface);
		return 0;
	} else {

		strncpy_nt(ifinfo.name, iface, MAXIFLEN);

		/* get rx and tx from procline */
		proclineptr = strchr(procline, ':');
		sscanf(proclineptr + 1, "%63s %63s %*s %*s %*s %*s %*s %*s %63s %63s", temp[0], temp[1], temp[2], temp[3]);

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

	strncpy_nt(ifinfo.name, iface, MAXIFLEN);

	snprintf(path, 64, "%s/%s/statistics", SYSCLASSNET, iface);

	if (debug)
		printf("path: %s\n", path);

	/* rx bytes */
	snprintf(file, 76, "%s/rx_bytes", path);
	if ((fp = fopen(file, "r")) == NULL) {
		if (debug)
			printf("Unable to read: %s - %s\n", file, strerror(errno));
		return 0;
	} else {
		if (fgets(buffer, 64, fp) != NULL) {
			ifinfo.rx = strtoull(buffer, (char **)NULL, 0);
		} else {
			fclose(fp);
			return 0;
		}
	}
	fclose(fp);

	/* tx bytes */
	snprintf(file, 76, "%s/tx_bytes", path);
	if ((fp = fopen(file, "r")) == NULL) {
		if (debug)
			printf("Unable to read: %s - %s\n", file, strerror(errno));
		return 0;
	} else {
		if (fgets(buffer, 64, fp) != NULL) {
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
		if ((fp = fopen(file, "r")) == NULL) {
			if (debug)
				printf("Unable to read: %s - %s\n", file, strerror(errno));
			return 0;
		} else {
			if (fgets(buffer, 64, fp) != NULL) {
				ifinfo.rxp = strtoull(buffer, (char **)NULL, 0);
			} else {
				fclose(fp);
				return 0;
			}
		}
		fclose(fp);

		/* tx packets */
		snprintf(file, 76, "%s/tx_packets", path);
		if ((fp = fopen(file, "r")) == NULL) {
			if (debug)
				printf("Unable to read: %s - %s\n", file, strerror(errno));
			return 0;
		} else {
			if (fgets(buffer, 64, fp) != NULL) {
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

#if defined(BSD_VNSTAT)
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
		strncpy_nt(ifinfo.name, iface, MAXIFLEN);
		ifinfo.rx = ifd.ifi_ibytes;
		ifinfo.tx = ifd.ifi_obytes;
		ifinfo.rxp = ifd.ifi_ipackets;
		ifinfo.txp = ifd.ifi_opackets;
		ifinfo.filled = 1;

		if (cfg.is64bit == -2) {
			if (sizeof(ifd.ifi_ibytes) == 8) {
				ifinfo.is64bit = 1;
			} else {
				ifinfo.is64bit = 0;
			}
		}
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

	// value in file is Mbits per second
	snprintf(file, 64, "%s/%s/speed", SYSCLASSNET, iface);

	if ((fp = fopen(file, "r")) == NULL) {
		if (debug)
			printf("Unable to open: %s - %s\n", file, strerror(errno));
		return 0;
	} else {
		if (fgets(buffer, 64, fp) != NULL) {
			speed = strtoull(buffer, (char **)NULL, 0);
		} else {
			if (debug)
				printf("Unable to read: %s - %s\n", file, strerror(errno));
			fclose(fp);
			return 0;
		}
	}
	fclose(fp);

#elif defined(BSD_VNSTAT)

	struct if_data ifd;

	if (!getifdata(iface, &ifd)) {
		if (debug)
			printf("Requested interface \"%s\" not found.\n", iface);
		return 0;
	} else {
		// value in ifi_baudrate is bits per second
		speed = (uint64_t)ifd.ifi_baudrate;
		if (speed < 1000000) {
			if (debug)
				printf("getifspeed: ignoring too small raw value %" PRIu64 " for interface \"%s\"\n", speed, iface);
			return 0;
		} else {
			speed /= 1000000;
		}
	}

#endif

	if (debug)
		printf("getifspeed: \"%s\": %" PRIu64 "\n", iface, speed);

	if (speed > 1000000) {
		speed = 0;
	}
	return (uint32_t)speed;
}

int isifavailable(const char *iface)
{
	int ret = 0, printstatus;

	printstatus = disableprinte;
	disableprinte = 1;
	ret = getifinfo(iface);
	disableprinte = printstatus;

	return ret;
}

int isifvalid(const char *iface)
{
	if (strstr(iface, ":") != NULL) {
		return 0;
	} else if (strcmp(iface, "lo") == 0) {
		return 0;
	} else if (strcmp(iface, "lo0") == 0) {
		return 0;
	} else if (strcmp(iface, "sit0") == 0) {
		return 0;
	}
	return 1;
}

/* tun interfaces have speed hardcoded as 10 in the Linux kernel regardless of used interface and can't
be trusted to provide correct details as a result even if the information is available:
https://github.com/torvalds/linux/blob/9d31d2338950293ec19d9b095fbaa9030899dcb4/drivers/net/tun.c#L3456 */
int istun(const char *iface)
{
#if defined(__linux__)
	if (strlen(iface) > 3 && strncmp(iface, "tun", 3) == 0) {
		if (isdigit(iface[3])) {
			return 1;
		}
	}
#else
	// suppress unused variable warning
	(void)iface;
#endif
	return 0;
}
