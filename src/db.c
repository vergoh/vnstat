#include "vnstat.h"
#include "db.h"
#include "proc.h"

int dmonth[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

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
							exit(0);
						}
					}
					printf("Database possibly corrupted, replacing with backup.\n");
				} else {
					printf("Error:\nUnable to open backup database \"%s\".\n", backup);
					exit(0);
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
	uint64_t e_rx, e_tx, t_rx, t_tx, max;
	int t_rxk, t_txk;
	time_t current, yesterday;
	
	current=time(NULL);
	yesterday=current-86400;
	
	e_rx=e_tx=t_rx=t_tx=t_rxk=t_txk=0;
					
	if (data.totalrx+data.totaltx==0 && data.totalrxk+data.totaltxk==0) {
		
		printf(" %s: Not enough data available yet.\n", data.interface);
		
	} else {
		
		/* total with today and possible yesterday */
		if (qmode==0) {
		
			d=localtime(&current);
			
			rx=(data.totalrx/((float)data.totalrx+data.totaltx))*100;
			tx=(data.totaltx/((float)data.totalrx+data.totaltx))*100;

			if ( data.day[0].rx==0 || data.day[0].tx==0 || (d->tm_hour*60+d->tm_min)==0 ) {
				e_rx=e_tx=0;
			} else {	
				e_rx=((data.day[0].rx)/(float)(d->tm_hour*60+d->tm_min))*1440;
				e_tx=((data.day[0].tx)/(float)(d->tm_hour*60+d->tm_min))*1440;
			}
			
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
			strftime(datebuff, 16, cfg.dformat, d);
			
			/* get formated date for previous day in database */
			d=localtime(&data.day[1].date);
			strftime(daytemp, 16, cfg.dformat, d);
			
			/* change daytemp to yesterday if formated days match */
			if (strcmp(datebuff, daytemp)==0) {
				strncpy(daytemp, "yesterday", 32);
			}
			
			printf("\t   received: ");
			showint(data.totalrx, data.totalrxk, 10);
			printf(" (%.1f%%)\n",rx);
			printf("\ttransmitted: ");
			showint(data.totaltx, data.totaltxk, 10);
			printf(" (%.1f%%)\n",tx);
			printf("\t      total: ");
			showint(data.totalrx+data.totaltx, data.totalrxk+data.totaltxk, 10);
			printf("\n\n");
			
			printf("\t                rx     |     tx     |  total\n");
			printf("\t-----------------------+------------+-----------\n");
			if (data.day[1].date!=0) {
				printf("\t%9s   ",daytemp);
				showint(data.day[1].rx, data.day[1].rxk, 7);
				printf(" | ");
				showint(data.day[1].tx, data.day[1].txk, 7);
				printf(" | ");
				showint(data.day[1].rx+data.day[1].tx, data.day[1].rxk+data.day[1].txk, 7);
				printf("\n");
			}
			printf("\t    today   ");
			showint(data.day[0].rx, data.day[0].rxk, 7);
			printf(" | ");
			showint(data.day[0].tx, data.day[0].txk, 7);
			printf(" | ");
			showint(data.day[0].rx+data.day[0].tx, data.day[0].rxk+data.day[0].txk, 7);
			printf("\n");
			printf("\t-----------------------+------------+-----------\n");
			printf("\testimated   ");
			showint(e_rx, 0, -7);
			printf(" | ");
			showint(e_tx, 0, -7);
			printf(" | ");
			showint(e_rx+e_tx, 0, -7);
			printf("\n");

		/* days */
		} else if (qmode==1) {

			printf("\n");
			if (strcmp(data.interface, data.nick)==0) {
				if (data.active)
					printf(" %s  /  daily\n\n", data.interface);
				else
					printf(" %s [disabled]  /  daily\n\n", data.interface);
			} else {
				if (data.active)
					printf(" %s (%s)  /  daily\n\n", data.nick, data.interface);
				else
					printf(" %s (%s) [disabled]  /  daily\n\n", data.nick, data.interface);
			}
		
			printf("    day         rx      |     tx      |  total\n");
			printf("------------------------+-------------+----------------------------------------\n");
			
			/* search maximum */
			max=0;
			for (i=29;i>=0;i--) {
				if (data.day[i].used) {
				
					t_rx=data.day[i].rx+data.day[i].tx;
					t_rxk=data.day[i].rxk+data.day[i].txk;
					
					if (t_rxk>=1024) {
						t_rx+=t_rxk/1024;
						t_rxk-=(t_rxk/1024)*1024;
					}
					
					t_rx=(t_rx*1024)+t_rxk;
					
					if (t_rx>max) {
						max=t_rx;
					}
				}
			}
			
			used=0;
			for (i=29;i>=0;i--) {
				if (data.day[i].used) {
					d=localtime(&data.day[i].date);
					strftime(datebuff, 16, cfg.dformat, d);
					printf(" %8s   ",datebuff);
					showint(data.day[i].rx, data.day[i].rxk, 7);
					printf("  | ");
					showint(data.day[i].tx, data.day[i].txk, 7);
					printf("  | ");
					showint(data.day[i].rx+data.day[i].tx, data.day[i].rxk+data.day[i].txk, 7);
					showbar(data.day[i].rx, data.day[i].rxk, data.day[i].tx, data.day[i].txk, max, 25);
					printf("\n");
					used++;
				}
			}
			if (used==0)
				printf("                       no data available\n");
			printf("------------------------+-------------+----------------------------------------\n");
			if (used!=0) {
				d=localtime(&current);
				if ( data.day[0].rx==0 || data.day[0].tx==0 || (d->tm_hour*60+d->tm_min)==0 ) {
					e_rx=e_tx=0;
				} else {				
					e_rx=((data.day[0].rx)/(float)(d->tm_hour*60+d->tm_min))*1440;
					e_tx=((data.day[0].tx)/(float)(d->tm_hour*60+d->tm_min))*1440;
				}
				printf(" estimated  ");
				showint(e_rx, 0, -7);
				printf("  | ");
				showint(e_tx, 0, -7);
				printf("  | ");
				showint(e_rx+e_tx, 0, -7);
				printf("\n");
			}
			
		/* months */
		} else if (qmode==2) {	

			printf("\n");
			if (strcmp(data.interface, data.nick)==0) {
				if (data.active)
					printf(" %s  /  monthly\n\n", data.interface);
				else
					printf(" %s [disabled]  /  monthly\n\n", data.interface);
			} else {
				if (data.active)
					printf(" %s (%s)  /  monthly\n\n", data.nick, data.interface);
				else
					printf(" %s (%s) [disabled]  /  monthly\n\n", data.nick, data.interface);
			}
			
			printf("   month         rx      |      tx      |   total\n");
			printf("-------------------------+--------------+--------------------------------------\n");

			/* search maximum */
			max=0;
			for (i=11;i>=0;i--) {
				if (data.month[i].used) {
				
					t_rx=data.month[i].rx+data.month[i].tx;
					t_rxk=data.month[i].rxk+data.month[i].txk;

					if (t_rxk>=1024) {
						t_rx+=t_rxk/1024;
						t_rxk-=(t_rxk/1024)*1024;
					}					
					
					t_rx=(t_rx*1024)+t_rxk;
					
					if (t_rx>max) {
						max=t_rx;
					}
				}
			}
			
			used=0;
			for (i=11;i>=0;i--) {
				if (data.month[i].used) {
					d=localtime(&data.month[i].month);
					strftime(datebuff, 16, cfg.mformat, d);
					printf(" %8s   ",datebuff);
					showint(data.month[i].rx, data.month[i].rxk, 8);
					printf("  | ");
					showint(data.month[i].tx, data.month[i].txk, 8);
					printf("  | ");
					showint(data.month[i].rx+data.month[i].tx, data.month[i].rxk+data.month[i].txk, 8);
					showbar(data.month[i].rx, data.month[i].rxk, data.month[i].tx, data.month[i].txk, max, 22);
					printf("\n");
					used++;
				}
			}
			if (used==0)
				printf("                        no data available\n");
			printf("-------------------------+--------------+--------------------------------------\n");			
			if (used!=0) {
				d=localtime(&current);
				if ( data.month[0].rx==0 || data.month[0].tx==0 || ((d->tm_mday-1)*24+d->tm_hour)==0 ) {
					e_rx=e_tx=0;
				} else {
					e_rx=((data.month[0].rx)/(float)((d->tm_mday-1)*24+d->tm_hour))*(dmonth[d->tm_mon]*24);
					e_tx=((data.month[0].tx)/(float)((d->tm_mday-1)*24+d->tm_hour))*(dmonth[d->tm_mon]*24);
				}
				printf("estimated   ");
				showint(e_rx, 0, -8);
				printf("  | ");
				showint(e_tx, 0, -8);
				printf("  | ");
				showint(e_rx+e_tx, 0, -8);
				printf("\n");
			}
		
		/* top10 */
		} else if (qmode==3) {

			printf("\n");
			if (strcmp(data.interface, data.nick)==0) {
				if (data.active)
					printf(" %s  /  top 10\n\n", data.interface);
				else
					printf(" %s [disabled]  /  top 10\n\n", data.interface);
			} else {
				if (data.active)
					printf(" %s (%s)  /  top 10\n\n", data.nick, data.interface);
				else
					printf(" %s (%s) [disabled]  /  top 10\n\n", data.nick, data.interface);
			}
			
			printf("   #       day         rx      |     tx      |  total\n");
			printf("-------------------------------+-------------+---------------------------------\n");

			/* search maximum */
			max=0;
			for (i=0;i<=9;i++) {
				if (data.top10[i].used) {
				
					t_rx=data.top10[i].rx+data.top10[i].tx;
					t_rxk=data.top10[i].rxk+data.top10[i].txk;

					if (t_rxk>=1024) {
						t_rx+=t_rxk/1024;
						t_rxk-=(t_rxk/1024)*1024;
					}						
					
					t_rx=(t_rx*1024)+t_rxk;
					
					if (t_rx>max) {
						max=t_rx;
					}
				}
			}

			used=0;
			for (i=0;i<=9;i++) {
				if (data.top10[i].used) {
					d=localtime(&data.top10[i].date);
					strftime(datebuff, 16, cfg.tformat, d);
					printf("  %2d  %10s   ", i+1, datebuff);
					showint(data.top10[i].rx, data.top10[i].rxk, 7);
					printf("  | ");
					showint(data.top10[i].tx, data.top10[i].txk, 7);
					printf("  | ");
					showint(data.top10[i].rx+data.top10[i].tx, data.top10[i].rxk+data.top10[i].txk, 7);
					showbar(data.top10[i].rx, data.top10[i].rxk, data.top10[i].tx, data.top10[i].txk, max, 18);
					printf("\n");
					used++;
				}
			}
			if (used==0)
				printf("                              no data available\n");
			printf("-------------------------------+-------------+---------------------------------\n");
		
		/* dumpdb */
		} else if (qmode==4) {
		
			printf("version;%d\n", data.version);
			printf("active;%d\n", data.active);
			printf("interface;%s\n", data.interface);
			printf("nick;%s\n", data.nick);
			printf("created;%d\n", (int)data.created);
			printf("updated;%d\n", (int)data.lastupdated);
			
			printf("totalrx;%Lu\n", data.totalrx);
			printf("totaltx;%Lu\n", data.totaltx);
			printf("currx;%Lu\n", data.currx);
			printf("curtx;%Lu\n", data.curtx);
			printf("totalrxk;%d\n", data.totalrxk);
			printf("totaltxk;%d\n", data.totaltxk);
			printf("btime;%Lu\n", data.btime);
			
			for (i=0;i<=29;i++) {
				printf("d;%d;%d;%Lu;%Lu;%d;%d;%d\n",i, (int)data.day[i].date, data.day[i].rx, data.day[i].tx, data.day[i].rxk, data.day[i].txk, data.day[i].used);
			}
			
			for (i=0;i<=11;i++) {
				printf("m;%d;%d;%Lu;%Lu;%d;%d;%d\n",i, (int)data.month[i].month, data.month[i].rx, data.month[i].tx, data.month[i].rxk, data.month[i].txk, data.month[i].used);
			}
			
			for (i=0;i<=9;i++) {
				printf("t;%d;%d;%Lu;%Lu;%d;%d;%d\n",i,(int)data.top10[i].date, data.top10[i].rx, data.top10[i].tx, data.top10[i].rxk, data.top10[i].txk, data.top10[i].used);
			}
			for (i=0;i<=23;i++) {
				printf("h;%d;%d;%Lu;%Lu\n",i,(int)data.hour[i].date, data.hour[i].rx, data.hour[i].tx);
			}			
		
		/* multiple dbs in one print */
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
			if ( data.day[0].rx==0 || data.day[0].tx==0 || (d->tm_hour*60+d->tm_min)==0 ) {
				e_rx=e_tx=0;
			} else {
				e_rx=((data.day[0].rx)/(float)(d->tm_hour*60+d->tm_min))*1440;
				e_tx=((data.day[0].tx)/(float)(d->tm_hour*60+d->tm_min))*1440;
			}

			/* get formated date for yesterday */
			d=localtime(&yesterday);
			strftime(datebuff, 16, cfg.dformat, d);
			
			/* get formated date for previous day in database */
			d=localtime(&data.day[1].date);
			strftime(daytemp, 16, cfg.dformat, d);
			
			/* change daytemp to yesterday if formated days match */
			if (strcmp(datebuff, daytemp)==0) {
				strncpy(daytemp, "yesterday", 32);
			}
			
			if (data.day[1].date!=0) {
				printf("     %9s   ",daytemp);
				showint(data.day[1].rx, data.day[1].rxk, 7);
				printf("  / ");
				showint(data.day[1].tx, data.day[1].txk, 7);
				printf("  / ");
				showint(data.day[1].rx+data.day[1].tx, data.day[1].rxk+data.day[1].txk, 7);
				printf("\n");
			}
			printf("         today   ");
			showint(data.day[0].rx, data.day[0].rxk, 7);
			printf("  / ");
			showint(data.day[0].tx, data.day[0].txk, 7);
			printf("  / ");
			showint(data.day[0].rx+data.day[0].tx, data.day[0].rxk+data.day[0].txk, 7);
			printf("  / ");
			showint(e_rx+e_tx, 0, -7);
			printf("\n\n");

		/* last 7 */
		} else if (qmode==6) {
		
			printf("\n");
			if (strcmp(data.interface, data.nick)==0) {
				if (data.active)
					printf("\t%s  /  weekly\n\n", data.interface);
				else
					printf("\t%s [disabled]  /  weekly\n\n", data.interface);
			} else {
				if (data.active)
					printf("\t%s (%s)  /  weekly\n\n", data.nick, data.interface);
				else
					printf("\t%s (%s) [disabled]  /  weekly\n\n", data.nick, data.interface);
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
			t_rx=t_tx=t_rxk=t_txk=0;
			for (i=0;i<30;i++) {
				if ((data.day[i].used) && (data.day[i].date>=current-604800)) {
					addtraffic(&t_rx, &t_rxk, data.day[i].rx, data.day[i].rxk);
					addtraffic(&t_tx, &t_txk, data.day[i].tx, data.day[i].txk);
					used++;
				}
			}

			if (used!=0) {
				printf("\t  last 7 days ");
				showint(t_rx, t_rxk, 9);
				printf("  | ");
				showint(t_tx, t_txk, 9);
				printf("  | ");
				showint(t_rx+t_tx, t_rxk+t_txk, 9);
				printf("\n");
				temp++;
			}

			/* traffic for previous week */
			used=0;
			t_rx=t_tx=t_rxk=t_txk=0;
			for (i=0;i<30;i++) {
				if (data.day[i].used) {
					d=localtime(&data.day[i].date);
					strftime(daytemp, 16, "%V", d);
					if (atoi(daytemp)==week-1) {
						addtraffic(&t_rx, &t_rxk, data.day[i].rx, data.day[i].rxk);
						addtraffic(&t_tx, &t_txk, data.day[i].tx, data.day[i].txk);
						used++;
					}
				}
			}
			
			if (used!=0) {
				printf("\t    last week ");
				showint(t_rx, t_rxk, 9);
				printf("  | ");
				showint(t_tx, t_txk, 9);
				printf("  | ");
				showint(t_rx+t_tx, t_rxk+t_txk, 9);
				printf("\n");
				temp++;
			}

			/* this week */
			used=0;
			t_rx=t_tx=t_rxk=t_txk=0;
			for (i=0;i<30;i++) {
				if (data.day[i].used) {
					d=localtime(&data.day[i].date);
					strftime(daytemp, 16, "%V", d);
					if (atoi(daytemp)==week) {
						addtraffic(&t_rx, &t_rxk, data.day[i].rx, data.day[i].rxk);
						addtraffic(&t_tx, &t_txk, data.day[i].tx, data.day[i].txk);
						used++;
					}
				}
			}

			/* get estimate for current week */
			if (used!=0) {
				d=localtime(&current);
				strftime(daytemp, 16, "%u", d);
				if ( t_rx==0 || t_tx==0 || ((atoi(daytemp)-1)*24+d->tm_hour)==0 ) {
					e_rx=e_tx=0;
				} else {
					e_rx=((t_rx)/(float)((atoi(daytemp)-1)*24+d->tm_hour)*168);
					e_tx=((t_tx)/(float)((atoi(daytemp)-1)*24+d->tm_hour)*168);
				}
				printf("\t current week ");
				showint(t_rx, t_rxk, 9);
				printf("  | ");
				showint(t_tx, t_txk, 9);
				printf("  | ");
				showint(t_rx+t_tx, t_rxk+t_txk, 9);
				printf("\n");
				temp++;
			}
			
			if (temp==0)
				printf("\t                         no data available\n");
			
			printf("\t----------------------------+---------------+--------------\n");
			if (used!=0) {
				printf("\t    estimated ");
				showint(e_rx, 0, -9);
				printf("  | ");
				showint(e_tx, 0, -9);
				printf("  | ");
				showint(e_rx+e_tx, 0, -9);
				printf("\n");
			}

		/* hours */
		} else if (qmode==7) {
		
			showhours();

		} else {
			/* users shouldn't see this text... */
			printf("Not such query mode: %d\n", qmode);
		}

	}

}

void showhours(void)
{
	int i, j, k;
	int tmax=0, max=0, s=0, dots=0;
	char matrix[24][80];
	
	int hour, min;
	struct tm *d;
	
	/* tmax = time max = current hour */
	/* max = transfer max */
	
	d=localtime(&data.lastupdated);
	hour=d->tm_hour;
	min=d->tm_min;
	
	for (i=0;i<=23;i++) {
		if (data.hour[i].date>=data.hour[tmax].date) {
			tmax=i;
		}
		if (data.hour[i].rx>=max) {
			max=data.hour[i].rx;
		}
		if (data.hour[i].tx>=max) {
			max=data.hour[i].tx;
		}
	}
	
	/* mr. proper */
	for (i=0;i<24;i++) {
		for (j=0;j<80;j++) {
			matrix[i][j]=' ';
		}
	}
	
	
	/* structure */
	sprintf(matrix[11]," -+--------------------------------------------------------------------------->");
	sprintf(matrix[14]," h   rx (kB)    tx (kB)      h   rx (kB)    tx (kB)      h   rx (kB)    tx (kB)");

	for (i=10;i>1;i--)
		matrix[i][2]='|';
		
	matrix[1][2]='^';
	matrix[12][2]='|';
	
	/* title */
	if (strcmp(data.interface, data.nick)==0) {
		if (data.active)
			sprintf(matrix[0]," %s", data.interface);
		else
			sprintf(matrix[0]," %s [disabled]", data.interface);
	} else {
		if (data.active)
			sprintf(matrix[0]," %s (%s)", data.nick, data.interface);
		else
			sprintf(matrix[0]," %s (%s) [disabled]", data.nick, data.interface);
	}
	
	/* time to the corner */
	sprintf(matrix[0]+74,"%02d:%02d", hour, min);
	
	/* numbers under x-axel and graphics :) */
	k=5;
	for (i=23;i>=0;i--) {
		s=tmax-i;
		if (s<0)
			s+=24;
			
		sprintf(matrix[12]+k,"%02d ",s);

		dots=10*(data.hour[s].rx/(float)max);
		for (j=0;j<dots;j++)
			matrix[10-j][k]=cfg.rxhourchar[0];

		dots=10*(data.hour[s].tx/(float)max);
		for (j=0;j<dots;j++)
			matrix[10-j][k+1]=cfg.txhourchar[0];

		k=k+3;
	}
	
	/* hours and traffic */
	for (i=0;i<=7;i++) {
		s=tmax+i+1;
		sprintf(matrix[15+i],"%02d %'10Lu %'10Lu    %02d %'10Lu %'10Lu    %02d %'10Lu %'10Lu",s%24, data.hour[s%24].rx, data.hour[s%24].tx, (s+8)%24, data.hour[(s+8)%24].rx, data.hour[(s+8)%24].tx, (s+16)%24, data.hour[(s+16)%24].rx, data.hour[(s+16)%24].tx);
	}
	
	/* clean \0 */
	for (i=0;i<23;i++) {
		for (j=0;j<80;j++) {
			if (matrix[i][j]=='\0') {
				matrix[i][j]=' ';
			}
		}
	} 	
	
	/* show matrix (yes, the last line isn't shown) */
	for (i=0;i<23;i++) {
		for (j=0;j<80;j++) {
			printf("%c",matrix[i][j]);
		}
		printf("\n");
	} 	

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
				printf("Hour %d (%d) cleaned.\n",i, (int)data.hour[i].date);
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
				printf("Current hour %d (%d) cleaned.\n",hour, (int)data.hour[i].date);
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
		printf("Days rotated. Current date: %d.%d.%d\n",d->tm_mday, d->tm_mon+1, d->tm_year+1900);
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
		printf("Months rotated. Current month: \"%d\".\n",d->tm_mon+1);
	}
}

int convertdb(FILE *db)
{
	int i, days, mod;
	DATA10 data10;
	DATA12 data12;
	time_t current;
	struct tm *d;
	int month=0, day;
	int tm_mday, tm_mon, tm_year;	int converted=0;
	
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


void showint(uint64_t mb, uint64_t kb, int len)
{
	uint64_t kB;
	int declen=2;

	/* don't show decimals .00 if len was negative (set that way for estimates) */
	if (len<0) {
		declen=0;
		len=abs(len);
	}

	if (mb!=0) {
		if (kb>=1024) {
			mb+=kb/1024;
			kb-=(kb/1024)*1024;
		}
		kB=(mb*1024)+kb;
	} else {
		kB=kb;
	}


	if ( (declen==0) && (kB==0) ){
		printf("%*s   ", len, "--");
	} else {
		/* try to figure out what unit to use */
		if (kB>=(1024*1024*1000)) {
			printf("%'*.2f TB", len, kB/(float)1024/(float)1024/(float)1024);
		} else if (kB>=(1024*1000)) {
			printf("%'*.2f GB", len, kB/(float)1024/(float)1024);
		} else if (kB>=1000) {
			printf("%'*.*f MB", len, declen, kB/(float)1024);
		} else {
			printf("%'*Lu kB", len, kB);
		}
	}
}

void showbar(uint64_t rx, int rxk, uint64_t tx, int txk, uint64_t max, int len)
{
	int i, l;
	
	if (rxk>=1024) {
		rx+=rxk/1024;
		rxk-=(rxk/1024)*1024;
	}

	if (txk>=1024) {
		tx+=txk/1024;
		txk-=(txk/1024)*1024;
	}

	rx=(rx*1024)+rxk;
	tx=(tx*1024)+txk;
	
	if ((rx+tx)!=max) {
		len=((rx+tx)/(float)max)*len;
	}


	if (len!=0) {
		printf("   ");

		if (tx>rx) {
			l=rint((rx/(float)(rx+tx)*len));

			for (i=0;i<l;i++) {
				printf("%c", cfg.rxchar[0]);
			}
			for (i=0;i<(len-l);i++) {
				printf("%c", cfg.txchar[0]);
			}
		} else {
			l=rint((tx/(float)(rx+tx)*len));
			
			for (i=0;i<(len-l);i++) {
				printf("%c", cfg.rxchar[0]);
			}
			for (i=0;i<l;i++) {
				printf("%c", cfg.txchar[0]);
			}		
		}
		
	}
	
}

void synccounters(char iface[32], char dirname[512])
{
	char temp[64], temp2[64];
	char *proclineptr;

	readdb(iface, dirname);
	readproc(iface);

	/* get rx and tx from procline */
	proclineptr = strchr(procline, ':');
	sscanf(proclineptr+1, "%s %*s %*s %*s %*s %*s %*s %*s %s",temp, temp2);

	/* set counters to current without counting traffic */
	data.currx=strtoull(temp, (char **)NULL, 0);
	data.curtx=strtoull(temp2, (char **)NULL, 0);
	
	writedb(iface, dirname, 0);
}

void lockdb(int fd, int dbwrite)
{
	int locktry=1;

	/* lock only if configured to do so */
	if (cfg.flock) {

		/* try locking file */
		while (flock(fd, LOCK_EX|LOCK_NB)!=0) {
		
			if (debug)
				printf("Database access locked (%d, %d)\n", dbwrite, locktry);
		
			/* give up if lock can't be obtained */
			if (locktry>=LOCKTRYLIMIT) {
				if (dbwrite) {
					printf("Error:\nLocking database file for write failed for %d tries:\n%s (%d)\n", locktry, strerror(errno), errno);
				} else {
					printf("Error:\nLocking database file for read failed for %d tries:\n%s (%d)\n", locktry, strerror(errno), errno);
				}	
				exit(0);				
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
				exit(0);
			}
	
			locktry++;
		}
	}
}
