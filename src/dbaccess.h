#ifndef DBACCESS_H
#define DBACCESS_H

int readdb(const char *iface, const char *dirname, const int force);
void initdb(void);
int lockdb(int fd, int dbwrite);
int checkdb(const char *iface, const char *dirname);
int removedb(const char *iface, const char *dirname);
int validatedb(void);

#endif
