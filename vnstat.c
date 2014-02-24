/*
vnStat - Copyright (c) 2002 Teemu Toivola <tst@iki.fi>

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; version 2 dated June, 1991.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program;  if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave., Cambridge, MA 02139, USA.
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <locale.h>
#include <sys/time.h>
#include <string.h>

#define MULTIUSERFILE "/var/spool/vnstat/db"

#define FPOINT 4294967296

typedef struct {
	char date[11];
	uint64_t rx, tx;
} DAY;

typedef struct {
	char month[4];
	uint64_t rx, tx;
} MONTH;

typedef struct {
	int version;
	char interface[32];
	uint64_t totalrx, totaltx, currx, curtx;
	int totalrxk, totaltxk;
	time_t lastupdated, created;
	DAY day[30];
	MONTH month[12];
	DAY top10[10];
	uint64_t btime;
} DATA;

void readproc(char iface[32]);
void parseproc(int newdb);
int readdb(char iface[32], char file[512]);
void writedb(char file[512], int newdb);
void changeinterface(char file[512], char newiface[32]);
void showdb(int qmode);
void rotatedays(void);
void rotatemonths(void);

DATA data;
char procline[512], statline[128];
int debug=0;

int main(int argc, char *argv[]) {

	int currentarg, update=0, query=0, newdb=0, qmode=0, opencheck=0, changeiface=0;
	char interface[32], filename[512];
	FILE *db;

	setlocale(LC_ALL, "en_US");
	strcpy(interface,"default");

	if (MULTIUSER) {
		strcpy(filename,MULTIUSERFILE);
		if (debug)
			printf("Multiuser compile\n");
	} else {
		strcpy(filename,getenv("HOME"));
		strcat(filename,"/.vnstatdb");
		if (debug)
			printf("Singleuser compile\n");
	}

	if ((db=fopen(filename,"r"))!=NULL) {
		if (debug)
			printf("Database found\n");
		fclose(db);
		opencheck=1;
		query=1;
	}

	for (currentarg=1; currentarg<argc; currentarg++) {
		if (debug)
			printf("arg %d: \"%s\"\n",currentarg,argv[currentarg]);
		if ((strcmp(argv[currentarg],"-h")==0) || (strcmp(argv[currentarg],"--help"))==0) {
		
			printf(" vnStat 1.0 by Teemu Toivola <tst@iki.fi>\n\n");
			printf("\t -h, --help\t help\n");
			printf("\t -i, --iface\t interface (default: eth0)\n");
			printf("\t -q, --query\t query database\n");
			printf("\t -u, --update\t update database\n");
			printf("\t -d, --days\t show days\n");
			printf("\t -m, --months\t show months\n");
			printf("\t -t, --top10\t show top10\n");
			printf("\t -D, --debug\t show some additional debug information\n");
			printf("\t -v, --version\t show version\n");
			if (!MULTIUSER)
				printf("\t -f, --file\t select database (default: $HOME/.vnstatdb)\n");
			
			exit(0);
		} else if ((strcmp(argv[currentarg],"-i")==0) || (strcmp(argv[currentarg],"--iface"))==0) {
			if (currentarg+1<argc) {
				strncpy(interface,argv[currentarg+1],32);
				if (debug)
					printf("Used interface: %s\n",interface);
				currentarg++;
				continue;
			} else {
				printf("Interface for -i missing\n");
				exit(1);
			}
		} else if ((strcmp(argv[currentarg],"-u")==0) || (strcmp(argv[currentarg],"--update"))==0) {
			update=1;
			query=0;
			if (debug)
				printf("Updating database...\n");
		} else if ((strcmp(argv[currentarg],"-q")==0) || (strcmp(argv[currentarg],"--query"))==0) {
			query=1;
		} else if ((strcmp(argv[currentarg],"-D")==0) || (strcmp(argv[currentarg],"--debug"))==0) {
			debug=1;
			printf("arg %d: \"%s\"\n",currentarg,argv[currentarg]);
		} else if ((strcmp(argv[currentarg],"-d")==0) || (strcmp(argv[currentarg],"--days"))==0) {
			qmode=1;
		} else if ((strcmp(argv[currentarg],"-m")==0) || (strcmp(argv[currentarg],"--months"))==0) {
			qmode=2;
		} else if ((strcmp(argv[currentarg],"-t")==0) || (strcmp(argv[currentarg],"--top10"))==0) {
			qmode=3;
		} else if ((strcmp(argv[currentarg],"-v")==0) || (strcmp(argv[currentarg],"--version"))==0) {
			printf("vnStat 1.0 by Teemu Toivola <tst@iki.fi>\n");
			exit(0);
		} else if ((strcmp(argv[currentarg],"-f")==0) || (strcmp(argv[currentarg],"--file"))==0 && !MULTIUSER) {
			if (currentarg+1<argc) {
				strncpy(filename,argv[currentarg+1],512);
				currentarg++;
				continue;
			} else {
				printf("File for -f missing\n");
				exit(1);
			}
		} else if (strcmp(argv[currentarg],"--override-interface")==0) {
			if (currentarg+1<argc) {
				strncpy(interface,argv[currentarg+1],32);
				if (debug)
					printf("Used interface: \"%s\"\n",interface);
				currentarg++;
				changeiface=1;
				continue;
			} else {
				printf("Interface for --override-interface missing\n");
				exit(1);
			}
		} else {
			printf("Unknown arg \"%s\".\n",argv[currentarg]);
			exit(1);
		}
		
	}

	if (changeiface) {
		changeinterface(filename, interface);
	}
	if (update) {
		newdb=readdb(interface, filename);
		readproc(data.interface);
		parseproc(newdb);
		writedb(filename, newdb);
	}
	if (query) {
		readdb(interface, filename);
		if (!newdb)
			showdb(qmode);
	}
	
	if (!query && !update && !changeiface)
		printf("Nothing to do. Use -h for help\n");
	
	return 0;
}

void readproc(char iface[32])
{
	FILE *fp;
	char temp[64], inface[32];
	int check;
	
	if ((fp=fopen("/proc/net/dev","r"))==NULL) {
		printf("Error:\nUnable to read /proc/net/dev.\n");
		exit(1);
	}

	if (strcmp(iface,"default")==0) {
		strcpy(inface,"eth0");
	} else {
		strcpy(inface,iface);
	}

	check=0;
	while (fgets(procline,512,fp)!=NULL) {
		sscanf(procline,"%s",temp);
		if (strncmp(inface,temp,strlen(inface))==0) {
			if (debug)
				printf("\n%s\n",procline);
			check=1;
			break;
		}
	}
	fclose(fp);
	
	if (check==0) {
		printf("Requested interface \"%s\" not found.\n",inface);
		exit(1);
	}

	if ((fp=fopen("/proc/stat","r"))==NULL) {
		printf("Error:\nUnable to read /proc/net/dev.\n");
		exit(1);
	}
	
	check=0;
	while (fgets(statline,128,fp)!=NULL) {
		sscanf(statline,"%s",temp);
		if (strcmp(temp,"btime")==0) {
			if (debug)
				printf("\n%s\n",statline);
			check=1;
			break;
		}
	}
	
	fclose(fp);
	
	if (check==0) {
		printf("Error:\nbtime missing from /proc/stat.\n");
		exit(1);
	}
}

void parseproc(int newdb)
{
	char temp[64], cdate[8];
	uint64_t rx, tx, rxchange=0, txchange=0, btime;
	time_t current;

	btime=strtoul(statline+6, (char **)NULL, 0);

	/* btime in /proc/stat seems to vary ±1 second so we use btime-5 just to be safe */
	if (data.btime<btime-5) {
		data.currx=0;
		data.curtx=0;
		if (debug)
			printf("System has been booted.\n");
	}

	data.btime=btime;

	current=time(NULL);
	strncpy(cdate,(char*)asctime(localtime(&current)),8);	
	
	rx=strtoul(procline+7, (char **)NULL, 0);

	if (newdb!=1) {
		if (data.currx<=rx) {
			rxchange=(rx-data.currx)/1024/1024;
			data.totalrxk+=((rx-data.currx)/1024)%1024;
			if (debug)
				printf("rx: %Lu - %Lu = %Lu\nrxk: %d\n",rx,data.currx,rx-data.currx,data.totalrxk);
		} else {
			rxchange=(FPOINT-data.currx+rx)/1024/1024;
			data.totalrxk+=((FPOINT-data.currx+rx)/1024)%1024;
			if (debug)
				printf("rx: %Lu - %Lu + %Lu = %Lu\nrxk: %d\n",FPOINT,data.currx,rx,FPOINT-data.currx+rx,data.totalrxk);
		}
	}
	
	if (data.totalrxk>=1024) {
		rxchange++;
		data.totalrxk-=1024;
	}
		
	data.currx=rx;
	data.totalrx+=rxchange;

	sscanf(procline+7,"%s %s %s %s %s %s %s %s %s",temp,temp,temp,temp,temp,temp,temp,temp,temp);
	tx=strtoul(temp, (char **)NULL, 0);

	if (newdb!=1) {
		if (data.curtx<=tx) {
			txchange=(tx-data.curtx)/1024/1024;
			data.totaltxk+=((tx-data.curtx)/1024)%1024;
			if (debug)
				printf("tx: %Lu - %Lu = %Lu\ntxk: %d\n",tx,data.curtx,tx-data.curtx,data.totaltxk);
		} else {
			txchange=(FPOINT-data.curtx+tx)/1024/1024;
			data.totaltxk+=((FPOINT-data.curtx+tx)/1024)%1024;
			if (debug)
				printf("tx: %Lu - %Lu + %Lu = %Lu\ntxk: %d\n",FPOINT,data.curtx,tx,FPOINT-data.curtx+tx,data.totaltxk);
		}
	}
	
	if (data.totaltxk>=1024) {
		txchange++;
		data.totaltxk-=1024;
	}
	
	data.curtx=tx;
	data.totaltx+=txchange;
	
	data.day[0].rx+=rxchange;
	data.day[0].tx+=txchange;
	data.month[0].rx+=rxchange;
	data.month[0].tx+=txchange;

	if (strncmp(data.day[0].date,(char*)asctime(localtime(&current)),10))
		rotatedays();
	if (strncmp(data.month[0].month,cdate+4,3))
		rotatemonths();

}

int readdb(char iface[32], char file[512])
{
	FILE *db;
	int newdb=0, i;
	
	if ((db=fopen(file,"r"))!=NULL) {
		fread(&data,sizeof(DATA),1,db);
		if (debug)
			printf("Database loaded for interface \"%s\"...\n",data.interface);
		fclose(db);
		if ((strcmp(data.interface,iface)) && (strcmp(iface,"default"))) {
			printf("Warning:\nThe current interface for this database is \"%s\".\n",data.interface);
			printf("Use --override-interface if you really want to change it to \"%s\".\n",iface);
			exit(1);
		}
	} else {
		printf("Error:\nUnable to read database \"%s\".\n",file);
		newdb=1;
		
		if (strcmp(iface,"default")==0) {
			strcpy(data.interface,"eth0");
		} else {
			strcpy(data.interface,iface);
		}
		data.version=1;
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
			strcpy(data.day[i].date,"   ");
		}
		for (i=0;i<=11;i++) {
			data.month[i].rx=0;
			data.month[i].tx=0;
			strcpy(data.month[i].month,"   ");
		}
		for (i=0;i<=9;i++) {
			data.top10[i].rx=0;
			data.top10[i].tx=0;
			strcpy(data.top10[i].date,"   ");
		}
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
	} else
		printf("Error:\nUnable to write database \"%s\".\n",file);
	
}

void changeinterface(char file[512], char newiface[32])
{
	FILE *db;
	
	if ((db=fopen(file,"r"))!=NULL) {
		fread(&data,sizeof(DATA),1,db);
		printf("Changing interface from %s to %s...",data.interface,newiface);
		if (debug)
			printf("\nDatabase loaded for interface \"%s\"...\n",data.interface);
		fclose(db);
	} else {
		printf("Error:\nUnable to read database \"%s\".\n",file);
		exit(1);
	}

	if ((db=fopen(file,"w"))!=NULL) {
		strcpy(data.interface,newiface);
		fwrite(&data,sizeof(DATA),1,db);
		if (debug)
			printf("Database saved...\n");
		fclose(db);
	} else {
		printf("\nError:\nUnable to write database \"%s\".\n",file);
		exit(1);
	}
	printf("done.\n");

}

void showdb(int qmode)
{
	float rx, tx;
	int i;
		
	if (data.totalrx+data.totaltx==0) {
		
		printf("Not enough data available yet.\n");
		
	} else {
		
		if (qmode==0) {
		
			rx=(data.totalrx/((float)data.totalrx+data.totaltx))*100;
			tx=(data.totaltx/((float)data.totalrx+data.totaltx))*100;
		
			/* printf("\nDatabase created: %s",(char*)asctime(localtime(&data.created))); */
		
			printf("Database updated: %s\n",(char*)asctime(localtime(&data.lastupdated)));
		
			printf("\t   Received: %'14Lu MB (%.1f%%)\n",data.totalrx,rx);
			printf("\tTransmitted: %'14Lu MB (%.1f%%)\n",data.totaltx,tx);
			printf("\t      Total: %'14Lu MB\n\n",data.totalrx+data.totaltx);
			
			printf("\t                rx     |     tx     |  total\n");
			printf("\t-----------------------+------------+-----------\n");
			printf("\t    Today   %'7Lu MB | %'7Lu MB | %'7Lu MB\n",data.day[0].rx,data.day[0].tx,data.day[0].rx+data.day[0].tx);
			printf("\tYesterday   %'7Lu MB | %'7Lu MB | %'7Lu MB\n",data.day[1].rx,data.day[1].tx,data.day[1].rx+data.day[1].tx);
			printf("\t-----------------------+------------+-----------\n");

		} else if (qmode==1) {
		
			printf("\t    day         rx      |     tx      |  total\n");
			printf("\t------------------------+-------------+--------------\n");
			
			for (i=0;i<=29;i++) {
				if (data.day[i].rx+data.day[i].tx==0 && i!=0)
					break;
				printf("\t   %s   %'7Lu MB  | %'7Lu MB  | %'7Lu MB\n",data.day[i].date+4,data.day[i].rx,data.day[i].tx,data.day[i].rx+data.day[i].tx);
			}
			printf("\t------------------------+-------------+--------------\n");
			
		} else if (qmode==2) {	
			
			printf("\t   month       rx      |      tx      |   total\n");
			printf("\t-----------------------+--------------+----------------\n");
			for (i=0;i<=11;i++) {
				if (data.month[i].rx+data.month[i].tx==0 && i!=0)
					break;
				printf("\t    %s   %'8Lu MB  | %'8Lu MB  | %'8Lu MB\n",data.month[i].month,data.month[i].rx,data.month[i].tx,data.month[i].rx+data.month[i].tx);
			}
			printf("\t-----------------------+--------------+----------------\n");			
			
		} else if (qmode==3) {
			
			printf("\t    #       day          rx      |     tx      |  total\n");
			printf("\t---------------------------------+-------------+--------------\n");
			for (i=0;i<=9;i++) {
				if (data.top10[i].rx+data.top10[i].tx==0)
					break;
				printf("\t   %2d   %s   %'7Lu MB  | %'7Lu MB  | %'7Lu MB\n",i+1,data.top10[i].date,data.top10[i].rx,data.top10[i].tx,data.top10[i].rx+data.top10[i].tx);
			}
			printf("\t---------------------------------+-------------+--------------\n");
		
		}

	}

}

void rotatedays(void)
{
	int i, j;
	time_t current;
	
	for (i=29;i>=1;i--) {
		data.day[i].rx=data.day[i-1].rx;
		data.day[i].tx=data.day[i-1].tx;
		strcpy(data.day[i].date,data.day[i-1].date);
	}
	
	current=time(NULL);
	
	data.day[0].rx=0;
	data.day[0].tx=0;
	strncpy(data.day[0].date,(char*)asctime(localtime(&current)),10);
	
	if (debug)
		printf("Days rotated. Current date: \"%s\".\n",data.day[0].date);

	for (i=0;i<=9;i++) {
		if ( data.day[1].rx+data.day[1].tx > data.top10[i].rx+data.top10[i].tx ) {
			for (j=9;j>=i+1;j--) {
				data.top10[j].rx=data.top10[j-1].rx;
				data.top10[j].tx=data.top10[j-1].tx;
				strcpy(data.top10[j].date,data.top10[j-1].date);
			}
			data.top10[i].rx=data.day[1].rx;
			data.top10[i].tx=data.day[1].tx;
			strcpy(data.top10[i].date,data.day[1].date);
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
	char cdate[8];
	
	for (i=11;i>=1;i--) {
		data.month[i].rx=data.month[i-1].rx;
		data.month[i].tx=data.month[i-1].tx;
		strcpy(data.month[i].month,data.month[i-1].month);
	}
	
	current=time(NULL);
	strncpy(cdate,(char*)asctime(localtime(&current)),8);
	
	data.month[0].rx=0;
	data.month[0].tx=0;
	strncpy(data.month[0].month,cdate+4,3);
	
	if (debug)
		printf("Months rotated. Current month: \"%s\".\n",data.month[0].month);
}
