#include "common.h"
#include "dbaccess.h"

int readdb(const char *iface, const char *dirname)
{
	FILE *db;
	char file[512], backup[512];
	int newdb=0;

	snprintf(file, 512, "%s/%s", dirname, iface);
	snprintf(backup, 512, "%s/.%s", dirname, iface);

	if ((db=fopen(file,"r"))!=NULL) {

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

		/* convert old database to new format if necessary */
		if (data.version<DBVERSION) {
			if (data.version!=-1) {
				snprintf(errorstring, 512, "Trying to convert database \"%s\" (v%d) to current db format", file, data.version);
				printe(PT_Info);
			}

			if ((data.version==-1) || (!convertdb(db))) {

				/* close current db and try using backup if database conversion failed */
				fclose(db);
				if ((db=fopen(backup,"r"))!=NULL) {

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
							printf("db: Database loaded for interface \"%s\"...\n", data.interface);
						}
					}

					if (data.version!=DBVERSION) {
						if (!convertdb(db)) {
							snprintf(errorstring, 512, "Unable to use backup database.");
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
				} else {
					snprintf(errorstring, 512, "Unable to open backup database \"%s\": %s", backup, strerror(errno));
					printe(PT_Error);
					if (noexit) {
						return -1;
					} else {
						exit(EXIT_FAILURE);
					}
				}
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
				strncpy(data.nick, iface, 32);
			}
			strncpy(data.interface, iface, 32);
		}
	} else {
		snprintf(errorstring, 512, "Unable to read database \"%s\": %s", file, strerror(errno));
		printe(PT_Error);

		newdb=1;
		initdb();
		strncpy(data.interface, iface, 32);
		strncpy(data.nick, data.interface, 32);
	}
	return newdb;
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

	if ((db=fopen(file,"w"))!=NULL) {

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
	} else {
		snprintf(errorstring, 512, "Unable to open database \"%s\" for writing: %s", file, strerror(errno));
		printe(PT_Error);
		return 0;
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

int convertdb(FILE *db)
{
	int i, days, mod;
	DATA10 data10;
	DATA12 data12;
	time_t current;
	struct tm *d;
	int month=0, day;
	int tm_mday, tm_mon, tm_year;
	int converted=0;

	current=time(NULL);
	d=localtime(&current);
	days=d->tm_mday-1;

	tm_mday=d->tm_mday;
	tm_mon=d->tm_mon;
	tm_year=d->tm_year;

	/* version 1.0 database format */
	if (data.version==1) {

		snprintf(errorstring, 512, "Converting to db v2...\n");
		printe(PT_Info);

		rewind(db);
		if (fread(&data10, sizeof(DATA10), 1, db)==0) {
			snprintf(errorstring, 512, "Unable to convert corrupted database.");
			printe(PT_Error);
			return 0;
		}

		/* set basic values */
		data12.version=2;
		strncpy(data12.interface, data10.interface, 32);
		strncpy(data12.nick, data10.interface, 32);
		data12.active=1;
		data12.totalrx=data10.totalrx;
		data12.totaltx=data10.totaltx;
		data12.currx=data10.currx;
		data12.curtx=data10.curtx;
		data12.totalrxk=data10.totalrxk;
		data12.totaltxk=data10.totaltxk;
		data12.lastupdated=data10.lastupdated;
		data12.created=data10.created;
		data12.btime=data10.btime;

		/* days */
		for (i=0; i<=29; i++) {
			if (data10.day[i].rx+data10.day[i].tx>0) {
				data12.day[i].rx=data10.day[i].rx;
				data12.day[i].tx=data10.day[i].tx;
				data12.day[i].date=current-(i*86400);
				data12.day[i].used=1;
			} else {
				data12.day[i].rx=0;
				data12.day[i].tx=0;
				data12.day[i].used=0;
			}
		}

		/* months */
		for (i=0; i<=11; i++) {
			if (data10.month[i].rx+data10.month[i].tx>0) {
				data12.month[i].rx=data10.month[i].rx;
				data12.month[i].tx=data10.month[i].tx;

				data12.month[i].month=current-(86400*days);

				/* we have to do this modulo thing to get the number of days right */
				mod=(d->tm_mon-i-1)%12;
				if (mod<0)
					mod=12+mod;
				days+=dmonth(mod);
				data12.month[i].used=1;
			} else {
				data12.month[i].rx=0;
				data12.month[i].tx=0;
				data12.month[i].used=0;
			}
		}

		/* top10 */
		for (i=0; i<=9; i++) {
			if (data10.top10[i].rx+data10.top10[i].tx>0) {
				data12.top10[i].rx=data10.top10[i].rx;
				data12.top10[i].tx=data10.top10[i].tx;

				/* get day */
				day=atoi(data10.top10[i].date+7);

				/* parse month */
				if (strncmp(data10.top10[i].date+4, "Jan", 3)==0) {
					month=0;
				} else if (strncmp(data10.top10[i].date+4, "Feb", 3)==0) {
					month=1;
				} else if (strncmp(data10.top10[i].date+4, "Mar", 3)==0) {
					month=2;
				} else if (strncmp(data10.top10[i].date+4, "Apr", 3)==0) {
					month=3;
				} else if (strncmp(data10.top10[i].date+4, "May", 3)==0) {
					month=4;
				} else if (strncmp(data10.top10[i].date+4, "Jun", 3)==0) {
					month=5;
				} else if (strncmp(data10.top10[i].date+4, "Jul", 3)==0) {
					month=6;
				} else if (strncmp(data10.top10[i].date+4, "Aug", 3)==0) {
					month=7;
				} else if (strncmp(data10.top10[i].date+4, "Sep", 3)==0) {
					month=8;
				} else if (strncmp(data10.top10[i].date+4, "Oct", 3)==0) {
					month=9;
				} else if (strncmp(data10.top10[i].date+4, "Nov", 3)==0) {
					month=10;
				} else if (strncmp(data10.top10[i].date+4, "Dec", 3)==0) {
					month=11;
				} else {
					month=-1;
					snprintf(errorstring, 512, "Convertion for date \"%s\" failed.", data10.top10[i].date);
					printe(PT_Error);

				}

				if (month==-1)
					break;

				/* guess year */
				d->tm_year=tm_year;
				if ((month>tm_mon) || ((month==tm_mon) && (day>tm_mday)))
					d->tm_year--;

				d->tm_mday=day;
				d->tm_mon=month;
				data12.top10[i].date=mktime(d);
				data12.top10[i].used=1;

			} else {
				data12.top10[i].used=0;
				data12.top10[i].rx=0;
				data12.top10[i].tx=0;
			}



		}

		/* reset top10 if there was some problem */
		if (month==-1) {
			for (i=0; i<=9; i++) {
				data12.top10[i].rx=data.top10[i].tx=0;
				data12.top10[i].used=0;
			}
		}

		converted=1;
	} 

	/* version 1.1-1.2 database format */
	if (data.version==2 || converted==1) {

		printf("Converting to db v3...\n");

		/* don't read from file if already in memory */
		if (converted==0) {
			rewind(db);
			if (fread(&data12, sizeof(DATA12), 1, db)==0) {
				snprintf(errorstring, 512, "Unable to convert corrupted database.");
				printe(PT_Error);
				return 0;
			}
		}

		/* set basic values */
		data.version=3;
		strncpy(data.interface, data12.interface, 32);
		strncpy(data.nick, data12.nick, 32);
		data.active=data12.active;
		data.totalrx=data12.totalrx;
		data.totaltx=data12.totaltx;
		data.currx=data12.currx;
		data.curtx=data12.curtx;
		data.totalrxk=data12.totalrxk;
		data.totaltxk=data12.totaltxk;
		data.lastupdated=data12.lastupdated;
		data.created=data12.created;
		data.btime=data12.btime;

		/* days */
		for (i=0; i<=29; i++) {
			if (data12.day[i].used) {
				data.day[i].rx=data12.day[i].rx;
				data.day[i].tx=data12.day[i].tx;
				data.day[i].rxk=data.day[i].txk=0;
				data.day[i].date=data12.day[i].date;
				data.day[i].used=1;
			} else {
				data.day[i].rx=data.day[i].tx=0;
				data.day[i].rxk=data.day[i].txk=0;
				data.day[i].used=0;
			}
		}

		/* months */
		for (i=0; i<=11; i++) {
			if (data12.month[i].used) {
				data.month[i].rx=data12.month[i].rx;
				data.month[i].tx=data12.month[i].tx;
				data.month[i].rxk=data.month[i].txk=0;
				data.month[i].month=data12.month[i].month;
				data.month[i].used=1;
			} else {
				data.month[i].rx=data.month[i].tx=0;
				data.month[i].rxk=data.month[i].txk=0;
				data.month[i].used=0;
			}
		}

		/* top10 */
		for (i=0;i<=9;i++) {
			if (data12.top10[i].used) {
				data.top10[i].rx=data12.top10[i].rx;
				data.top10[i].tx=data12.top10[i].tx;
				data.top10[i].rxk=data.top10[i].txk=0;
				data.top10[i].date=data12.top10[i].date;
				data.top10[i].used=1;
			} else {
				data.top10[i].rx=data.top10[i].tx=0;
				data.top10[i].rxk=data.top10[i].txk=0;
				data.top10[i].date=0;
				data.top10[i].used=0;
			}
		}

		/* hours */
		for (i=0;i<=23;i++) {
			data.hour[i].rx=0;
			data.hour[i].tx=0;
			data.hour[i].date=0;
		}

	}

	/* corrupted or unknown version handling */
	if (data.version==0) {
		snprintf(errorstring, 512, "Unable to convert corrupted database.");
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

	snprintf(errorstring, 512, "Conversion done.");
	printe(PT_Info);

	return 1;
}

int lockdb(int fd, int dbwrite)
{
	int operation, locktry=1;

	/* lock only if configured to do so */
	if (cfg.flock) {

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

	if (data.version>DBVERSION) {
		printf("Invalid database version: %d\n", data.version);
		return 0;
	}

	if (data.active<0 || data.active>1) {
		printf("Invalid database activity status: %d\n", data.active);
		return 0;
	}

	if (!strlen(data.interface)) {
		printf("Invalid database interface string: %s\n", data.interface);
		return 0;
	}

	if (!data.created || !data.lastupdated || !data.btime) {
		printf("Invalid database timestamp.\n");
		return 0;
	}

	rxsum = txsum = 0;
	used = 1;
	for (i=0; i<30; i++) {
		if (data.day[i].used<0 || data.day[i].used>1) {
			printf("Invalid database daily use information: %d %d\n", i, data.day[i].used);
			return 0;
		}
		if (data.day[i].rxk<0 || data.day[i].txk<0) {
			printf("Invalid database daily traffic: %d\n", i);
			return 0;
		}
		if (data.day[i].used && !used) {
			printf("Invalid database daily use order: %d\n", i);
			return 0;
		} else if (!data.day[i].used) {
			used = 0;
		}
		if (data.day[i].used) {
			rxsum += data.day[i].rx;
			txsum += data.day[i].tx;
		}
	}

	if (data.totalrx < rxsum || data.totaltx < txsum) {
		printf("Invalid database total traffic compared to daily usage.\n");
		return 0;
	}

	rxsum = txsum = 0;
	used = 1;
	for (i=0; i<12; i++) {
		if (data.month[i].used<0 || data.month[i].used>1) {
			printf("Invalid database monthly use information: %d %d\n", i, data.month[i].used);
			return 0;
		}
		if (data.month[i].rxk<0 || data.month[i].txk<0) {
			printf("Invalid database monthly traffic: %d\n", i);
			return 0;
		}
		if (data.month[i].used && !used) {
			printf("Invalid database monthly use order: %d\n", i);
			return 0;
		} else if (!data.month[i].used) {
			used = 0;
		}
		if (data.month[i].used) {
			rxsum += data.month[i].rx;
			txsum += data.month[i].tx;
		}
	}

	if (data.totalrx < rxsum || data.totaltx < txsum) {
		printf("Invalid database total traffic compared to monthly usage.\n");
		return 0;
	}

	used = 1;
	for (i=0; i<10; i++) {
		if (data.top10[i].used<0 || data.top10[i].used>1) {
			printf("Invalid database top10 use information: %d %d\n", i, data.top10[i].used);
			return 0;
		}
		if (data.top10[i].rxk<0 || data.top10[i].txk<0) {
			printf("Invalid database top10 traffic: %d\n", i);
			return 0;
		}
		if (data.top10[i].used && !used) {
			printf("Invalid database top10 use order: %d\n", i);
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
	int i, count;
	uint64_t tempint;
	DAY day;
	MONTH month;
	HOUR hour;

	if ((input=fopen(filename, "r"))==NULL) {
		printf("Error: opening file \"%s\" failed: %s\n", filename, strerror(errno));
		return 0;
	}

	while (fgets(line, sizeof(line), input) != NULL) {
		if (debug) {
			printf("parsing %s", line);
		}

		if (strlen(line)<9) {
			continue;
		}

		sscanf(line, "version;%d", &data.version);
		sscanf(line, "active;%d", &data.active);
		sscanf(line, "interface;%511s", data.interface);
		sscanf(line, "nick;%511s", data.nick);
		if (sscanf(line, "created;%"PRIu64, &tempint)) {
			data.created = (time_t)tempint;
		}
		if (sscanf(line, "updated;%"PRIu64, &tempint)) {
			data.lastupdated = (time_t)tempint;
		}
		sscanf(line, "totalrx;%"PRIu64, &data.totalrx);
		sscanf(line, "totaltx;%"PRIu64, &data.totaltx);
		sscanf(line, "currx;%"PRIu64, &data.currx);
		sscanf(line, "curtx;%"PRIu64, &data.curtx);
		sscanf(line, "totalrxk;%d", &data.totalrxk);
		sscanf(line, "totaltxk;%d", &data.totaltxk);
		sscanf(line, "btime;%"PRIu64, &data.btime);

		count = sscanf(line, "d;%d;%"PRIu64";%"PRIu64";%"PRIu64";%d;%d;%d",
				&i, &tempint, &day.rx, &day.tx, &day.rxk, &day.txk, &day.used);
		if (count == 7) {
			if (i >= 0 && i < (int)sizeof(data.day) / (int)sizeof(DAY)) {
				day.date = (time_t)tempint;
				data.day[i] = day;
			}
		}

		count = sscanf(line, "m;%d;%"PRIu64";%"PRIu64";%"PRIu64";%d;%d;%d",
				&i, &tempint, &month.rx, &month.tx, &month.rxk, &month.txk, &month.used);
		if (count == 7) {
			if ( i >= 0 && i < (int)sizeof(data.month) / (int)sizeof(MONTH) ) {
				month.month = (time_t)tempint;
				data.month[i] = month;
			}
		}

		count = sscanf(line, "t;%d;%"PRIu64";%"PRIu64";%"PRIu64";%d;%d;%d",
				&i, &tempint, &day.rx, &day.tx, &day.rxk, &day.txk, &day.used);
		if (count == 7) {
			if ( i >= 0 && i < (int)sizeof(data.top10) / (int)sizeof(DAY) ) {
				day.date = (time_t)tempint;
				data.top10[i] = day;
			}
		}

		count = sscanf(line, "h;%d;%"PRIu64";%"PRIu64";%"PRIu64,
				&i, &tempint, &hour.rx, &hour.tx);
		if (count == 4) {
			if ( i >= 0 && i < (int)sizeof(data.hour) / (int)sizeof(HOUR) ) {
				hour.date = (time_t)tempint;
				data.hour[i] = hour;
			}
		}
	}

	return 1;
}
