#include "vnstat.h"
#include "dbaccess.h"

int readdb(char iface[32], char dirname[512])
{
	FILE *db;
	char file[512], backup[512];
	int newdb=0, i;

	snprintf(file, 512, "%s/%s", dirname, iface);	
	snprintf(backup, 512, "%s/.%s", dirname, iface);

	if ((db=fopen(file,"r"))!=NULL) {
	
		/* lock file */
		lockdb(fileno(db), 0);

		fread(&data,sizeof(DATA),1,db);
		if (debug)
			printf("Database loaded for interface \"%s\"...\n",data.interface);

		/* convert old database to new format if necessary */
		if (data.version!=DBVERSION) {
			printf("Trying to convert database \"%s\" (v%d) to current db format\n", file, data.version);

			if (!convertdb(db)) {

				/* close current db and try using backup if database conversion failed */
				fclose(db);
				if ((db=fopen(backup,"r"))!=NULL) {
				
					/* lock file */
					lockdb(fileno(db), 0);
				
					fread(&data,sizeof(DATA),1,db);
					if (debug)
						printf("Database loaded for interface \"%s\"...\n",data.interface);

					if (data.version!=DBVERSION) {
						if (!convertdb(db)) {
							printf("Error:\nUnable to use backup database.\n");
							exit(1);
						}
					}
					printf("Database possibly corrupted, using backup instead.\n");
				} else {
					printf("Error:\nUnable to open backup database \"%s\".\n", backup);
					exit(1);
				}
			}
		}

		fclose(db);
			
		if (strcmp(data.interface,iface)) {
			printf("Warning:\nThe previous interface for this file was \"%s\".\n",data.interface);
			printf("It has now been replaced with \"%s\".\n\n",iface);
			printf("You can ignore this message if you renamed the filename.\n");
			if (strcmp(data.interface, data.nick)==0)
				strncpy(data.nick, iface, 32);
			strncpy(data.interface, iface, 32);
		}
	} else {
		printf("Error:\nUnable to read database \"%s\".\n",file);
		newdb=1;
		
		/* set default values for a new database */
		strncpy(data.interface, iface, 32);
		strncpy(data.nick, data.interface, 32);
		data.version=DBVERSION;
		data.active=1;
		data.totalrx=0;
		data.totaltx=0;
		data.currx=0;
		data.curtx=0;
		data.totalrxk=0;
		data.totaltxk=0;
		data.lastupdated=time(NULL);
		data.created=time(NULL);
		
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
		data.day[0].date=data.month[0].month=time(NULL);
		data.btime=FP32;
	}
	return newdb;
}

void writedb(char iface[32], char dirname[512], int newdb)
{
	FILE *db;
	char file[512], backup[512];

	snprintf(file, 512, "%s/%s", dirname, iface);
	snprintf(backup, 512, "%s/.%s", dirname, iface);

	/* try to make backup of old file */
	rename(file, backup);

	/* make sure version stays correct */
	data.version=DBVERSION;

	if ((db=fopen(file,"w"))!=NULL) {

		/* lock file */
		lockdb(fileno(db), 1);

		data.lastupdated=time(NULL);
		fwrite(&data,sizeof(DATA),1,db);
		if (debug)
			printf("Database saved...\n");
		fclose(db);
		if (newdb)
			printf("-> A new database has been created.\n");
	} else {
		printf("Error:\nUnable to write database \"%s\".\n",file);
		printf("Make sure it's write enabled for this user.\n");
		printf("Database not updated.\n");
	}
}


int convertdb(FILE *db)
{
	int i, days, mod;
	DATA10 data10;
	DATA12 data12;
	time_t current;
	struct tm *d;
	int dmonth[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
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
		
		printf("Converting to db v2...\n");
		
		rewind(db);
		fread(&data10, sizeof(DATA10), 1, db);
		
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
				data12.day[i].rx=data12.day[i].tx=0;
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
				days+=dmonth[mod];
				data12.month[i].used=1;
			} else {
				data12.month[i].rx=data12.month[i].tx=0;
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
					printf("Convertion for date \"%s\" failed.\n",data10.top10[i].date);
					printf("Reseting top10...\n");
					
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
				data12.top10[i].rx=data12.top10[i].tx=0;
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
			fread(&data12, sizeof(DATA12), 1, db);
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
		printf("Error:\nUnable to convert corrupted database.\n");
		return 0;
	} else if (data.version!=DBVERSION) {
		printf("Error:\nUnable to convert database version \"%d\".\n", data.version);
		return 0;
	}

	printf("Convertion done.\n");

	return 1;
}

void lockdb(int fd, int dbwrite)
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
		
			if (debug)
				printf("Database access locked (%d, %d)\n", dbwrite, locktry);
		
			/* give up if lock can't be obtained */
			if (locktry>=LOCKTRYLIMIT) {
				if (dbwrite) {
					printf("Error:\nLocking database file for write failed for %d tries:\n%s (%d)\n", locktry, strerror(errno), errno);
				} else {
					printf("Error:\nLocking database file for read failed for %d tries:\n%s (%d)\n", locktry, strerror(errno), errno);
				}	
				exit(1);				
			}
		
			/* someone else has the lock */
			if (errno==EWOULDBLOCK) {
				sleep(1);
			
			/* real error */
			} else {
				if (dbwrite) {
					printf("Error:\nLocking database file for write failed:\n%s (%d)\n", strerror(errno), errno);
				} else {
					printf("Error:\nLocking database file for read failed:\n%s (%d)\n", strerror(errno), errno);
				}
				exit(1);
			}
	
			locktry++;
		}
	}
}
