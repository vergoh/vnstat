#include "common.h"
#include "id.h"

uid_t getuser(const char *user)
{
	const struct passwd *pw;
	uid_t uid;

	if (!strlen(user)) {
		return getuid();
	}

	if (isnumeric(user)) {
		uid = (uid_t)atoi(user);
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
	const struct group *gr;
	gid_t gid;

	if (!strlen(group)) {
		return getgid();
	}

	if (isnumeric(group)) {
		gid = (gid_t)atoi(group);
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

	if (isnumeric(user) && atoi(user) == 0) {
		return;
	}

	uid = getuser(user);

	if (uid == getuser("")) {
		if (debug)
			printf("no user switching needed, already as requested user \"%s\"\n", user);
		return;
	}

	if (!hasroot()) {
		printf("Error: User can only be set as root.\n");
		exit(EXIT_FAILURE);
	}

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

	if (isnumeric(group) && atoi(group) == 0) {
		return;
	}

	gid = getgroup(group);

	if (gid == getgroup("")) {
		if (debug)
			printf("no group switching needed, already as requested group \"%s\"\n", group);
		return;
	}

	if (!hasroot()) {
		printf("Error: Group can only be set as root.\n");
		exit(EXIT_FAILURE);
	}

	if (debug)
		printf("switching to group id %d.\n", gid);

	if (setgid(gid) != 0) {
		perror("Error: setgid");
		exit(EXIT_FAILURE);
	}
}

int hasroot(void)
{
	if (getuid() != 0 && geteuid() != 0) {
		return 0;
	}
	return 1;
}
