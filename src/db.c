#include "vnstat.h"
#include "db.h"
#include "dbaccess.h"
#include "ifinfo.h"
#include "misc.h"

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
	int dmonth[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
	
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
			
			printf("\t   received: %s", getvalue(data.totalrx, data.totalrxk, 10));
			printf(" (%.1f%%)\n",rx);
			printf("\ttransmitted: %s", getvalue(data.totaltx, data.totaltxk, 10));
			printf(" (%.1f%%)\n",tx);
			printf("\t      total: %s", getvalue(data.totalrx+data.totaltx, data.totalrxk+data.totaltxk, 10));
			printf("\n\n");
			
			printf("\t                rx     |     tx     |  total\n");
			printf("\t-----------------------+------------+-----------\n");
			if (data.day[1].date!=0) {
				printf("\t%9s   %s", daytemp, getvalue(data.day[1].rx, data.day[1].rxk, 7));
				printf(" | %s", getvalue(data.day[1].tx, data.day[1].txk, 7));
				printf(" | %s", getvalue(data.day[1].rx+data.day[1].tx, data.day[1].rxk+data.day[1].txk, 7));
				printf("\n");
			}
			printf("\t    today   %s", getvalue(data.day[0].rx, data.day[0].rxk, 7));
			printf(" | %s", getvalue(data.day[0].tx, data.day[0].txk, 7));
			printf(" | %s", getvalue(data.day[0].rx+data.day[0].tx, data.day[0].rxk+data.day[0].txk, 7));
			printf("\n");
			printf("\t-----------------------+------------+-----------\n");
			printf("\testimated   %s", getvalue(e_rx, 0, -7));
			printf(" | %s", getvalue(e_tx, 0, -7));
			printf(" | %s", getvalue(e_rx+e_tx, 0, -7));
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
					printf(" %8s   %s", datebuff, getvalue(data.day[i].rx, data.day[i].rxk, 7));
					printf("  | %s", getvalue(data.day[i].tx, data.day[i].txk, 7));
					printf("  | %s", getvalue(data.day[i].rx+data.day[i].tx, data.day[i].rxk+data.day[i].txk, 7));
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
				printf(" estimated  %s", getvalue(e_rx, 0, -7));
				printf("  | %s", getvalue(e_tx, 0, -7));
				printf("  | %s", getvalue(e_rx+e_tx, 0, -7));
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
					printf(" %8s   %s", datebuff, getvalue(data.month[i].rx, data.month[i].rxk, 8));
					printf("  | %s", getvalue(data.month[i].tx, data.month[i].txk, 8));
					printf("  | %s", getvalue(data.month[i].rx+data.month[i].tx, data.month[i].rxk+data.month[i].txk, 8));
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
				printf(" estimated  %s", getvalue(e_rx, 0, -8));
				printf("  | %s", getvalue(e_tx, 0, -8));
				printf("  | %s", getvalue(e_rx+e_tx, 0, -8));
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
					printf("  %2d  %10s   %s", i+1, datebuff, getvalue(data.top10[i].rx, data.top10[i].rxk, 7));
					printf("  | %s", getvalue(data.top10[i].tx, data.top10[i].txk, 7));
					printf("  | %s", getvalue(data.top10[i].rx+data.top10[i].tx, data.top10[i].rxk+data.top10[i].txk, 7));
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
				printf("     %9s   %s",daytemp, getvalue(data.day[1].rx, data.day[1].rxk, 7));
				printf("  / %s", getvalue(data.day[1].tx, data.day[1].txk, 7));
				printf("  / %s", getvalue(data.day[1].rx+data.day[1].tx, data.day[1].rxk+data.day[1].txk, 7));
				printf("\n");
			}
			printf("         today   %s", getvalue(data.day[0].rx, data.day[0].rxk, 7));
			printf("  / %s", getvalue(data.day[0].tx, data.day[0].txk, 7));
			printf("  / %s", getvalue(data.day[0].rx+data.day[0].tx, data.day[0].rxk+data.day[0].txk, 7));
			printf("  / %s", getvalue(e_rx+e_tx, 0, -7));
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
				printf("\t  last 7 days %s", getvalue(t_rx, t_rxk, 9));
				printf("  | %s", getvalue(t_tx, t_txk, 9));
				printf("  | %s", getvalue(t_rx+t_tx, t_rxk+t_txk, 9));
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
				printf("\t    last week %s", getvalue(t_rx, t_rxk, 9));
				printf("  | %s", getvalue(t_tx, t_txk, 9));
				printf("  | %s", getvalue(t_rx+t_tx, t_rxk+t_txk, 9));
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
				printf("\t current week %s", getvalue(t_rx, t_rxk, 9));
				printf("  | %s", getvalue(t_tx, t_txk, 9));
				printf("  | %s", getvalue(t_rx+t_tx, t_rxk+t_txk, 9));
				printf("\n");
				temp++;
			}
			
			if (temp==0)
				printf("\t                         no data available\n");
			
			printf("\t----------------------------+---------------+--------------\n");
			if (used!=0) {
				printf("\t    estimated %s", getvalue(e_rx, 0, -9));
				printf("  | %s", getvalue(e_tx, 0, -9));
				printf("  | %s", getvalue(e_rx+e_tx, 0, -9));
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

void synccounters(char iface[32], char dirname[512])
{
	readdb(iface, dirname);
	getifinfo(iface);

	/* set counters to current without counting traffic */
	data.currx = ifinfo.rx;
	data.curtx = ifinfo.tx;
	
	writedb(iface, dirname, 0);
}
