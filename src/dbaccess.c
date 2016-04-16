#include "common.h"
#include "dbaccess.h"

int readdb(const char *iface, const char *dirname)
{
	FILE *db;
	char file[512], backup[512];

	snprintf(file, 512, "%s/%s", dirname, iface);
	snprintf(backup, 512, "%s/.%s", dirname, iface);

	if ((db=fopen(file,"r"))==NULL) {
		snprintf(errorstring, 512, "Unable to read database \"%s\": %s", file, strerror(errno));
		printe(PT_Error);

		/* create new database template */
		initdb();
		strncpy_nt(data.interface, iface, 32);
		strncpy_nt(data.nick, data.interface, 32);
		return 1;
	}

	/* lock file */
	if (!lockdb(fileno(db), 0)) {
		fclose(db);
		return -1;
	}

	if (fread(&data,sizeof(DATA),1,db)==0) {
		data.version=-1;
		if (debug) {
			printf("db: Database read failed for file \"%s\".\n", file);
		}
	} else {
		if (debug) {
			printf("db: Database loaded for interface \"%s\"...\n", data.interface);
		}
	}

	if (data.version == DBVERSION) {
		if (!validatedb()) {
			data.version=-1;
			if (debug) {
				printf("db: Database for interface \"%s\" fails to validate, trying with backup\n", data.interface);
			}
		}
	}

	/* convert old database to new format if necessary */
	if (data.version<DBVERSION) {
		if (data.version!=-1) {
			snprintf(errorstring, 512, "Trying to convert database \"%s\" (v%d) to current db format", file, data.version);
			printe(PT_Info);
		}

		if ((data.version==-1) || (!convertdb())) {

			/* close current db and try using backup if database conversion failed */
			fclose(db);
			if ((db=fopen(backup,"r"))==NULL) {
				snprintf(errorstring, 512, "Unable to open backup database \"%s\": %s", backup, strerror(errno));
				printe(PT_Error);
				if (noexit) {
					return -1;
				} else {
					exit(EXIT_FAILURE);
				}
			}

			/* lock file */
			if (!lockdb(fileno(db), 0)) {
				fclose(db);
				return -1;
			}

			if (fread(&data,sizeof(DATA),1,db)==0) {
				snprintf(errorstring, 512, "Database load failed even when using backup (%s). Aborting.", strerror(errno));
				printe(PT_Error);
				fclose(db);

				if (noexit) {
					return -1;
				} else {
					exit(EXIT_FAILURE);
				}
			} else {
				if (debug) {
					printf("db: Backup database loaded for interface \"%s\"...\n", data.interface);
				}
			}

			if (data.version == DBVERSION) {
				if (!validatedb()) {
					data.version=-1;
					if (debug) {
						printf("db: Backup database for interface \"%s\" fails to validate\n", data.interface);
					}
				}
			}

			if (data.version!=DBVERSION) {
				if ((data.version==-1) || (!convertdb())) {
					snprintf(errorstring, 512, "Unable to use database \"%s\" or backup database \"%s\".", file, backup);
					printe(PT_Error);
					fclose(db);

					if (noexit) {
						return -1;
					} else {
						exit(EXIT_FAILURE);
					}
				}
			}
			snprintf(errorstring, 512, "Database possibly corrupted, using backup instead.");
			printe(PT_Info);
		}

	} else if (data.version>DBVERSION) {
		snprintf(errorstring, 512, "Downgrading database \"%s\" (v%d) is not supported.", file, data.version);
		printe(PT_Error);
		fclose(db);

		if (noexit) {
			return -1;
		} else {
			exit(EXIT_FAILURE);
		}
	}

	fclose(db);

	if (strcmp(data.interface,iface)) {
		snprintf(errorstring, 512, "Warning:\nThe previous interface for this file was \"%s\".",data.interface);
		printe(PT_Multiline);
		snprintf(errorstring, 512, "It has now been replaced with \"%s\".",iface);
		printe(PT_Multiline);
		snprintf(errorstring, 512, "You can ignore this message if you renamed the filename.");
		printe(PT_Multiline);
		snprintf(errorstring, 512, "Interface name mismatch, renamed \"%s\" -> \"%s\"", data.interface, iface);
		printe(PT_ShortMultiline);
		if (strcmp(data.interface, data.nick)==0) {
			strncpy_nt(data.nick, iface, 32);
		}
		strncpy_nt(data.interface, iface, 32);
	}

	return 0;
}

void initdb(void)
{
	int i;
	time_t current;
	struct tm *d;

	current=time(NULL);
	d=localtime(&current);

	/* set default values for a new database */
	data.version=DBVERSION;
	data.active=1;
	data.totalrx=0;
	data.totaltx=0;
	data.currx=0;
	data.curtx=0;
	data.totalrxk=0;
	data.totaltxk=0;
	data.lastupdated=current;
	data.created=current;

	/* days */
	for (i=0;i<=29;i++) {
		data.day[i].rx=0;
		data.day[i].tx=0;
		data.day[i].rxk=0;
		data.day[i].txk=0;
		data.day[i].date=0;
		data.day[i].used=0;
	}

	/* months */
	for (i=0;i<=11;i++) {
		data.month[i].rx=0;
		data.month[i].tx=0;
		data.month[i].rxk=0;
		data.month[i].txk=0;
		data.month[i].month=0;
		data.month[i].used=0;
	}

	/* top10 */
	for (i=0;i<=9;i++) {
		data.top10[i].rx=0;
		data.top10[i].tx=0;
		data.top10[i].rxk=0;
		data.top10[i].txk=0;
		data.top10[i].date=0;
		data.top10[i].used=0;
	}

	/* hours */
	for (i=0;i<=23;i++) {
		data.hour[i].rx=0;
		data.hour[i].tx=0;
		data.hour[i].date=0;
	}

	data.day[0].used=data.month[0].used=1;
	data.day[0].date=current;

	/* calculate new date for current month if current day is less
           than the set monthrotate value so that new databases begin
           from the right month */
	if (d->tm_mday < cfg.monthrotate) {
		d->tm_mday=cfg.monthrotate;
		d->tm_mon--;
		data.month[0].month=mktime(d);
	} else {
		data.month[0].month=current;
	}

	data.btime=MAX32;
}

int writedb(const char *iface, const char *dirname, int newdb)
{
	FILE *db;
	char file[512], backup[512];

	snprintf(file, 512, "%s/%s", dirname, iface);
	snprintf(backup, 512, "%s/.%s", dirname, iface);

	/* try to make backup of old data if this isn't a new database */
	if (!newdb && !backupdb(file, backup)) {
		snprintf(errorstring, 512, "Unable to create database backup \"%s\".", backup);
		printe(PT_Error);
		return 0;
	}

	/* make sure version stays correct */
	data.version=DBVERSION;

	if ((db=fopen(file,"w"))==NULL) {
		snprintf(errorstring, 512, "Unable to open database \"%s\" for writing: %s", file, strerror(errno));
		printe(PT_Error);
		return 0;
	}

	/* lock file */
	if (!lockdb(fileno(db), 1)) {
		fclose(db);
		return 0;
	}

	/* update timestamp when not merging */
	if (newdb!=2) {
		data.lastupdated=time(NULL);
	}

	if (fwrite(&data,sizeof(DATA),1,db)==0) {
		snprintf(errorstring, 512, "Unable to write database \"%s\": %s", file, strerror(errno));
		printe(PT_Error);
		fclose(db);
		return 0;
	} else {
		if (debug) {
			printf("db: Database \"%s\" saved.\n", file);
		}
		fclose(db);
		if ((newdb) && (noexit==0)) {
			snprintf(errorstring, 512, "-> A new database has been created.");
			printe(PT_Info);
		}
	}

	return 1;
}

int backupdb(const char *current, const char *backup)
{
	FILE *bf;
	int c, b, bytes;
	char buffer[512];

	/* from */
	if ((c = open(current, O_RDONLY)) == -1) {
		return 0;
	}

	/* to, fopen() in order to get file mode bits correctly */
	if ((bf = fopen(backup, "w")) == NULL) {
		close(c);
		return 0;
	}
	b = fileno(bf);

	/* copy data */
	while((bytes = (int)read(c, buffer, sizeof(buffer))) > 0) {
		if (write(b, buffer, bytes) < 0) {
			close(c);
			fclose(bf);
			return 0;
		}
	}

	close(c);
	fclose(bf);

	return 1;
}

int convertdb(void)
{
	/* corrupted or unknown version handling */
	if (data.version==0) {
		snprintf(errorstring, 512, "Unable to convert corrupted database.");
		printe(PT_Error);
		return 0;
	} else if (data.version<DBVERSION) {
		snprintf(errorstring, 512, "Database upgrade from version \"%d\" is not supported.", data.version);
		printe(PT_Error);
		return 0;
	} else if (data.version>DBVERSION) {
		snprintf(errorstring, 512, "Unable to downgrade database from version \"%d\".", data.version);
		printe(PT_Error);
		return 0;
	} else if (data.version!=DBVERSION) {
		snprintf(errorstring, 512, "Unable to convert database version \"%d\".", data.version);
		printe(PT_Error);
		return 0;
	}

	return 1;
}

int lockdb(int fd, int dbwrite)
{
	int operation, locktry=1;

	/* lock only if configured to do so */
	if (!cfg.flock) {
		return 1;
	}

	if (dbwrite) {
		operation = LOCK_EX|LOCK_NB;
	} else {
		operation = LOCK_SH|LOCK_NB;
	}

	/* try locking file */
	while (flock(fd, operation)!=0) {

		if (debug) {
			printf("db: Database access locked (%d, %d)\n", dbwrite, locktry);
		}

		/* give up if lock can't be obtained */
		if (locktry>=LOCKTRYLIMIT) {
			if (dbwrite) {
				snprintf(errorstring, 512, "Locking database file for write failed for %d tries:\n%s (%d)", locktry, strerror(errno), errno);
			} else {
				snprintf(errorstring, 512, "Locking database file for read failed for %d tries:\n%s (%d)", locktry, strerror(errno), errno);
			}
			printe(PT_Error);
			return 0;
		}

		/* someone else has the lock */
		if (errno==EWOULDBLOCK) {
			sleep(1);

		/* real error */
		} else {
			if (dbwrite) {
				snprintf(errorstring, 512, "Locking database file for write failed:\n%s (%d)", strerror(errno), errno);
			} else {
				snprintf(errorstring, 512, "Locking database file for read failed:\n%s (%d)", strerror(errno), errno);
			}
			printe(PT_Error);
			return 0;
		}

		locktry++;
	}

	return 1;
}

int checkdb(const char *iface, const char *dirname)
{
	char file[512];
	struct statvfs buf;

	snprintf(file, 512, "%s/%s", dirname, iface);

	if (statvfs(file, &buf)==0) {
		return 1; /* file exists */
	} else {
		return 0; /* no file or some other error */
	}
}

int removedb(const char *iface, const char *dirname)
{
	char file[512];

	/* remove backup first */
	snprintf(file, 512, "%s/.%s", dirname, iface);
	unlink(file);

	snprintf(file, 512, "%s/%s", dirname, iface);
	if (unlink(file)!=0) {
		return 0;
	}

	return 1;
}

void cleanhours(void)
{
	int i, day, hour;
	time_t current;
	struct tm *d;

	current=time(NULL);

	/* remove old data if needed */
	for (i=0;i<=23;i++) {
		if ( (data.hour[i].date!=0) && (data.hour[i].date<=(current-86400)) ) { /* 86400 = 24 hours = too old */
			data.hour[i].rx=0;
			data.hour[i].tx=0;
			data.hour[i].date=0;
			if (debug) {
				printf("db: Hour %d (%u) cleaned.\n", i, (unsigned int)data.hour[i].date);
			}
		}
	}

	/* clean current if it's not from today to avoid incrementing old data */
	d=localtime(&current);
	day=d->tm_mday;
	hour=d->tm_hour;
	d=localtime(&data.hour[hour].date);
	if (d->tm_mday!=day) {
		data.hour[hour].rx=0;
		data.hour[hour].tx=0;
		if (debug) {
			printf("db: Current hour %d (%u) cleaned.\n", hour, (unsigned int)data.hour[hour].date);
		}
		data.hour[hour].date=current;
	}
}

void rotatedays(void)
{
	int i, j;
	time_t current;
	struct tm *d;

	for (i=29;i>=1;i--) {
		data.day[i].rx=data.day[i-1].rx;
		data.day[i].tx=data.day[i-1].tx;
		data.day[i].rxk=data.day[i-1].rxk;
		data.day[i].txk=data.day[i-1].txk;
		data.day[i].date=data.day[i-1].date;
		data.day[i].used=data.day[i-1].used;
	}

	current=time(NULL);

	data.day[0].rx=0;
	data.day[0].tx=0;
	data.day[0].rxk=0;
	data.day[0].txk=0;
	data.day[0].date=current;

	if (debug) {
		d=localtime(&data.day[0].date);
		printf("db: Days rotated. Current date: %d.%d.%d\n", d->tm_mday, d->tm_mon+1, d->tm_year+1900);
	}

	/* top10 update */
	for (i=0;i<=9;i++) {
		if ( data.day[1].rx+data.day[1].tx >= data.top10[i].rx+data.top10[i].tx ) {

			/* if MBs are same but kB smaller then continue searching */
			if ( (data.day[1].rx+data.day[1].tx == data.top10[i].rx+data.top10[i].tx) && (data.day[1].rxk+data.day[1].txk <= data.top10[i].rxk+data.top10[i].txk) ) {
				continue;
			}

			for (j=9;j>=i+1;j--) {
				data.top10[j].rx=data.top10[j-1].rx;
				data.top10[j].tx=data.top10[j-1].tx;
				data.top10[j].rxk=data.top10[j-1].rxk;
				data.top10[j].txk=data.top10[j-1].txk;
				data.top10[j].date=data.top10[j-1].date;
				data.top10[j].used=data.top10[j-1].used;
			}
			data.top10[i].rx=data.day[1].rx;
			data.top10[i].tx=data.day[1].tx;
			data.top10[i].rxk=data.day[1].rxk;
			data.top10[i].txk=data.day[1].txk;
			data.top10[i].date=data.day[1].date;
			data.top10[i].used=data.day[1].used;
			break;
		}
	}

	if (debug) {
		printf("db: Top10 updated.\n");
	}

}

void rotatemonths(void)
{
	int i;
	time_t current;
	struct tm *d;

	for (i=11;i>=1;i--) {
		data.month[i].rx=data.month[i-1].rx;
		data.month[i].tx=data.month[i-1].tx;
		data.month[i].rxk=data.month[i-1].rxk;
		data.month[i].txk=data.month[i-1].txk;
		data.month[i].month=data.month[i-1].month;
		data.month[i].used=data.month[i-1].used;
	}

	current=time(NULL);

	data.month[0].rx=0;
	data.month[0].tx=0;
	data.month[0].rxk=0;
	data.month[0].txk=0;
	data.month[0].month=current;

	if (debug) {
		d=localtime(&data.month[0].month);
		printf("db: Months rotated. Current month: \"%d\".\n", d->tm_mon+1);
	}
}

void cleartop10(const char *iface, const char *dirname)
{
	int i;

	if (readdb(iface, dirname)!=0) {
		exit(EXIT_FAILURE);
	}

	for (i=0; i<=9; i++) {
		data.top10[i].rx=data.top10[i].tx=0;
		data.top10[i].rxk=data.top10[i].txk=0;
		data.top10[i].used=0;
	}

	writedb(iface, dirname, 0);
	printf("Top10 cleared for interface \"%s\".\n", data.interface);
}

void rebuilddbtotal(const char *iface, const char *dirname)
{
	int i;

	if (readdb(iface, dirname)!=0) {
		exit(EXIT_FAILURE);
	}

	data.totalrx=data.totaltx=data.totalrxk=data.totaltxk=0;
	for (i=0; i<=11; i++) {
		if (data.month[i].used) {
			addtraffic(&data.totalrx, &data.totalrxk, data.month[i].rx, data.month[i].rxk);
			addtraffic(&data.totaltx, &data.totaltxk, data.month[i].tx, data.month[i].txk);
		}
	}

	writedb(iface, dirname, 0);
	printf("Total transfer rebuild completed for interface \"%s\".\n", data.interface);
}

int validatedb(void)
{
	int i, used;
	uint64_t rxsum, txsum;

	if (debug) {
		printf("validating loaded database\n");
	}

	if (data.version>DBVERSION) {
		snprintf(errorstring, 512, "%s: Invalid database version: %d", data.interface, data.version);
		printe(PT_Error);
		return 0;
	}

	if (data.active<0 || data.active>1) {
		snprintf(errorstring, 512, "%s: Invalid database activity status: %d", data.interface, data.active);
		printe(PT_Error);
		return 0;
	}

	if (!strlen(data.interface)) {
		snprintf(errorstring, 512, "Invalid database interface string: %s", data.interface);
		printe(PT_Error);
		return 0;
	}

	if (!data.created || !data.lastupdated || !data.btime) {
		snprintf(errorstring, 512, "%s: Invalid database timestamp.", data.interface);
		printe(PT_Error);
		return 0;
	}

	rxsum = txsum = 0;
	used = 1;
	for (i=0; i<30; i++) {
		if (data.day[i].used<0 || data.day[i].used>1) {
			snprintf(errorstring, 512, "%s: Invalid database daily use information: %d %d", data.interface, i, data.day[i].used);
			printe(PT_Error);
			return 0;
		}
		if (data.day[i].rxk<0 || data.day[i].txk<0) {
			snprintf(errorstring, 512, "%s: Invalid database daily traffic: %d", data.interface, i);
			printe(PT_Error);
			return 0;
		}
		if (data.day[i].used && !used) {
			snprintf(errorstring, 512, "%s: Invalid database daily use order: %d", data.interface, i);
			printe(PT_Error);
			return 0;
		} else if (!data.day[i].used) {
			used = 0;
		}
		if (data.day[i].used) {
			rxsum += data.day[i].rx;
			txsum += data.day[i].tx;
		}
	}

	for (i=1; i<30; i++) {
		if (!data.day[i].used) {
			break;
		}
		if (data.day[i-1].date < data.day[i].date) {
			snprintf(errorstring, 512, "%s: Invalid database daily date order: %u (%d) < %u (%d)", data.interface, (unsigned int)data.day[i-1].date, i-1, (unsigned int)data.day[i].date, i);
			printe(PT_Error);
			return 0;
		}
	}

	if (data.totalrx < rxsum || data.totaltx < txsum) {
		snprintf(errorstring, 512, "%s: Invalid database total traffic compared to daily usage.", data.interface);
		printe(PT_Error);
		return 0;
	}

	rxsum = txsum = 0;
	used = 1;
	for (i=0; i<12; i++) {
		if (data.month[i].used<0 || data.month[i].used>1) {
			snprintf(errorstring, 512, "%s: Invalid database monthly use information: %d %d", data.interface, i, data.month[i].used);
			printe(PT_Error);
			return 0;
		}
		if (data.month[i].rxk<0 || data.month[i].txk<0) {
			snprintf(errorstring, 512, "%s: Invalid database monthly traffic: %d", data.interface, i);
			printe(PT_Error);
			return 0;
		}
		if (data.month[i].used && !used) {
			snprintf(errorstring, 512, "%s: Invalid database monthly use order: %d", data.interface, i);
			printe(PT_Error);
			return 0;
		} else if (!data.month[i].used) {
			used = 0;
		}
		if (data.month[i].used) {
			rxsum += data.month[i].rx;
			txsum += data.month[i].tx;
		}
	}

	for (i=1; i<12; i++) {
		if (!data.month[i].used) {
			break;
		}
		if (data.month[i-1].month < data.month[i].month) {
			snprintf(errorstring, 512, "%s: Invalid database monthly date order: %u (%d) < %u (%d)", data.interface, (unsigned int)data.month[i-1].month, i-1, (unsigned int)data.month[i].month, i);
			printe(PT_Error);
			return 0;
		}
	}

	if (data.totalrx < rxsum || data.totaltx < txsum) {
		snprintf(errorstring, 512, "%s: Invalid database total traffic compared to monthly usage.", data.interface);
		printe(PT_Error);
		return 0;
	}

	used = 1;
	for (i=0; i<10; i++) {
		if (data.top10[i].used<0 || data.top10[i].used>1) {
			snprintf(errorstring, 512, "%s: Invalid database top10 use information: %d %d", data.interface, i, data.top10[i].used);
			printe(PT_Error);
			return 0;
		}
		if (data.top10[i].rxk<0 || data.top10[i].txk<0) {
			snprintf(errorstring, 512, "%s: Invalid database top10 traffic: %d", data.interface, i);
			printe(PT_Error);
			return 0;
		}
		if (data.top10[i].used && !used) {
			snprintf(errorstring, 512, "%s: Invalid database top10 use order: %d", data.interface, i);
			printe(PT_Error);
			return 0;
		} else if (!data.top10[i].used) {
			used = 0;
		}
	}

	return 1;
}

int importdb(const char *filename)
{
	FILE *input;
	char line[512];
	int i, count, linecount, scancount;
	uint64_t tempint;
	DAY day;
	MONTH month;
	HOUR hour;

	if ((input=fopen(filename, "r"))==NULL) {
		printf("Error: opening file \"%s\" failed: %s\n", filename, strerror(errno));
		return 0;
	}

	linecount = 0;
	while (fgets(line, sizeof(line), input) != NULL) {
		if (debug) {
			printf("parsing %s", line);
		}

		if (strlen(line)<6) {
			continue;
		}

		scancount = 0;
		scancount += sscanf(line, "version;%2d", &data.version);
		scancount += sscanf(line, "active;%2d", &data.active);
		scancount += sscanf(line, "interface;%31s", data.interface);
		scancount += sscanf(line, "nick;%31s", data.nick);
		if (sscanf(line, "created;%20"PRIu64, &tempint)) {
			data.created = (time_t)tempint;
			scancount++;
		}
		if (sscanf(line, "updated;%20"PRIu64, &tempint)) {
			data.lastupdated = (time_t)tempint;
			scancount++;
		}
		scancount += sscanf(line, "totalrx;%20"PRIu64, &data.totalrx);
		scancount += sscanf(line, "totaltx;%20"PRIu64, &data.totaltx);
		scancount += sscanf(line, "currx;%20"PRIu64, &data.currx);
		scancount += sscanf(line, "curtx;%20"PRIu64, &data.curtx);
		scancount += sscanf(line, "totalrxk;%10d", &data.totalrxk);
		scancount += sscanf(line, "totaltxk;%10d", &data.totaltxk);
		scancount += sscanf(line, "btime;%20"PRIu64, &data.btime);

		count = sscanf(line, "d;%2d;%20"PRIu64";%20"PRIu64";%20"PRIu64";%10d;%10d;%2d",
				&i, &tempint, &day.rx, &day.tx, &day.rxk, &day.txk, &day.used);
		if (count == 7) {
			if (i >= 0 && i < (int)sizeof(data.day) / (int)sizeof(DAY)) {
				day.date = (time_t)tempint;
				data.day[i] = day;
				scancount++;
			}
		}

		count = sscanf(line, "m;%2d;%20"PRIu64";%20"PRIu64";%20"PRIu64";%10d;%10d;%2d",
				&i, &tempint, &month.rx, &month.tx, &month.rxk, &month.txk, &month.used);
		if (count == 7) {
			if ( i >= 0 && i < (int)sizeof(data.month) / (int)sizeof(MONTH) ) {
				month.month = (time_t)tempint;
				data.month[i] = month;
				scancount++;
			}
		}

		count = sscanf(line, "t;%2d;%20"PRIu64";%20"PRIu64";%20"PRIu64";%10d;%10d;%2d",
				&i, &tempint, &day.rx, &day.tx, &day.rxk, &day.txk, &day.used);
		if (count == 7) {
			if ( i >= 0 && i < (int)sizeof(data.top10) / (int)sizeof(DAY) ) {
				day.date = (time_t)tempint;
				data.top10[i] = day;
				scancount++;
			}
		}

		count = sscanf(line, "h;%2d;%20"PRIu64";%20"PRIu64";%20"PRIu64,
				&i, &tempint, &hour.rx, &hour.tx);
		if (count == 4) {
			if ( i >= 0 && i < (int)sizeof(data.hour) / (int)sizeof(HOUR) ) {
				hour.date = (time_t)tempint;
				data.hour[i] = hour;
				scancount++;
			}
		}

		if (scancount) {
			linecount++;
		}
	}

	fclose(input);

	return linecount;
}
