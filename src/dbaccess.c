#include "common.h"
#include "dbsql.h"
#include "fs.h"
#include "dbaccess.h"

int importlegacydb(const char *iface, const char *dirname)
{
	DATA data;

	snprintf(errorstring, 1024, "Importing data from legacy database \"%s\".", iface);
	printe(PT_Infoless);

	if (db_getinterfacecountbyname(iface)) {
		return 0;
	}

	if (readdb(&data, iface, dirname, 0) != 0) {
		return 0;
	}

	if (!db_addinterface(iface)) {
		return 0;
	}

	if (!insertlegacydata(&data, iface)) {
		return 0;
	}

	return 1;
}

int insertlegacydata(DATA *data, const char *iface)
{
	int i, year;
	time_t yeartime;
	struct tm *stm;
	uint64_t rx, tx;

	if (!db_begintransaction()) {
		return 0;
	}

	if (!db_setactive(iface, data->active)) {
		db_rollbacktransaction();
		return 0;
	}
	if (strcmp(iface, data->nick) != 0) {
		if (!db_setalias(iface, data->nick)) {
			db_rollbacktransaction();
			return 0;
		}
	}
	if (!db_setcounters(iface, data->currx, data->curtx)) {
		db_rollbacktransaction();
		return 0;
	}
	if (!db_setcreation(iface, data->created)) {
		db_rollbacktransaction();
		return 0;
	}

	for (i = 23; i >= 0; i--) {
		if (data->hour[i].date > 0 && ( data->hour[i].rx > 0 || data->hour[i].tx > 0 )) {
			if (!db_insertdata("hour", iface, data->hour[i].rx*1024, data->hour[i].tx*1024, (uint64_t)data->hour[i].date)) {
				db_rollbacktransaction();
				return 0;
			}
		}
	}
	for (i = 29; i >= 0; i--) {
		if (data->day[i].used) {
			if (!db_insertdata("day", iface, data->day[i].rx*1024*1024+data->day[i].rxk*1024, data->day[i].tx*1024*1024+data->day[i].txk*1024, (uint64_t)data->day[i].date)) {
				db_rollbacktransaction();
				return 0;
			}
		}
	}
	for (i = 11; i >= 0; i--) {
		if (data->month[i].used) {
			if (!db_insertdata("month", iface, data->month[i].rx*1024*1024+data->month[i].rxk*1024, data->month[i].tx*1024*1024+data->month[i].txk*1024, (uint64_t)data->month[i].month)) {
				db_rollbacktransaction();
				return 0;
			}
		}
	}
	for (i = 9; i >= 0; i--) {
		if (data->top10[i].used) {
			if (!db_insertdata("top", iface, data->top10[i].rx*1024*1024+data->top10[i].rxk*1024, data->top10[i].tx*1024*1024+data->top10[i].txk*1024, (uint64_t)data->top10[i].date)) {
				db_rollbacktransaction();
				return 0;
			}
		}
	}

	/* construct yearly data from legacy monthly data */
	rx = 0;
	tx = 0;
	year = 0;
	yeartime = 0;
	for (i = 11; i >= 0; i--) {
		if (!data->month[i].used) {
			continue;
		}
		stm = localtime(&data->month[i].month);
		/* sanity check for possible invalid data */
		if (stm->tm_year+1900 <= 1980 || stm->tm_year+1900 >= 2050) {
			continue;
		}
		if (stm->tm_year+1900 != year) {
			if (year != 0 && ( rx > 0 || tx > 0 )) {
				if (!db_insertdata("year", iface, rx, tx, (uint64_t)yeartime)) {
					db_rollbacktransaction();
					return 0;
				}
			}
			year = stm->tm_year + 1900;
			yeartime = data->month[i].month;
			rx = 0;
			tx = 0;
		}
		rx += data->month[i].rx*1024*1024+data->month[i].rxk*1024;
		tx += data->month[i].tx*1024*1024+data->month[i].txk*1024;
	}
	if (year != 0 && ( rx > 0 || tx > 0 )) {
		if (!db_insertdata("year", iface, rx, tx, (uint64_t)yeartime)) {
			db_rollbacktransaction();
			return 0;
		}
	}

	if (!db_settotal(iface, data->totalrx*1024*1024+data->totalrxk*1024, data->totaltx*1024*1024+data->totaltxk*1024)) {
		db_rollbacktransaction();
		return 0;
	}

	return db_committransaction();
}

int readdb(DATA *data, const char *iface, const char *dirname, const int force)
{
	FILE *db;
	char file[512], backup[512];

	snprintf(file, 512, "%s/%s", dirname, iface);
	snprintf(backup, 512, "%s/.%s", dirname, iface);

	if ((db=fopen(file,"r"))==NULL) {
		snprintf(errorstring, 1024, "Unable to read database \"%s\": %s", file, strerror(errno));
		printe(PT_Error);

		/* create new database template */
		initdb(data);
		strncpy_nt(data->interface, iface, 32);
		strncpy_nt(data->nick, data->interface, 32);
		return 1;
	}

	/* lock file */
	if (!lockdb(fileno(db), 0)) {
		fclose(db);
		return -1;
	}

	if (fread(data,sizeof(DATA),1,db)!=1 || ferror(db)) {
		data->version=-1;
		if (debug) {
			printf("db: Database read failed for file \"%s\".\n", file);
		}
	} else {
		if (debug) {
			data->interface[sizeof(data->interface) - 1] = '\0';
			printf("db: Database loaded for interface \"%s\"...\n", data->interface);
		}
	}
	data->interface[sizeof(data->interface) - 1] = '\0';
	data->nick[sizeof(data->nick) - 1] = '\0';

	if (data->version == LEGACYDBVERSION) {
		if (!validatedb(data) && !force) {
			data->version=-1;
			if (debug) {
				printf("db: Database for interface \"%s\" fails to validate, trying with backup\n", data->interface);
			}
		}
	}

	/* convert old database to new format if necessary */
	if (data->version<LEGACYDBVERSION) {
		if (data->version==-1) {

			/* close current db and try using backup if database conversion failed */
			fclose(db);
			if ((db=fopen(backup,"r"))==NULL) {
				snprintf(errorstring, 1024, "Unable to open backup database \"%s\": %s", backup, strerror(errno));
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

			if (fread(data,sizeof(DATA),1,db)!=1 || ferror(db)) {
				snprintf(errorstring, 1024, "Database load failed even when using backup (%s). Aborting.", strerror(errno));
				printe(PT_Error);
				fclose(db);

				if (noexit) {
					return -1;
				} else {
					exit(EXIT_FAILURE);
				}
			} else {
				if (debug) {
					data->interface[sizeof(data->interface) - 1] = '\0';
					printf("db: Backup database loaded for interface \"%s\"...\n", data->interface);
				}
			}
			data->interface[sizeof(data->interface) - 1] = '\0';
			data->nick[sizeof(data->nick) - 1] = '\0';

			if (data->version == LEGACYDBVERSION) {
				if (!validatedb(data)) {
					data->version=-1;
					if (debug) {
						printf("db: Backup database for interface \"%s\" fails to validate\n", data->interface);
					}
				}
			}

			if (data->version!=LEGACYDBVERSION) {
				if (data->version==-1) {
					snprintf(errorstring, 1024, "Unable to use database \"%s\" or backup database \"%s\".", file, backup);
					printe(PT_Error);
					fclose(db);

					if (noexit) {
						return -1;
					} else {
						exit(EXIT_FAILURE);
					}
				}
			}
			snprintf(errorstring, 1024, "Database possibly corrupted, using backup instead.");
			printe(PT_Info);
		}

	} else if (data->version>LEGACYDBVERSION) {
		snprintf(errorstring, 1024, "Downgrading database \"%s\" (v%d) is not supported.", file, data->version);
		printe(PT_Error);
		fclose(db);

		if (noexit) {
			return -1;
		} else {
			exit(EXIT_FAILURE);
		}
	}

	fclose(db);

	if (strcmp(data->interface,iface)) {
		snprintf(errorstring, 1024, "Warning:\nThe previous interface for this file was \"%s\".",data->interface);
		printe(PT_Multiline);
		snprintf(errorstring, 1024, "It has now been replaced with \"%s\".",iface);
		printe(PT_Multiline);
		snprintf(errorstring, 1024, "You can ignore this message if you renamed the filename.");
		printe(PT_Multiline);
		snprintf(errorstring, 1024, "Interface name mismatch, renamed \"%s\" -> \"%s\"", data->interface, iface);
		printe(PT_ShortMultiline);
		if (strcmp(data->interface, data->nick)==0) {
			strncpy_nt(data->nick, iface, 32);
		}
		strncpy_nt(data->interface, iface, 32);
	}

	return 0;
}

void initdb(DATA *data)
{
	int i;
	time_t current;
	struct tm *d;

	current=time(NULL);
	d=localtime(&current);

	/* set default values for a new database */
	data->version=LEGACYDBVERSION;
	data->active=1;
	data->totalrx=0;
	data->totaltx=0;
	data->currx=0;
	data->curtx=0;
	data->totalrxk=0;
	data->totaltxk=0;
	data->lastupdated=current;
	data->created=current;

	/* days */
	for (i=0;i<=29;i++) {
		data->day[i].rx=0;
		data->day[i].tx=0;
		data->day[i].rxk=0;
		data->day[i].txk=0;
		data->day[i].date=0;
		data->day[i].used=0;
	}

	/* months */
	for (i=0;i<=11;i++) {
		data->month[i].rx=0;
		data->month[i].tx=0;
		data->month[i].rxk=0;
		data->month[i].txk=0;
		data->month[i].month=0;
		data->month[i].used=0;
	}

	/* top10 */
	for (i=0;i<=9;i++) {
		data->top10[i].rx=0;
		data->top10[i].tx=0;
		data->top10[i].rxk=0;
		data->top10[i].txk=0;
		data->top10[i].date=0;
		data->top10[i].used=0;
	}

	/* hours */
	for (i=0;i<=23;i++) {
		data->hour[i].rx=0;
		data->hour[i].tx=0;
		data->hour[i].date=0;
	}

	data->day[0].used=data->month[0].used=1;
	data->day[0].date=current;

	/* calculate new date for current month if current day is less
           than the set monthrotate value so that new databases begin
           from the right month */
	if (d->tm_mday < cfg.monthrotate) {
		d->tm_mday=cfg.monthrotate;
		d->tm_mon--;
		data->month[0].month=mktime(d);
	} else {
		data->month[0].month=current;
	}

	data->btime=MAX32;
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
				snprintf(errorstring, 1024, "Locking database file for write failed for %d tries: %s", locktry, strerror(errno));
			} else {
				snprintf(errorstring, 1024, "Locking database file for read failed for %d tries: %s", locktry, strerror(errno));
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
				snprintf(errorstring, 1024, "Locking database file for write failed: %s", strerror(errno));
			} else {
				snprintf(errorstring, 1024, "Locking database file for read failed: %s", strerror(errno));
			}
			printe(PT_Error);
			return 0;
		}

		locktry++;
	}

	return 1;
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

int validatedb(DATA *data)
{
	int i, used;
	uint64_t rxsum, txsum;

	if (debug) {
		printf("validating loaded database\n");
	}

	/* enforce string termination */
	data->interface[sizeof(data->interface)-1] = '\0';
	data->nick[sizeof(data->nick)-1] = '\0';

	if (data->version>LEGACYDBVERSION) {
		snprintf(errorstring, 1024, "%s: Invalid database version: %d", data->interface, data->version);
		printe(PT_Error);
		return 0;
	}

	if (data->active<0 || data->active>1) {
		snprintf(errorstring, 1024, "%s: Invalid database activity status: %d", data->interface, data->active);
		printe(PT_Error);
		return 0;
	}

	if (!strlen(data->interface)) {
		snprintf(errorstring, 1024, "Invalid database interface string: %s", data->interface);
		printe(PT_Error);
		return 0;
	}

	if (!data->created || !data->lastupdated || !data->btime) {
		snprintf(errorstring, 1024, "%s: Invalid database timestamp.", data->interface);
		printe(PT_Error);
		return 0;
	}

	rxsum = txsum = 0;
	used = 1;
	for (i=0; i<30; i++) {
		if (data->day[i].used<0 || data->day[i].used>1) {
			snprintf(errorstring, 1024, "%s: Invalid database daily use information: %d %d", data->interface, i, data->day[i].used);
			printe(PT_Error);
			return 0;
		}
		if (data->day[i].rxk<0 || data->day[i].txk<0) {
			snprintf(errorstring, 1024, "%s: Invalid database daily traffic: %d", data->interface, i);
			printe(PT_Error);
			return 0;
		}
		if (data->day[i].used && !used) {
			snprintf(errorstring, 1024, "%s: Invalid database daily use order: %d", data->interface, i);
			printe(PT_Error);
			return 0;
		} else if (!data->day[i].used) {
			used = 0;
		}
		if (data->day[i].used) {
			rxsum += data->day[i].rx;
			txsum += data->day[i].tx;
		}
	}

	for (i=1; i<30; i++) {
		if (!data->day[i].used) {
			break;
		}
		if (data->day[i-1].date < data->day[i].date) {
			snprintf(errorstring, 1024, "%s: Invalid database daily date order: %u (%d) < %u (%d)", data->interface, (unsigned int)data->day[i-1].date, i-1, (unsigned int)data->day[i].date, i);
			printe(PT_Error);
			return 0;
		}
	}

	if (data->totalrx < rxsum || data->totaltx < txsum) {
		snprintf(errorstring, 1024, "%s: Invalid database total traffic compared to daily usage.", data->interface);
		printe(PT_Error);
		return 0;
	}

	rxsum = txsum = 0;
	used = 1;
	for (i=0; i<12; i++) {
		if (data->month[i].used<0 || data->month[i].used>1) {
			snprintf(errorstring, 1024, "%s: Invalid database monthly use information: %d %d", data->interface, i, data->month[i].used);
			printe(PT_Error);
			return 0;
		}
		if (data->month[i].rxk<0 || data->month[i].txk<0) {
			snprintf(errorstring, 1024, "%s: Invalid database monthly traffic: %d", data->interface, i);
			printe(PT_Error);
			return 0;
		}
		if (data->month[i].used && !used) {
			snprintf(errorstring, 1024, "%s: Invalid database monthly use order: %d", data->interface, i);
			printe(PT_Error);
			return 0;
		} else if (!data->month[i].used) {
			used = 0;
		}
		if (data->month[i].used) {
			rxsum += data->month[i].rx;
			txsum += data->month[i].tx;
		}
	}

	for (i=1; i<12; i++) {
		if (!data->month[i].used) {
			break;
		}
		if (data->month[i-1].month < data->month[i].month) {
			snprintf(errorstring, 1024, "%s: Invalid database monthly date order: %u (%d) < %u (%d)", data->interface, (unsigned int)data->month[i-1].month, i-1, (unsigned int)data->month[i].month, i);
			printe(PT_Error);
			return 0;
		}
	}

	if (data->totalrx < rxsum || data->totaltx < txsum) {
		snprintf(errorstring, 1024, "%s: Invalid database total traffic compared to monthly usage.", data->interface);
		printe(PT_Error);
		return 0;
	}

	used = 1;
	for (i=0; i<10; i++) {
		if (data->top10[i].used<0 || data->top10[i].used>1) {
			snprintf(errorstring, 1024, "%s: Invalid database top10 use information: %d %d", data->interface, i, data->top10[i].used);
			printe(PT_Error);
			return 0;
		}
		if (data->top10[i].rxk<0 || data->top10[i].txk<0) {
			snprintf(errorstring, 1024, "%s: Invalid database top10 traffic: %d", data->interface, i);
			printe(PT_Error);
			return 0;
		}
		if (data->top10[i].used && !used) {
			snprintf(errorstring, 1024, "%s: Invalid database top10 use order: %d", data->interface, i);
			printe(PT_Error);
			return 0;
		} else if (!data->top10[i].used) {
			used = 0;
		}
	}

	return 1;
}
