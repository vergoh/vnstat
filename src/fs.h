#ifndef FS_H
#define FS_H

int direxists(const char *dir);
int mkpath(const char *dir, const mode_t mode);
void preparevnstatdir(const char *dir, const char *user, const char *group);
void updatedirowner(const char *dir, const char *user, const char *group);

#endif
