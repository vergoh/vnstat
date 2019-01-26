#ifndef FS_H
#define FS_H

/* O_CLOEXEC is specified starting POSIX.1-2008 / glibc 2.12 / Linux 2.6.23 */
#if HAVE_DECL_O_CLOEXEC
#define FS_OPEN_RO_FLAGS O_RDONLY | O_CLOEXEC
#else
#define FS_OPEN_RO_FLAGS O_RDONLY
#endif

int direxists(const char *dir);
int fileexists(const char *file);
int mkpath(const char *dir, const mode_t mode);
void preparevnstatdir(const char *dir, const char *user, const char *group);
void updatedirowner(const char *dir, const char *user, const char *group);
void updatedirownerid(const char *dir, const uid_t uid, const gid_t gid);
void matchdbownerwithdirowner(const char *dir);
int getdirowner(const char *dir, uid_t *uid, gid_t *gid);

#endif
