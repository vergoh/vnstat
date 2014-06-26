#include "common.h"
#include "ifinfo.h"
#include "dbaccess.h"
#include "cfg.h"
#include "daemon.h"

void daemonize(void)
{
	int i;
	char str[10];

	if (getppid()==1) {
		return; /* already a daemon */
	}

	i = (int)fork();

	if (i<0) { /* fork error */
		perror("Error: fork");
		exit(EXIT_FAILURE);
	}
	if (i>0) { /* parent exits */
		exit(EXIT_SUCCESS);
	}
	/* child (daemon) continues */

	setsid(); /* obtain a new process group */

	if (cfg.uselogging) {
		snprintf(errorstring, 512, "vnStat daemon %s started. (uid:%d gid:%d)", VNSTATVERSION, (int)getuid(), (int)getgid());
		if (!printe(PT_Info)) {
			printf("Error: Unable to use logfile. Exiting.\n");
			exit(EXIT_FAILURE);
		}
	}

	/* lock / pid file */
	pidfile = open(cfg.pidfile, O_RDWR|O_CREAT, 0644);
	if (pidfile<0) {
		perror("Error: pidfile");
		snprintf(errorstring, 512, "opening pidfile \"%s\" failed (%s), exiting.", cfg.pidfile, strerror(errno));
		printe(PT_Error);
		exit(EXIT_FAILURE); /* can't open */
	}
	if (lockf(pidfile,F_TLOCK,0)<0) {
		perror("Error: pidfile lock");
		snprintf(errorstring, 512, "pidfile \"%s\" lock failed (%s), exiting.", cfg.pidfile, strerror(errno));
		printe(PT_Error);
		exit(EXIT_FAILURE); /* can't lock */
	}

	/* close all descriptors except lock file */
	for (i=getdtablesize();i>=0;--i) {
		if (i!=pidfile) {
			close(i);
		}
	}

	/* redirect standard i/o to null */
	i=open("/dev/null",O_RDWR); /* stdin */

	/* stdout */
	if (dup(i) < 0) {
		perror("Error: dup(stdout)");
		snprintf(errorstring, 512, "dup(stdout) failed, exiting.");
		printe(PT_Error);
		exit(EXIT_FAILURE);
	}
	/* stderr */
	if (dup(i) < 0) {
		perror("Error: dup(stderr)");
		snprintf(errorstring, 512, "dup(stderr) failed, exiting.");
		printe(PT_Error);
		exit(EXIT_FAILURE);
	}

	umask(027); /* set newly created file permissions */

	/* change running directory */
	if (chdir("/") < 0) {
		perror("Error: chdir(/)");
		snprintf(errorstring, 512, "directory change to / failed, exiting.");
		printe(PT_Error);
		exit(EXIT_FAILURE);
	}

	/* first instance continues */
	snprintf(str, 10, "%d\n", (int)getpid());

	/* record pid to pidfile */
	if (write(pidfile,str,strlen(str)) < 0) {
		perror("Error: write(pidfile)");
		snprintf(errorstring, 512, "writing to pidfile %s failed, exiting.", cfg.pidfile);
		printe(PT_Error);
		exit(EXIT_FAILURE);
	}

	signal(SIGCHLD,SIG_IGN); /* ignore child */
	signal(SIGTSTP,SIG_IGN); /* ignore tty signals */
	signal(SIGTTOU,SIG_IGN);
	signal(SIGTTIN,SIG_IGN);

	if (cfg.uselogging==1) {
		snprintf(errorstring, 512, "Daemon running with pid %d.", (int)getpid());
		printe(PT_Info);
	}
}

int addinterfaces(const char *dirname)
{
	char *ifacelist, interface[32];
	int index = 0, count = 0, bwlimit = 0;

	/* get list of currently visible interfaces */
	if (getiflist(&ifacelist)==0) {
		free(ifacelist);
		return 0;
	}

	if (strlen(ifacelist)<2) {
		free(ifacelist);
		return 0;
	}

	if (debug)
		printf("Interface list: \"%s\"\n", ifacelist);

	while (sscanf(ifacelist+index, "%31s", interface)!=EOF) {
		if (debug)
			printf("Processing: \"%s\"\n", interface);

		index += strlen(interface)+1;

		/* skip local interfaces */
		if ((strcmp(interface,"lo")==0) || (strcmp(interface,"lo0")==0) || (strcmp(interface,"sit0")==0)) {
			if (debug)
				printf("skip\n");
			continue;
		}

		/* create database for interface */
		initdb();
		strncpy_nt(data.interface, interface, 32);
		strncpy_nt(data.nick, data.interface, 32);
		if (!getifinfo(interface)) {
			if (debug)
				printf("getifinfo failed, skip\n");
			continue;
		}
		parseifinfo(1);
		if (!writedb(interface, dirname, 1)) {
			continue;
		}
		count++;
		bwlimit = ibwget(interface);
		if (bwlimit > 0) {
			printf("\"%s\" added with %d Mbit bandwidth limit.\n", interface, bwlimit);
		} else {
			printf("\"%s\" added. Warning: no bandwidth limit has been set.\n", interface);
		}
	}

	if (count==1) {
		printf("-> %d interface added.", count);
	} else {
		printf("-> %d interfaces added.", count);
	}

	if (count) {
		printf(" Limits can be modified using the configuration file. See \"man vnstat.conf\".");
	}

	printf("\n");

	free(ifacelist);
	return count;
}

void debugtimestamp(void)
{
	time_t now;
	char timestamp[22];

	now = time(NULL);
	strftime(timestamp, 22, "%Y-%m-%d %H:%M:%S", localtime(&now));
	printf("%s\n", timestamp);
}

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
