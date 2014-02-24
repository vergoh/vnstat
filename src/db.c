#include "vnstat.h"
#include "db.h"

int dmonth[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

int readdb(char iface[32], char file[512])
{
	FILE *db;
	int newdb=0, i;
	
	if ((db=fopen(file,"r"))!=NULL) {
		fread(&data,sizeof(DATA),1,db);
		if (debug)
			printf("Database loaded for interface \"%s\"...\n",data.interface);

		/* convert old database to new format if necessary */
		if (data.version!=DBVERSION)
			convertdb(db);
			
		fclose(db);
		if (strcmp(data.interface,iface)) {
			printf("Warning:\nThe previous interface for this file was \"%s\".\n",data.interface);
			printf("It has now been replaced with \"%s\".\n\n",iface);
			printf("You can ignore this message if you renamed the filename.\n");
			if (strcmp(data.interface, data.nick)==0)
				strcpy(data.nick, iface);
			strcpy(data.interface, iface);
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
		for (i=0;i<=29;i++) {
			data.day[i].rx=0;
			data.day[i].tx=0;
			data.day[i].date=0;
			data.day[i].used=0;
		}
		for (i=0;i<=11;i++) {
			data.month[i].rx=0;
			data.month[i].tx=0;
			data.month[i].month=0;
			data.month[i].used=0;
		}
		for (i=0;i<=9;i++) {
			data.top10[i].rx=0;
			data.top10[i].tx=0;
			data.top10[i].date=0;
			data.top10[i].used=0;
		}
		data.day[0].used=data.month[0].used=1;
		data.day[0].date=data.month[0].month=time(NULL);
		data.btime=FPOINT-1;
	}
	return newdb;
}

void writedb(char file[512], int newdb)
{
	FILE *db;

	if ((db=fopen(file,"w"))!=NULL) {
		data.lastupdated=time(NULL);
		fwrite(&data,sizeof(DATA),1,db);
		if (debug)
			printf("Database saved...\n");
		fclose(db);
		if (newdb)
			printf("New database generated.\n");
	} else {
		printf("Error:\nUnable to write database \"%s\".\n",file);
		printf("Make sure it's write enabled for this user.\n");
		printf("Database not updated.\n");
	}
}

void showdb(int qmode)
{
	float rx, tx;
	int i, used, week, temp;
	struct tm *d;
	char datebuff[16];
	char daytemp[128];
	uint64_t e_rx, e_tx, t_rx, t_tx;
	time_t current, yesterday;
	
	current=time(NULL);
	yesterday=current-86400;
					
	if (data.totalrx+data.totaltx==0) {
		
		printf(" %s: Not enough data available yet.\n", data.interface);
		
	} else {
		
		if (qmode==0) {
		
			d=localtime(&current);
			
			rx=(data.totalrx/((float)data.totalrx+data.totaltx))*100;
			tx=(data.totaltx/((float)data.totalrx+data.totaltx))*100;

			e_rx=((data.day[0].rx)/(float)(d->tm_hour*60+d->tm_min))*1440;
			e_tx=((data.day[0].tx)/(float)(d->tm_hour*60+d->tm_min))*1440;
			
			/* printf("\nDatabase created: %s",(char*)asctime(localtime(&data.created))); */
		
			printf("Database updated: %s\n",(char*)asctime(localtime(&data.lastupdated)));
			
			if (strcmp(data.interface, data.nick)==0) {
				if (data.active)
					printf("\t%s\n\n", data.interface);
				else
					printf("\t%s [disabled]\n\n", data.interface);
			} else {
				if (data.active)
					printf("\t%s (%s)\n\n", data.nick, data.interface);
				else
					printf("\t%s (%s) [disabled]\n\n", data.nick, data.interface);
			}
			
			/* get formated date for yesterday */
			d=localtime(&yesterday);
			strftime(datebuff, 16, DFORMAT, d);
			
			/* get formated date for previous day in database */
			d=localtime(&data.day[1].date);
			strftime(daytemp, 16, DFORMAT, d);
			
			/* change daytemp to yesterday if formated days match */
			if (strcmp(datebuff, daytemp)==0) {
				strcpy(daytemp, "yesterday");
			}
			
			printf("\t   received: %'14Lu MB (%.1f%%)\n",data.totalrx,rx);
			printf("\ttransmitted: %'14Lu MB (%.1f%%)\n",data.totaltx,tx);
			printf("\t      total: %'14Lu MB\n\n",data.totalrx+data.totaltx);
			
			printf("\t                rx     |     tx     |  total\n");
			printf("\t-----------------------+------------+-----------\n");
			printf("\t%9s   %'7Lu MB | %'7Lu MB | %'7Lu MB\n",daytemp,data.day[1].rx,data.day[1].tx,data.day[1].rx+data.day[1].tx);
			printf("\t    today   %'7Lu MB | %'7Lu MB | %'7Lu MB\n",data.day[0].rx,data.day[0].tx,data.day[0].rx+data.day[0].tx);
			printf("\t-----------------------+------------+-----------\n");
			printf("\testimated   %'7Lu MB | %'7Lu MB | %'7Lu MB\n",e_rx, e_tx, e_rx+e_tx);

		} else if (qmode==1) {

			printf("\n");
			if (strcmp(data.interface, data.nick)==0) {
				if (data.active)
					printf("\t%s\n\n", data.interface);
				else
					printf("\t%s [disabled]\n\n", data.interface);
			} else {
				if (data.active)
					printf("\t%s (%s)\n\n", data.nick, data.interface);
				else
					printf("\t%s (%s) [disabled]\n\n", data.nick, data.interface);
			}
		
			printf("\t    day         rx      |     tx      |  total\n");
			printf("\t------------------------+-------------+--------------\n");
			
			used=0;
			for (i=29;i>=0;i--) {
				if (data.day[i].used) {
					d=localtime(&data.day[i].date);
					strftime(datebuff, 16, DFORMAT, d);
					printf("\t   %6s   %'7Lu MB  | %'7Lu MB  | %'7Lu MB\n",datebuff, data.day[i].rx,data.day[i].tx,data.day[i].rx+data.day[i].tx);
					used++;
				}
			}
			if (used==0)
				printf("\t                 no data available\n");
			printf("\t------------------------+-------------+--------------\n");
			if (used!=0) {
				d=localtime(&current);
				e_rx=((data.day[0].rx)/(float)(d->tm_hour*60+d->tm_min))*1440;
				e_tx=((data.day[0].tx)/(float)(d->tm_hour*60+d->tm_min))*1440;
				printf("\t estimated  %'7Lu MB  | %'7Lu MB  | %'7Lu MB\n", e_rx, e_tx, e_rx+e_tx);			
			}
			
		} else if (qmode==2) {	

			printf("\n");
			if (strcmp(data.interface, data.nick)==0) {
				if (data.active)
					printf("\t%s\n\n", data.interface);
				else
					printf("\t%s [disabled]\n\n", data.interface);
			} else {
				if (data.active)
					printf("\t%s (%s)\n\n", data.nick, data.interface);
				else
					printf("\t%s (%s) [disabled]\n\n", data.nick, data.interface);
			}
			
			printf("\t   month        rx      |       tx      |    total\n");
			printf("\t------------------------+---------------+---------------\n");
			
			used=0;
			for (i=11;i>=0;i--) {
				if (data.month[i].used) {
					d=localtime(&data.month[i].month);
					strftime(datebuff, 16, MFORMAT, d);
					printf("\t  %7s %'9Lu MB  | %'9Lu MB  | %'9Lu MB\n",datebuff, data.month[i].rx,data.month[i].tx,data.month[i].rx+data.month[i].tx);
					used++;
				}
			}
			if (used==0)
				printf("\t                 no data available\n");
			printf("\t------------------------+---------------+---------------\n");			
			if (used!=0) {
				d=localtime(&current);
				e_rx=((data.month[0].rx)/(float)((d->tm_mday-1)*24+d->tm_hour))*(dmonth[d->tm_mon]*24);
				e_tx=((data.month[0].tx)/(float)((d->tm_mday-1)*24+d->tm_hour))*(dmonth[d->tm_mon]*24);
				printf("\testimated %'9Lu MB  | %'9Lu MB  | %'9Lu MB\n", e_rx, e_tx, e_rx+e_tx);
			}
			
		} else if (qmode==3) {

			printf("\n");
			if (strcmp(data.interface, data.nick)==0) {
				if (data.active)
					printf("\t%s\n\n", data.interface);
				else
					printf("\t%s [disabled]\n\n", data.interface);
			} else {
				if (data.active)
					printf("\t%s (%s)\n\n", data.nick, data.interface);
				else
					printf("\t%s (%s) [disabled]\n\n", data.nick, data.interface);
			}
			
			printf("\t   #       day          rx      |     tx      |  total\n");
			printf("\t--------------------------------+-------------+-------------\n");

			used=0;
			for (i=0;i<=9;i++) {
				if (data.top10[i].used) {
					d=localtime(&data.top10[i].date);
					strftime(datebuff, 16, TFORMAT, d);
					printf("\t  %2d    %8s    %'7Lu MB  | %'7Lu MB  | %'7Lu MB\n", i+1, datebuff, data.top10[i].rx,data.top10[i].tx,data.top10[i].rx+data.top10[i].tx);
					used++;
				}
			}
			if (used==0)
				printf("\t                    no data available\n");
			printf("\t--------------------------------+-------------+-------------\n");
		
		} else if (qmode==4) {
		
			printf("version;%d\n", data.version);
			printf("active;%d\n", data.active);
			printf("interface;%s\n", data.interface);
			printf("nick;%s\n", data.nick);
			printf("created;%d\n", data.created);
			printf("updated;%d\n", data.lastupdated);
			
			printf("totalrx;%Lu\n", data.totalrx);
			printf("totaltx;%Lu\n", data.totaltx);
			printf("currx;%Lu\n", data.currx);
			printf("curtx;%Lu\n", data.curtx);
			printf("totalrxk;%d\n", data.totalrxk);
			printf("totaltxk;%d\n", data.totaltxk);
			printf("btime;%Lu\n", data.btime);
			
			for (i=0;i<=29;i++) {
				printf("d;%d;%d;%Lu;%Lu;%d\n",i, data.day[i].date, data.day[i].rx, data.day[i].tx, data.day[i].used);
			}
			
			for (i=0;i<=11;i++) {
				printf("m;%d;%d;%Lu;%Lu;%d\n",i, data.month[i].month, data.month[i].rx, data.month[i].tx, data.month[i].used);
			}
			
			for (i=0;i<=9;i++) {
				printf("t;%d;%d;%Lu;%Lu;%d\n",i,data.top10[i].date, data.top10[i].rx, data.top10[i].tx, data.top10[i].used);
			}
		} else if (qmode==5) {
		
			if (strcmp(data.interface, data.nick)==0) {
				if (data.active)
					printf(" %s:\n", data.interface);
				else
					printf(" %s [disabled]:\n", data.interface);
			} else {
				if (data.active)
					printf(" %s (%s):\n", data.nick, data.interface);
				else
					printf(" %s (%s) [disabled]:\n", data.nick, data.interface);
			}
			/* time needed for estimates */
			d=localtime(&current);
			e_rx=((data.day[0].rx)/(float)(d->tm_hour*60+d->tm_min))*1440;
			e_tx=((data.day[0].tx)/(float)(d->tm_hour*60+d->tm_min))*1440;

			/* get formated date for yesterday */
			d=localtime(&yesterday);
			strftime(datebuff, 16, DFORMAT, d);
			
			/* get formated date for previous day in database */
			d=localtime(&data.day[1].date);
			strftime(daytemp, 16, DFORMAT, d);
			
			/* change daytemp to yesterday if formated days match */
			if (strcmp(datebuff, daytemp)==0) {
				strcpy(daytemp, "yesterday");
			}
						
			printf("     %9s   %'7Lu MB  / %'7Lu MB  / %'7Lu MB\n",daytemp,data.day[1].rx, data.day[1].tx, data.day[1].rx+data.day[1].tx);
			printf("         today   %'7Lu MB  / %'7Lu MB  / %'7Lu MB  / %'7Lu MB\n\n",data.day[0].rx, data.day[0].tx, data.day[0].rx+data.day[0].tx, e_rx+e_tx);

		} else if (qmode==6) {
		
			printf("\n");
			if (strcmp(data.interface, data.nick)==0) {
				if (data.active)
					printf("\t%s\n\n", data.interface);
				else
					printf("\t%s [disabled]\n\n", data.interface);
			} else {
				if (data.active)
					printf("\t%s (%s)\n\n", data.nick, data.interface);
				else
					printf("\t%s (%s) [disabled]\n\n", data.nick, data.interface);
			}	

			printf("\t                    rx      |       tx      |    total\n");
			printf("\t----------------------------+---------------+--------------\n");
			
			/* get current week number */
			d=localtime(&current);
			strftime(daytemp, 16, "%V", d);
			week=atoi(daytemp);
			
			/* last 7 days */
			used=0;
			temp=0;
			t_rx=t_tx=0;
			for (i=0;i<30;i++) {
				if ((data.day[i].used) && (data.day[i].date>=current-604800)) {
					t_rx+=data.day[i].rx;
					t_tx+=data.day[i].tx;
					used++;
				}
			}

			if (used!=0) {
				printf("\t  last 7 days %'9Lu MB  | %'9Lu MB  | %'9Lu MB\n", t_rx, t_tx, t_rx+t_tx);
				temp++;
			}

			/* traffic for previous week */
			used=0;
			t_rx=t_tx=0;
			for (i=0;i<30;i++) {
				if (data.day[i].used) {
					d=localtime(&data.day[i].date);
					strftime(daytemp, 16, "%V", d);
					if (atoi(daytemp)==week-1) {
						t_rx+=data.day[i].rx;
						t_tx+=data.day[i].tx;
						used++;
					}
				}
			}
			
			if (used!=0) {
				printf("\t    last week %'9Lu MB  | %'9Lu MB  | %'9Lu MB\n", t_rx, t_tx, t_rx+t_tx);
				temp++;
			}

			/* this week */
			used=0;
			t_rx=t_tx=0;
			for (i=0;i<30;i++) {
				if (data.day[i].used) {
					d=localtime(&data.day[i].date);
					strftime(daytemp, 16, "%V", d);
					if (atoi(daytemp)==week) {
						t_rx+=data.day[i].rx;
						t_tx+=data.day[i].tx;
						used++;
					}
				}
			}

			/* get estimate for current week */
			if (used!=0) {
				d=localtime(&current);
				strftime(daytemp, 16, "%u", d);
				e_rx=((t_rx)/(float)((atoi(daytemp)-1)*24+d->tm_hour)*168);
				e_tx=((t_tx)/(float)((atoi(daytemp)-1)*24+d->tm_hour)*168);
				printf("\t current week %'9Lu MB  | %'9Lu MB  | %'9Lu MB\n", t_rx, t_tx, t_rx+t_tx);
				temp++;
			}
			
			if (temp==0)
				printf("\t                         no data available\n");
			
			printf("\t----------------------------+---------------+--------------\n");
			if (used!=0)
				printf("\t    estimated %'9Lu MB  | %'9Lu MB  | %'9Lu MB\n", e_rx, e_tx, e_rx+e_tx);

		} else {
			/* users shouldn't see this text... */
			printf("Not such query mode: %d\n", qmode);
		}

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
		data.day[i].date=data.day[i-1].date;
		data.day[i].used=data.day[i-1].used;
	}
	
	current=time(NULL);
	
	data.day[0].rx=0;
	data.day[0].tx=0;
	data.day[0].date=current;
	
	if (debug) {
		d=localtime(&data.day[0].date);
		printf("Days rotated. Current date: %d.%d.%d\n",d->tm_mday, d->tm_mon+1, d->tm_year+1900);
	}

	/* top10 update */
	for (i=0;i<=9;i++) {
		if ( data.day[1].rx+data.day[1].tx > data.top10[i].rx+data.top10[i].tx ) {
			for (j=9;j>=i+1;j--) {
				data.top10[j].rx=data.top10[j-1].rx;
				data.top10[j].tx=data.top10[j-1].tx;
				data.top10[j].date=data.top10[j-1].date;
				data.top10[j].used=data.top10[j-1].used;
			}
			data.top10[i].rx=data.day[1].rx;
			data.top10[i].tx=data.day[1].tx;
			data.top10[i].date=data.day[1].date;
			data.top10[i].used=data.day[1].used;
			break;
		}
	}
	
	if (debug)
		printf("Top10 updated.\n");

}

void rotatemonths(void)
{
	int i;
	time_t current;
	struct tm *d;
	
	for (i=11;i>=1;i--) {
		data.month[i].rx=data.month[i-1].rx;
		data.month[i].tx=data.month[i-1].tx;
		data.month[i].month=data.month[i-1].month;
		data.month[i].used=data.month[i-1].used;
	}
	
	current=time(NULL);
	
	data.month[0].rx=0;
	data.month[0].tx=0;
	data.month[0].month=current;
	
	if (debug) {
		d=localtime(&data.month[0].month);
		printf("Months rotated. Current month: \"%d\".\n",d->tm_mon+1);
	}
}

void convertdb(FILE *db)
{
	int i, days, mod;
	DATA10 data10;
	time_t current;
	struct tm *d;
	int month=0, day;
	int tm_mday, tm_mon, tm_year;
	
	printf("Converting database to new format\n");
	
	current=time(NULL);
	d=localtime(&current);
	days=d->tm_mday-1;
	
	tm_mday=d->tm_mday;
	tm_mon=d->tm_mon;
	tm_year=d->tm_year;
	
	/* version 1.0 database format */
	if (data.version==1) {
		
		rewind(db);
		fread(&data10, sizeof(DATA10), 1, db);
		
		/* set basic values */
		data.version=2;
		strcpy(data.interface, data10.interface);
		strcpy(data.nick, data10.interface);
		data.active=1;
		data.totalrx=data10.totalrx;
		data.totaltx=data10.totaltx;
		data.currx=data10.currx;
		data.curtx=data10.curtx;
		data.totalrxk=data10.totalrxk;
		data.totaltxk=data10.totaltxk;
		data.lastupdated=data10.lastupdated;
		data.created=data10.created;
		data.btime=data10.btime;
		
		/* days */
		for (i=0; i<=29; i++) {
			if (data10.day[i].rx+data10.day[i].tx>0) {
				data.day[i].rx=data10.day[i].rx;
				data.day[i].tx=data10.day[i].tx;
				data.day[i].date=current-(i*86400);
				data.day[i].used=1;
			} else {
				data.day[i].rx=data.day[i].tx=0;
				data.day[i].used=0;
			}			
		}
		
		/* months */
		for (i=0; i<=11; i++) {
			if (data10.month[i].rx+data10.month[i].tx>0) {
				data.month[i].rx=data10.month[i].rx;
				data.month[i].tx=data10.month[i].tx;

				data.month[i].month=current-(86400*days);
				
				/* we have to do this modulo thing to get the number of days right */
				mod=(d->tm_mon-i-1)%12;
				if (mod<0)
					mod=12+mod;
				days+=dmonth[mod];
				data.month[i].used=1;
			} else {
				data.month[i].rx=data.month[i].tx=0;
				data.month[i].used=0;
			}			
		}		
		
		/* top10 */
		for (i=0; i<=9; i++) {
			if (data10.top10[i].rx+data10.top10[i].tx>0) {
				data.top10[i].rx=data10.top10[i].rx;
				data.top10[i].tx=data10.top10[i].tx;				
			
				day=atoi(data10.top10[i].date+7);
			
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
					printf("Conversion for date \"%s\" failed.\n",data10.top10[i].date);
					printf("Reseting top10...\n");
					
				}

				if (month==-1)
					break;

				d->tm_year=tm_year;
				if ((month>tm_mon) || ((month==tm_mon) && (day>tm_mday)))
					d->tm_year--;

				d->tm_mday=day;
				d->tm_mon=month;
				data.top10[i].date=mktime(d);
				data.top10[i].used=1;

			} else {
				data.top10[i].used=0;
				data.top10[i].rx=data.top10[i].tx=0;
			}
			
			
		
		}
		
		/* reset top10 if there was some problem */
		if (month==-1) {
			for (i=0; i<=9; i++) {
				data.top10[i].rx=data.top10[i].tx=0;
				data.top10[i].used=0;
			}
		}
	
	/* unknown version */	
	} else {
		printf("Error:\nUnknown database version \"%d\".\n", data.version);
		exit(0);
	}
}
