#include "common.h"
#include "misc.h"
#include "dbshow.h"

void showdb(int qmode)
{
	float rxp, txp;
	int i, used, week, temp;
	struct tm *d;
	char datebuff[16];
	char daytemp[32], daytemp2[32];
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
		
			/* get formated date for yesterday */
			d=localtime(&current);
			strftime(datebuff, 16, cfg.dformat, d);
			
			/* get formated date for latest day in database */
			d=localtime(&data.day[0].date);
			strftime(daytemp2, 16, cfg.dformat, d);
			
			/* change daytemp to today if formated days match */
			if (strcmp(datebuff, daytemp2)==0) {
				strncpy(daytemp2, "today", 32);
			}
			
			if (data.totalrx>1024 || data.totaltx>1024)	{
				rxp = (data.totalrx/(float)(data.totalrx+data.totaltx))*100;
			} else {
				rxp = ( ((data.totalrx*1024)+data.totalrxk) / (float)(((data.totalrx*1024)+data.totalrxk)+((data.totaltx*1024)+data.totaltxk)) )*100;
			}	
			txp = (float)100 - rxp;

			/* use database update time for estimates */
			d=localtime(&data.lastupdated);

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
					printf("        %s\n\n", data.interface);
				else
					printf("        %s [disabled]\n\n", data.interface);
			} else {
				if (data.active)
					printf("        %s (%s)\n\n", data.nick, data.interface);
				else
					printf("        %s (%s) [disabled]\n\n", data.nick, data.interface);
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
			
			/* get formated date for creation date */
			d=localtime(&data.created);
			strftime(datebuff, 16, cfg.tformat, d);
			
			printf("           received: %s", getvalue(data.totalrx, data.totalrxk, 10, 1));
			printf("   (%.1f%%)\n", rxp);
			printf("        transmitted: %s", getvalue(data.totaltx, data.totaltxk, 10, 1));
			printf("   (%.1f%%)\n", txp);
			printf("              total: %s", getvalue(data.totalrx+data.totaltx, data.totalrxk+data.totaltxk, 10, 1));
			printf("   since %s\n", datebuff);
			printf("\n");
			
			printf("                        rx      |     tx      |   total\n");
			printf("        ------------------------+-------------+------------\n");
			if (data.day[1].date!=0) {
				printf("        %9s   %s", daytemp, getvalue(data.day[1].rx, data.day[1].rxk, 7, 1));
				printf(" | %s", getvalue(data.day[1].tx, data.day[1].txk, 7, 1));
				printf(" | %s", getvalue(data.day[1].rx+data.day[1].tx, data.day[1].rxk+data.day[1].txk, 7, -1));
				printf("\n");
			}
			printf("        %9s   %s", daytemp2, getvalue(data.day[0].rx, data.day[0].rxk, 7, 1));
			printf(" | %s", getvalue(data.day[0].tx, data.day[0].txk, 7, 1));
			printf(" | %s", getvalue(data.day[0].rx+data.day[0].tx, data.day[0].rxk+data.day[0].txk, 7, -1));
			printf("\n");
			printf("        ------------------------+-------------+------------\n");
			printf("        estimated   %s", getvalue(e_rx, 0, 7, 2));
			printf(" | %s", getvalue(e_tx, 0, 7, 2));
			printf(" | %s", getvalue(e_rx+e_tx, 0, 7, -2));
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
					printf(" %8s   %s", datebuff, getvalue(data.day[i].rx, data.day[i].rxk, 7, 1));
					printf(" | %s", getvalue(data.day[i].tx, data.day[i].txk, 7, 1));
					printf(" | %s", getvalue(data.day[i].rx+data.day[i].tx, data.day[i].rxk+data.day[i].txk, 7, 1));
					showbar(data.day[i].rx, data.day[i].rxk, data.day[i].tx, data.day[i].txk, max, 25);
					printf("\n");
					used++;
				}
			}
			if (used==0)
				printf("                        no data available\n");
			printf("------------------------+-------------+----------------------------------------\n");
			if (used!=0) {
				/* use database update time for estimates */
				d=localtime(&data.lastupdated);
				if ( data.day[0].rx==0 || data.day[0].tx==0 || (d->tm_hour*60+d->tm_min)==0 ) {
					e_rx=e_tx=0;
				} else {				
					e_rx=((data.day[0].rx)/(float)(d->tm_hour*60+d->tm_min))*1440;
					e_tx=((data.day[0].tx)/(float)(d->tm_hour*60+d->tm_min))*1440;
				}
				printf(" estimated  %s", getvalue(e_rx, 0, 7, 2));
				printf(" | %s", getvalue(e_tx, 0, 7, 2));
				printf(" | %s", getvalue(e_rx+e_tx, 0, 7, 2));
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
					printf(" %8s   %s", datebuff, getvalue(data.month[i].rx, data.month[i].rxk, 8, 1));
					printf(" | %s", getvalue(data.month[i].tx, data.month[i].txk, 8, 1));
					printf(" | %s", getvalue(data.month[i].rx+data.month[i].tx, data.month[i].rxk+data.month[i].txk, 8, 1));
					showbar(data.month[i].rx, data.month[i].rxk, data.month[i].tx, data.month[i].txk, max, 22);
					printf("\n");
					used++;
				}
			}
			if (used==0)
				printf("                        no data available\n");
			printf("-------------------------+--------------+--------------------------------------\n");			
			if (used!=0) {
				/* use database update time for estimates */
				d=localtime(&data.lastupdated);
				if ( data.month[0].rx==0 || data.month[0].tx==0 || ((d->tm_mday-1)*24+d->tm_hour)==0 ) {
					e_rx=e_tx=0;
				} else {
					e_rx=((data.month[0].rx)/(float)((d->tm_mday-1)*24+d->tm_hour))*(dmonth[d->tm_mon]*24);
					e_tx=((data.month[0].tx)/(float)((d->tm_mday-1)*24+d->tm_hour))*(dmonth[d->tm_mon]*24);
				}
				printf(" estimated  %s", getvalue(e_rx, 0, 8, 2));
				printf(" | %s", getvalue(e_tx, 0, 8, 2));
				printf(" | %s", getvalue(e_rx+e_tx, 0, 8, 2));
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
			
			printf("   #       day          rx     |      tx     |  total\n");
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
					printf("  %2d  %10s   %s", i+1, datebuff, getvalue(data.top10[i].rx, data.top10[i].rxk, 7, 1));
					printf(" | %s", getvalue(data.top10[i].tx, data.top10[i].txk, 7, 1));
					printf(" | %s", getvalue(data.top10[i].rx+data.top10[i].tx, data.top10[i].rxk+data.top10[i].txk, 7, 1));
					showbar(data.top10[i].rx, data.top10[i].rxk, data.top10[i].tx, data.top10[i].txk, max, 18);
					printf("\n");
					used++;
				}
			}
			if (used==0)
				printf("                             no data available\n");
			printf("-------------------------------+-------------+---------------------------------\n");
		
		/* dumpdb */
		} else if (qmode==4) {
		
			printf("version;%d\n", data.version);
			printf("active;%d\n", data.active);
			printf("interface;%s\n", data.interface);
			printf("nick;%s\n", data.nick);
			printf("created;%d\n", (int)data.created);
			printf("updated;%d\n", (int)data.lastupdated);
			
			printf("totalrx;%"PRIu64"\n", data.totalrx);
			printf("totaltx;%"PRIu64"\n", data.totaltx);
			printf("currx;%"PRIu64"\n", data.currx);
			printf("curtx;%"PRIu64"\n", data.curtx);
			printf("totalrxk;%d\n", data.totalrxk);
			printf("totaltxk;%d\n", data.totaltxk);
			printf("btime;%"PRIu64"\n", data.btime);
			
			for (i=0;i<=29;i++) {
				printf("d;%d;%d;%"PRIu64";%"PRIu64";%d;%d;%d\n",i, (int)data.day[i].date, data.day[i].rx, data.day[i].tx, data.day[i].rxk, data.day[i].txk, data.day[i].used);
			}
			
			for (i=0;i<=11;i++) {
				printf("m;%d;%d;%"PRIu64";%"PRIu64";%d;%d;%d\n",i, (int)data.month[i].month, data.month[i].rx, data.month[i].tx, data.month[i].rxk, data.month[i].txk, data.month[i].used);
			}
			
			for (i=0;i<=9;i++) {
				printf("t;%d;%d;%"PRIu64";%"PRIu64";%d;%d;%d\n",i,(int)data.top10[i].date, data.top10[i].rx, data.top10[i].tx, data.top10[i].rxk, data.top10[i].txk, data.top10[i].used);
			}
			for (i=0;i<=23;i++) {
				printf("h;%d;%d;%"PRIu64";%"PRIu64"\n",i,(int)data.hour[i].date, data.hour[i].rx, data.hour[i].tx);
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

			/* get formated date for today */
			d=localtime(&current);
			strftime(datebuff, 16, cfg.dformat, d);
			
			/* get formated date for lastest day in database */
			d=localtime(&data.day[0].date);
			strftime(daytemp2, 16, cfg.dformat, d);
			
			/* change daytemp to today if formated days match */
			if (strcmp(datebuff, daytemp2)==0) {
				strncpy(daytemp2, "today", 32);
			}

			/* use database update time for estimates */
			d=localtime(&data.lastupdated);

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
				printf("     %9s   %s",daytemp, getvalue(data.day[1].rx, data.day[1].rxk, 7, 1));
				printf("  / %s", getvalue(data.day[1].tx, data.day[1].txk, 7, 1));
				printf("  / %s", getvalue(data.day[1].rx+data.day[1].tx, data.day[1].rxk+data.day[1].txk, 7, 1));
				printf("\n");
			}
			printf("     %9s   %s", daytemp2, getvalue(data.day[0].rx, data.day[0].rxk, 7, 1));
			printf("  / %s", getvalue(data.day[0].tx, data.day[0].txk, 7, 1));
			printf("  / %s", getvalue(data.day[0].rx+data.day[0].tx, data.day[0].rxk+data.day[0].txk, 7, 1));
			printf("  / %s", getvalue(e_rx+e_tx, 0, 7, 2));
			printf("\n\n");

		/* last 7 */
		} else if (qmode==6) {
		
			printf("\n");
			if (strcmp(data.interface, data.nick)==0) {
				if (data.active)
					printf("        %s  /  weekly\n\n", data.interface);
				else
					printf("        %s [disabled]  /  weekly\n\n", data.interface);
			} else {
				if (data.active)
					printf("        %s (%s)  /  weekly\n\n", data.nick, data.interface);
				else
					printf("        %s (%s) [disabled]  /  weekly\n\n", data.nick, data.interface);
			}	

			printf("                            rx      |     tx      |  total\n");
			printf("         ---------------------------+-------------+------------\n");
			
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
				printf("          last 7 days   %s", getvalue(t_rx, t_rxk, 7, 1));
				printf(" | %s", getvalue(t_tx, t_txk, 7, 1));
				printf(" | %s", getvalue(t_rx+t_tx, t_rxk+t_txk, 7, -1));
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
				printf("            last week   %s", getvalue(t_rx, t_rxk, 7, 1));
				printf(" | %s", getvalue(t_tx, t_txk, 7, 1));
				printf(" | %s", getvalue(t_rx+t_tx, t_rxk+t_txk, 7, -1));
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
				/* use database update time for estimates */
				d=localtime(&data.lastupdated);
				strftime(daytemp, 16, "%u", d);
				if ( t_rx==0 || t_tx==0 || ((atoi(daytemp)-1)*24+d->tm_hour)==0 ) {
					e_rx=e_tx=0;
				} else {
					e_rx=((t_rx)/(float)((atoi(daytemp)-1)*24+d->tm_hour)*168);
					e_tx=((t_tx)/(float)((atoi(daytemp)-1)*24+d->tm_hour)*168);
				}
				printf("         current week   %s", getvalue(t_rx, t_rxk, 7, 1));
				printf(" | %s", getvalue(t_tx, t_txk, 7, 1));
				printf(" | %s", getvalue(t_rx+t_tx, t_rxk+t_txk, 7, -1));
				printf("\n");
				temp++;
			}
			
			if (temp==0)
				printf("                                  no data available\n");
			
			printf("         ---------------------------+-------------+------------\n");
			if (used!=0) {
				printf("            estimated   %s", getvalue(e_rx, 0, 7, 2));
				printf(" | %s", getvalue(e_tx, 0, 7, 2));
				printf(" | %s", getvalue(e_rx+e_tx, 0, 7, -2));
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
	if (cfg.unit==0) {
		sprintf(matrix[14]," h  rx (KiB)   tx (KiB)      h  rx (KiB)   tx (KiB)      h  rx (KiB)   tx (KiB)");
	} else {
		sprintf(matrix[14]," h   rx (KB)    tx (KB)      h   rx (KB)    tx (KB)      h   rx (KB)    tx (KB)");
	}

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
	
	/* numbers under x-axis and graphics :) */
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
#if !defined(__OpenBSD__)
		sprintf(matrix[15+i],"%02d %'10"PRIu64" %'10"PRIu64"    %02d %'10"PRIu64" %'10"PRIu64"    %02d %'10"PRIu64" %'10"PRIu64"",s%24, data.hour[s%24].rx, data.hour[s%24].tx, (s+8)%24, data.hour[(s+8)%24].rx, data.hour[(s+8)%24].tx, (s+16)%24, data.hour[(s+16)%24].rx, data.hour[(s+16)%24].tx);
#else
		sprintf(matrix[15+i],"%02d %10"PRIu64" %10"PRIu64"    %02d %10"PRIu64" %10"PRIu64"    %02d %10"PRIu64" %10"PRIu64"",s%24, data.hour[s%24].rx, data.hour[s%24].tx, (s+8)%24, data.hour[(s+8)%24].rx, data.hour[(s+8)%24].tx, (s+16)%24, data.hour[(s+16)%24].rx, data.hour[(s+16)%24].tx);
#endif
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
		printf("  ");

		if (tx>rx) {
			l=rintf((rx/(float)(rx+tx)*len));

			for (i=0;i<l;i++) {
				printf("%c", cfg.rxchar[0]);
			}
			for (i=0;i<(len-l);i++) {
				printf("%c", cfg.txchar[0]);
			}
		} else {
			l=rintf((tx/(float)(rx+tx)*len));
			
			for (i=0;i<(len-l);i++) {
				printf("%c", cfg.rxchar[0]);
			}
			for (i=0;i<l;i++) {
				printf("%c", cfg.txchar[0]);
			}		
		}
		
	}
	
}
