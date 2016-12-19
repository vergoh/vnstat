#ifndef FS_H
#define FS_H

int direxists(const char *dir);
int fileexists(const char *file);
int mkpath(const char *dir, const mode_t mode);
void preparevnstatdir(const char *dir, const char *user, const char *group);
void updatedirowner(const char *dir, const char *user, const char *group);
void updatedirownerid(const char *dir, const uid_t uid, const gid_t gid);
void matchdbownerwithdirowner(const char *dir);
int getdirowner(const char *dir, uid_t *uid, gid_t *gid);

#endif
