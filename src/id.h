#ifndef ID_H
#define ID_H

uid_t getuser(const char *user);
gid_t getgroup(const char *group);
void setuser(const char *user);
void setgroup(const char *group);
int hasroot(void);

#endif
