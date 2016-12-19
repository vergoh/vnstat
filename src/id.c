#include "common.h"
#include "id.h"

uid_t getuser(const char *user)
{
	struct passwd *pw;
	uid_t uid;

	if (!strlen(user)) {
		return getuid();
	}

	if (isnumeric(user)) {
		uid = atoi(user);
		pw = getpwuid(uid);
	} else {
		pw = getpwnam(user);
	}

	if (pw == NULL) {
		printf("Error: No such user: \"%s\".\n", user);
		exit(EXIT_FAILURE);
	}

	uid = pw->pw_uid;

	if (debug)
		printf("getuser(%s / %d): %s (%d)\n", user, atoi(user), pw->pw_name, (int)uid);

	return uid;
}

gid_t getgroup(const char *group)
{
	struct group *gr;
	gid_t gid;

	if (!strlen(group)) {
		return getgid();
	}

	if (isnumeric(group)) {
		gid = atoi(group);
		gr = getgrgid(gid);
	} else {
		gr = getgrnam(group);
	}

	if (gr == NULL) {
		printf("Error: No such group: \"%s\".\n", group);
		exit(EXIT_FAILURE);
	}

	gid = gr->gr_gid;

	if (debug)
		printf("getgroup(%s / %d): %s (%d)\n", group, atoi(group), gr->gr_name, (int)gid);

	return gid;
}

void setuser(const char *user)
{
	uid_t uid;

	if (!strlen(user)) {
		return;
	}

	if (getuid() != 0 && geteuid() != 0) {
		printf("Error: User can only be set as root.\n");
		exit(EXIT_FAILURE);
	}

	if (isnumeric(user) && atoi(user) == 0) {
		return;
	}

	uid = getuser(user);

	if (debug)
		printf("switching to user id %d.\n", uid);

	if (setuid(uid) != 0) {
		perror("Error: setuid");
		exit(EXIT_FAILURE);
	}
}

void setgroup(const char *group)
{
	gid_t gid;

	if (!strlen(group)) {
		return;
	}

	if (getuid() != 0 && geteuid() != 0) {
		printf("Error: Group can only be set as root.\n");
		exit(EXIT_FAILURE);
	}

	if (isnumeric(group) && atoi(group) == 0) {
		return;
	}

	gid = getgroup(group);

	if (debug)
		printf("switching to group id %d.\n", gid);

	if (setgid(gid) != 0) {
		perror("Error: setgid");
		exit(EXIT_FAILURE);
	}
}
