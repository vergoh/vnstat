#ifndef DAEMON_H
#define DAEMON_H

void daemonize(void);
int addinterfaces(const char *dirname);
void debugtimestamp(void);
uid_t getuser(const char *user);
gid_t getgroup(const char *group);
void setuser(const char *user);
void setgroup(const char *group);

#endif
