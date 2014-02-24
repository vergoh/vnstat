#include "common.h"
#include "vnstati.h"
#include "misc.h"
#include "image.h"

void colorinit(void)
{
	int rgb[3];

	/* text, edge and header colors */
	hextorgb(cfg.ctext, rgb);
	ctext = gdImageColorAllocate(im, rgb[0], rgb[1], rgb[2]);
	hextorgb(cfg.cedge, rgb);
	cedge = gdImageColorAllocate(im, rgb[0], rgb[1], rgb[2]);
	hextorgb(cfg.cheader, rgb);
	cheader = gdImageColorAllocate(im, rgb[0], rgb[1], rgb[2]);
	hextorgb(cfg.cheadertitle, rgb);
	cheadertitle = gdImageColorAllocate(im, rgb[0], rgb[1], rgb[2]);
	hextorgb(cfg.cheaderdate, rgb);
	cheaderdate = gdImageColorAllocate(im, rgb[0], rgb[1], rgb[2]);

	/* lines */
	hextorgb(cfg.cline, rgb);
	cline = gdImageColorAllocate(im, rgb[0], rgb[1], rgb[2]);
	if (cfg.clinel[0] == '-') {
		modcolor(rgb, 50, 1);
	} else {
		hextorgb(cfg.clinel, rgb);
	}
	clinel = gdImageColorAllocate(im, rgb[0], rgb[1], rgb[2]);

	/* background */
	hextorgb(cfg.cbg, rgb);	
	cbackground = gdImageColorAllocate(im, rgb[0], rgb[1], rgb[2]);
	modcolor(rgb, -35, 0);
	cvnstat = gdImageColorAllocate(im, rgb[0], rgb[1], rgb[2]);

	/* rx */
	hextorgb(cfg.crx, rgb);
	crx = gdImageColorAllocate(im, rgb[0], rgb[1], rgb[2]);
	if (cfg.crxd[0] == '-') {
		modcolor(rgb, -50, 1);
	} else {
		hextorgb(cfg.crxd, rgb);
	}
	crxd = gdImageColorAllocate(im, rgb[0], rgb[1], rgb[2]);
	
	/* tx */
	hextorgb(cfg.ctx, rgb);
	ctx = gdImageColorAllocate(im, rgb[0], rgb[1], rgb[2]);
	if (cfg.ctxd[0] == '-') {
		modcolor(rgb, -50, 1);
	} else {
		hextorgb(cfg.ctxd, rgb);
	}
	ctxd = gdImageColorAllocate(im, rgb[0], rgb[1], rgb[2]);
}

void layoutinit(char *title, int width, int height, int showheader, int showedge)
{
	struct tm *d;
	char datestring[32];

	/* get time in given format */
	d = localtime(&data.lastupdated);
	strftime(datestring, 32, cfg.hformat, d);

	/* background, edges */
	gdImageFill(im, 0, 0, cbackground);
	if (showedge) {
		gdImageRectangle(im, 0, 0, width-1, height-1, cedge);
	}

	/* titlebox, title, date */
	if (showheader) {
		if (showedge) {
			gdImageFilledRectangle(im, 3, 3, width-4, 24, cheader);
			gdImageString(im, gdFontGetGiant(), 12, 6, (unsigned char*)title, cheadertitle);
			gdImageString(im, gdFontGetTiny(), width-(strlen(datestring)*gdFontGetTiny()->w+12), 10, (unsigned char*)datestring, cheaderdate);
		} else {
			gdImageFilledRectangle(im, 2, 2, width-3, 24, cheader);
			gdImageString(im, gdFontGetGiant(), 12, 5, (unsigned char*)title, cheadertitle);
			gdImageString(im, gdFontGetTiny(), width-(strlen(datestring)*gdFontGetTiny()->w+12), 9, (unsigned char*)datestring, cheaderdate);
		}
	} else {
		if (showedge) {
			gdImageString(im, gdFontGetTiny(), 6, height-13, (unsigned char*)datestring, cvnstat);
		} else {
			gdImageString(im, gdFontGetTiny(), 5, height-12, (unsigned char*)datestring, cvnstat);
		}
	}

	/* generator */
	if (showedge) {
		gdImageString(im, gdFontGetTiny(), width-115, height-13, (unsigned char*)"vnStat / Teemu Toivola", cvnstat);
	} else {
		gdImageString(im, gdFontGetTiny(), width-114, height-12, (unsigned char*)"vnStat / Teemu Toivola", cvnstat);
	}
}

void drawlegend(int x, int y)
{

	/* color legend */
	gdImageString(im, gdFontGetSmall(), x, y, (unsigned char*)"rx     tx", ctext);
	gdImageFilledRectangle(im, x-12, y+4, x-6, y+10, crx);
	gdImageRectangle(im, x-12, y+4, x-6, y+10, ctext);
	gdImageFilledRectangle(im, x+30, y+4, x+36, y+10, ctx);
	gdImageRectangle(im, x+30, y+4, x+36, y+10, ctext);	

}

void drawbar(int x, int y, int len, uint64_t rx, int rxk, uint64_t tx, int txk, uint64_t max)
{
	int l;
	
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

		if (tx>rx) {
			l=rintf((rx/(float)(rx+tx)*len));

			gdImageFilledRectangle(im, x, y+YBEGINOFFSET, x+l, y+YENDOFFSET, crx);
			gdImageRectangle(im, x, y+YBEGINOFFSET, x+l, y+YENDOFFSET, crxd);

			gdImageFilledRectangle(im, x+l, y+YBEGINOFFSET, x+len, y+YENDOFFSET, ctx);
			gdImageRectangle(im, x+l, y+YBEGINOFFSET, x+len, y+YENDOFFSET, ctxd);

		} else {
			l=rintf((tx/(float)(rx+tx)*len));

			gdImageFilledRectangle(im, x, y+YBEGINOFFSET, x+(len-l), y+YENDOFFSET, crx);
			gdImageRectangle(im, x, y+YBEGINOFFSET, x+(len-l), y+YENDOFFSET, crxd);

			gdImageFilledRectangle(im, x+(len-l), y+YBEGINOFFSET, x+len, y+YENDOFFSET, ctx);
			gdImageRectangle(im, x+(len-l), y+YBEGINOFFSET, x+len, y+YENDOFFSET, ctxd);
		}
	}
}

void drawpole(int x, int y, int len, uint64_t rx, uint64_t tx, uint64_t max)
{
	int l;

	l = (rx/(float)max)*len;
	gdImageFilledRectangle(im, x, y+(len-l), x+7, y+len, crx);

	l = (tx/(float)max)*len;
	gdImageFilledRectangle(im, x+5, y+(len-l), x+12, y+len, ctx);
}

void drawhours(int x, int y, int rate)
{
	int i, tmax=0, s=0, step, prev=0, diff=0, chour;
	uint64_t max=1, scaleunit=0;
	char buffer[32];
	time_t current;
	struct tm *d;
	
	current = time(NULL);
	chour = localtime(&current)->tm_hour;

	/* tmax (time max) = current hour */
	/* max = transfer max */

	for (i = 0; i < 24; i++) {

		/* convert hourly transfer to hourly rate if needed */
		if (rate) {
			if ((current-data.hour[i].date) > 3600) {
				data.hour[i].rx = data.hour[i].rx / (float)450; /* rx * 8 / 3600 */
				data.hour[i].tx = data.hour[i].tx / (float)450; /* tx * 8 / 3600 */			
			} else {
				/* scale ongoing hour properly */
				d = localtime(&data.hour[i].date);
				if (chour != d->tm_hour) {
					data.hour[i].rx = data.hour[i].rx / (float)450; /* rx * 8 / 3600 */
					data.hour[i].tx = data.hour[i].tx / (float)450; /* tx * 8 / 3600 */				
				} else {
					diff = d->tm_min*60;
					if (!diff) {
						diff = 1;
					}
					data.hour[i].rx = data.hour[i].rx * 8 / (float)diff;
					data.hour[i].tx = data.hour[i].tx * 8 / (float)diff;
				}
			}
		}

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
	
	/* scale values */
	scaleunit = getscale(max);
	if (max/scaleunit > 4) {
		step = 2;
	} else {
		step = 1;
	}
	
	for (i=step; scaleunit*i <= max; i=i+step) {
		s = 121 * ((scaleunit * i) / (float)max);
		gdImageLine(im, x+36, y+124-s, x+460, y+124-s, cline);
		gdImageLine(im, x+36, y+124-((s+prev)/2), x+460, y+124-((s+prev)/2), clinel);
		gdImageString(im, gdFontGetTiny(), x+16, y+121-s, (unsigned char*)getimagevalue(scaleunit*i, 3, rate), ctext);
		prev = s;
	}
	s = 121 * ((scaleunit * i) / (float)max);
	if ( ((s+prev)/2) <= 128 ) {
		gdImageLine(im, x+36, y+124-((s+prev)/2), x+460, y+124-((s+prev)/2), clinel);
	}

	/* scale text */
	if (rate) {
		gdImageStringUp(im, gdFontGetTiny(), x-2, y+70, (unsigned char*)getimagescale(scaleunit, 1), ctext);
	} else {
		gdImageStringUp(im, gdFontGetTiny(), x-2, y+60, (unsigned char*)getimagescale(scaleunit, 0), ctext);
	}

	/* x-axis values and poles */
	for (i = 0; i < 24; i++) {
		s=tmax-i;
		if (s<0) {
			s+=24;
		}
		sprintf(buffer, "%02d ", s);
		gdImageString(im, gdFontGetTiny(), x+440-(i*17), y+128, (unsigned char*)buffer, ctext);
		drawpole(x+438-(i*17), y, 124, data.hour[s].rx, data.hour[s].tx, max);
	}

	/* axis */
	gdImageLine(im, x+36-4, y+124, x+466, y+124, ctext);
	gdImageLine(im, x+36, y-10, x+36, y+124+4, ctext);

	/* arrows */
	gdImageLine(im, x+465, y+124, x+462, y+122, ctext);
	gdImageLine(im, x+465, y+124, x+462, y+126, ctext);
	gdImageLine(im, x+462, y+122, x+462, y+126, ctext);
	gdImageLine(im, x+36, y-9, x+38, y-6, ctext);
	gdImageLine(im, x+36, y-9, x+34, y-6, ctext);
	gdImageLine(im, x+34, y-6, x+38, y-6, ctext);

}

void drawsummary(int type, int showheader, int showedge, int rate)
{
	int piex, piey, piew, pieh, arc, textx, texty;
	int i, tk, width, height, headermod;
	float rxp = 50, txp = 50;
	uint64_t t, max, e_rx, e_tx;
	char buffer[512], datebuff[16], daytemp[32], daytemp2[32];
	time_t yesterday;
	struct tm *d;

	switch (type) {
		case 1:
			width = 980;
			height = 200;
			break;
		case 2:
			width = 500;
			height = 370;
			break;
		default:
			width = 500;
			height = 200;
			break;
	}

	if (!showheader) {
		headermod = 26;
		height -= 22;
	} else {
		headermod = 0;
	}

	yesterday=current-86400;

	im = gdImageCreate(width, height);

	colorinit();
	
	if (strcmp(data.nick, data.interface)==0) {
		sprintf(buffer, "%s", data.interface);	
	} else {
		sprintf(buffer, "%s (%s)", data.nick, data.interface);
	}
	
	layoutinit(buffer, width, height, showheader, showedge);
	drawlegend(383, 110-headermod);

	if (data.totalrx || data.totalrxk || data.totaltx || data.totaltxk) {
		if (data.totalrx>1024 || data.totaltx>1024)	{
			rxp = (data.totalrx/(float)(data.totalrx+data.totaltx))*100;
		} else {
			rxp = ( ((data.totalrx*1024)+data.totalrxk) / (float)(((data.totalrx*1024)+data.totalrxk)+((data.totaltx*1024)+data.totaltxk)) )*100;
		}	
		txp = (float)100 - rxp;
	}

	d=localtime(&data.lastupdated);

	if ( data.day[0].rx==0 || data.day[0].tx==0 || (d->tm_hour*60+d->tm_min)==0 ) {
		e_rx = e_tx=0;
	} else {	
		e_rx = ((data.day[0].rx)/(float)(d->tm_hour*60+d->tm_min))*1440;
		e_tx = ((data.day[0].tx)/(float)(d->tm_hour*60+d->tm_min))*1440;
	}
	
	piex = 400;
	piey = 63-headermod;
	piew = 110;
	pieh = 45;
	arc = (txp / (float)100) * 360;
	
	/* pie chart */
	for(i = 14; i > 0; i--) {
		gdImageFilledArc(im, piex, piey+i, piew, pieh, 270, 270+arc, ctxd, gdEdged|gdNoFill);
		gdImageFilledArc(im, piex, piey+i, piew, pieh, 270+arc, 270, crxd, gdEdged|gdNoFill);
	}

	gdImageFilledArc(im, piex, piey, piew, pieh, 270, 270+arc, ctx, 0);
	gdImageFilledArc(im, piex, piey, piew, pieh, 270, 270+arc, ctxd, gdEdged|gdNoFill);
	gdImageFilledArc(im, piex, piey, piew, pieh, 270+arc, 270, crx, 0);
	gdImageFilledArc(im, piex, piey, piew, pieh, 270+arc, 270, crxd, gdEdged|gdNoFill);
	
	textx = 30;
	texty = 48-headermod;

	/* totals */
	sprintf(buffer, "   received: %s  (%.1f%%)", getvalue(data.totalrx, data.totalrxk, 9, -1), rxp);
	gdImageString(im, gdFontGetLarge(), textx, texty, (unsigned char*)buffer, ctext);
	sprintf(buffer, "transmitted: %s  (%.1f%%)", getvalue(data.totaltx, data.totaltxk, 9, -1), txp);
	gdImageString(im, gdFontGetLarge(), textx, texty+15, (unsigned char*)buffer, ctext);
	sprintf(buffer, "      total: %s", getvalue(data.totalrx+data.totaltx, data.totalrxk+data.totaltxk, 9, -1));
	gdImageString(im, gdFontGetLarge(), textx, texty+30, (unsigned char*)buffer, ctext);
	
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

	/* get formated date for today */
	d=localtime(&current);
	strftime(datebuff, 16, cfg.dformat, d);

	/* get formated date for current day in database */
	d=localtime(&data.day[0].date);
	strftime(daytemp2, 16, cfg.dformat, d);

	/* change daytemp to today if formated days match */
	if (strcmp(datebuff, daytemp2)==0) {
		strncpy(daytemp2, "today", 32);
	}

	textx = 20;
	texty = 118-headermod;

	/* yesterday & today */
	gdImageString(im, gdFontGetSmall(), textx, texty, (unsigned char*)"                rx           tx        total", ctext);
	gdImageLine(im, textx-4, texty+16, textx+290, texty+16, cline);
	gdImageLine(im, textx-4, texty+49, textx+290, texty+49, cline);
	gdImageLine(im, textx+140, texty+4, textx+140, texty+64, cline);
	gdImageLine(im, textx+218, texty+4, textx+218, texty+64, cline);

	if (data.day[1].date!=0) {
		snprintf(buffer, 32, "%9s   ", daytemp);
		strncat(buffer, getvalue(data.day[1].rx, data.day[1].rxk, 6, -1), 32);
		strcat(buffer, "   ");
		strncat(buffer, getvalue(data.day[1].tx, data.day[1].txk, 6, -1), 32);
		strcat(buffer, "   ");
		strncat(buffer, getvalue(data.day[1].rx+data.day[1].tx, data.day[1].rxk+data.day[1].txk, 6, -1), 32);
		gdImageString(im, gdFontGetSmall(), textx, texty+20, (unsigned char*)buffer, ctext);
	}

	snprintf(buffer, 32, "%9s   ", daytemp2);
	strncat(buffer, getvalue(data.day[0].rx, data.day[0].rxk, 6, -1), 32);
	strcat(buffer, "   ");
	strncat(buffer, getvalue(data.day[0].tx, data.day[0].txk, 6, -1), 32);
	strcat(buffer, "   ");
	strncat(buffer, getvalue(data.day[0].rx+data.day[0].tx, data.day[0].rxk+data.day[0].txk, 6, -1), 32);
	gdImageString(im, gdFontGetSmall(), textx, texty+32, (unsigned char*)buffer, ctext);

	sprintf(buffer, "estimated   ");
	strncat(buffer, getvalue(e_rx, 0, 6, -2), 32);
	strcat(buffer, "   ");
	strncat(buffer, getvalue(e_tx, 0, 6, -2), 32);
	strcat(buffer, "   ");
	strncat(buffer, getvalue(e_rx+e_tx, 0, 6, -2), 32);
	gdImageString(im, gdFontGetSmall(), textx, texty+52, (unsigned char*)buffer, ctext);

	/* search maximum */
	max=1;
	for (i = 1; i >= 0; i--) {
		if (data.day[i].used) {
				
			t=data.day[i].rx+data.day[i].tx;
			tk=data.day[i].rxk+data.day[i].txk;
					
			if (tk>=1024) {
				t+=tk/1024;
				tk-=(tk/1024)*1024;
			}
					
			t=(t*1024)+tk;
					
			if (t>max) {
				max=t;
			}
		}
	}
	
	/* bars for both */
	drawbar(textx+300, texty+24, 165, data.day[1].rx, data.day[1].rxk, data.day[1].tx, data.day[1].txk, max);
	drawbar(textx+300, texty+36, 165, data.day[0].rx, data.day[0].rxk, data.day[0].tx, data.day[0].txk, max);

	/* hours if requested */
	switch (type) {
		case 1:
			drawhours(500, 46-headermod, rate);
			break;
		case 2:
			drawhours(16, 215-headermod, rate);
			break;
		default:
			break;
	}

}

void drawhourly(int showheader, int showedge, int rate)
{
	int width, height;
	char buffer[512];

	width = 500;
	height = 200;

	if (!showheader) {
		height -= 22;
	}

	im = gdImageCreate(width, height);

	colorinit();
	
	if (strcmp(data.nick, data.interface)==0) {
		sprintf(buffer, "%s / hourly", data.interface);	
	} else {
		sprintf(buffer, "%s (%s) / hourly", data.nick, data.interface);
	}
	
	layoutinit(buffer, width, height, showheader, showedge);

	if (showheader) {
		drawhours(12, 46, rate);
	} else {
		drawhours(12, 22, rate);
	}
}

void drawdaily(int showheader, int showedge)
{
	int textx, texty, lines;
	int i, tk, width, height, headermod;
	uint64_t t, max, e_rx, e_tx;
	char buffer[512], datebuff[16];
	struct tm *d;

	/* count how many days needs to be shown */
	lines = 0;
	for (i = 0; i < 30; i++) {
		if (data.day[i].used) {
			lines++;
		}
	}
	
	width = 500;
	height = 98 + (12 * lines);

	if (!showheader) {
		headermod = 26;
		height -= 22;
	} else {
		headermod = 0;
	}

	im = gdImageCreate(width, height);

	colorinit();
	
	if (strcmp(data.nick, data.interface)==0) {
		sprintf(buffer, "%s / daily", data.interface);	
	} else {
		sprintf(buffer, "%s (%s) / daily", data.nick, data.interface);
	}
	
	layoutinit(buffer, width, height, showheader, showedge);
	
	if (lines) {
		drawlegend(385, 40-headermod);
	}

	textx = 10;
	texty = 40-headermod;
	
	gdImageString(im, gdFontGetSmall(), textx, texty, (unsigned char*)"    day         rx            tx         total", ctext);
	gdImageLine(im, textx+2, texty+16, textx+302, texty+16, cline);
	gdImageLine(im, textx+144, texty+2, textx+144, texty+40+(12*lines), cline);
	gdImageLine(im, textx+228, texty+2, textx+228, texty+40+(12*lines), cline);

	texty += 20;

	/* search maximum */
	max=1;
	for (i = 29; i >= 0; i--) {
		if (data.day[i].used) {
				
			t=data.day[i].rx+data.day[i].tx;
			tk=data.day[i].rxk+data.day[i].txk;
					
			if (tk>=1024) {
				t+=tk/1024;
				tk-=(tk/1024)*1024;
			}
					
			t=(t*1024)+tk;
					
			if (t>max) {
				max=t;
			}
		}
	}
	
	for (i = 29; i >= 0; i--) {
		if (data.day[i].used) {

			d = localtime(&data.day[i].date);
			strftime(datebuff, 16, cfg.dformat, d);
			snprintf(buffer, 32, " %8s   ", datebuff);
			strncat(buffer, getvalue(data.day[i].rx, data.day[i].rxk, 6, -1), 32);
			strcat(buffer, "    ");
			strncat(buffer, getvalue(data.day[i].tx, data.day[i].txk, 6, -1), 32);
			strcat(buffer, "    ");
			strncat(buffer, getvalue(data.day[i].rx+data.day[i].tx, data.day[i].rxk+data.day[i].txk, 6, -1), 32);
			gdImageString(im, gdFontGetSmall(), textx, texty, (unsigned char*)buffer, ctext);
			drawbar(textx+310, texty+4, 165, data.day[i].rx, data.day[i].rxk, data.day[i].tx, data.day[i].txk, max);
			texty += 12;
		}
	}

	if (lines==0) {
		gdImageString(im, gdFontGetSmall(), textx, texty, (unsigned char*)"                 no data available", ctext);
		texty += 12;
	}

	gdImageLine(im, textx+2, texty+5, textx+302, texty+5, cline);

	if (lines) {

		d=localtime(&data.lastupdated);
		if ( data.day[0].rx==0 || data.day[0].tx==0 || (d->tm_hour*60+d->tm_min)==0 ) {
			e_rx=e_tx=0;
		} else {				
			e_rx=((data.day[0].rx)/(float)(d->tm_hour*60+d->tm_min))*1440;
			e_tx=((data.day[0].tx)/(float)(d->tm_hour*60+d->tm_min))*1440;
		}
		sprintf(buffer, " estimated  ");
		strncat(buffer, getvalue(e_rx, 0, 6, -2), 32);
		strcat(buffer, "    ");
		strncat(buffer, getvalue(e_tx, 0, 6, -2), 32);
		strcat(buffer, "    ");
		strncat(buffer, getvalue(e_rx+e_tx, 0, 6, -2), 32);
	
		gdImageString(im, gdFontGetSmall(), textx, texty+8, (unsigned char*)buffer, ctext);
	}

}

void drawmonthly(int showheader, int showedge)
{
	int textx, texty, lines;
	int i, tk, width, height, headermod;
	uint64_t t, max, e_rx, e_tx;
	char buffer[512], datebuff[16];
	struct tm *d;
	int dmonth[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

	/* count how many months needs to be shown */
	lines = 0;
	for (i = 0; i < 12; i++) {
		if (data.month[i].used) {
			lines++;
		}
	}
	
	width = 500;
	height = 98 + (12 * lines);

	if (!showheader) {
		headermod = 26;
		height -= 22;
	} else {
		headermod = 0;
	}

	im = gdImageCreate(width, height);

	colorinit();
	
	if (strcmp(data.nick, data.interface)==0) {
		sprintf(buffer, "%s / monthly", data.interface);	
	} else {
		sprintf(buffer, "%s (%s) / monthly", data.nick, data.interface);
	}
	
	layoutinit(buffer, width, height, showheader, showedge);
	
	if (lines) {
		drawlegend(385, 40-headermod);
	}

	textx = 10;
	texty = 40-headermod;
	
	gdImageString(im, gdFontGetSmall(), textx, texty, (unsigned char*)"   month        rx            tx         total", ctext);
	gdImageLine(im, textx+2, texty+16, textx+302, texty+16, cline);
	gdImageLine(im, textx+144, texty+2, textx+144, texty+40+(12*lines), cline);
	gdImageLine(im, textx+228, texty+2, textx+228, texty+40+(12*lines), cline);

	texty += 20;

	/* search maximum */
	max=1;
	for (i = 11; i >= 0; i--) {
		if (data.month[i].used) {
				
			t=data.month[i].rx+data.month[i].tx;
			tk=data.month[i].rxk+data.month[i].txk;
					
			if (tk>=1024) {
				t+=tk/1024;
				tk-=(tk/1024)*1024;
			}
					
			t=(t*1024)+tk;
					
			if (t>max) {
				max=t;
			}
		}
	}
	
	for (i = 11; i >= 0; i--) {
		if (data.month[i].used) {

			d = localtime(&data.month[i].month);
			strftime(datebuff, 16, cfg.mformat, d);
			snprintf(buffer, 32, " %8s   ", datebuff);
			strncat(buffer, getvalue(data.month[i].rx, data.month[i].rxk, 6, -1), 32);
			strcat(buffer, "    ");
			strncat(buffer, getvalue(data.month[i].tx, data.month[i].txk, 6, -1), 32);
			strcat(buffer, "    ");
			strncat(buffer, getvalue(data.month[i].rx+data.month[i].tx, data.month[i].rxk+data.month[i].txk, 6, -1), 32);
			gdImageString(im, gdFontGetSmall(), textx, texty, (unsigned char*)buffer, ctext);
			drawbar(textx+310, texty+4, 165, data.month[i].rx, data.month[i].rxk, data.month[i].tx, data.month[i].txk, max);
			texty += 12;
		}
	}

	if (lines==0) {
		gdImageString(im, gdFontGetSmall(), textx, texty, (unsigned char*)"                 no data available", ctext);
		texty += 12;
	}

	gdImageLine(im, textx+2, texty+5, textx+302, texty+5, cline);

	if (lines) {

		d=localtime(&data.lastupdated);
		if ( data.month[0].rx==0 || data.month[0].tx==0 || ((d->tm_mday-1)*24+d->tm_hour)==0 ) {
			e_rx=e_tx=0;
		} else {				
			e_rx=((data.month[0].rx)/(float)((d->tm_mday-1)*24+d->tm_hour))*(dmonth[d->tm_mon]*24);
			e_tx=((data.month[0].tx)/(float)((d->tm_mday-1)*24+d->tm_hour))*(dmonth[d->tm_mon]*24);
		}
		sprintf(buffer, " estimated  ");
		strncat(buffer, getvalue(e_rx, 0, 6, -2), 32);
		strcat(buffer, "    ");
		strncat(buffer, getvalue(e_tx, 0, 6, -2), 32);
		strcat(buffer, "    ");
		strncat(buffer, getvalue(e_rx+e_tx, 0, 6, -2), 32);
	
		gdImageString(im, gdFontGetSmall(), textx, texty+8, (unsigned char*)buffer, ctext);
	}
}

void drawtop(int showheader, int showedge)
{
	int textx, texty, lines = 0;
	int i, tk, width, height, headermod;
	uint64_t t, max = 1;
	char buffer[512], datebuff[16];
	struct tm *d;

	/* count how many days needs to be shown and search maximum */
	for (i = 0; i < 10; i++) {
		if (data.top10[i].used) {
			lines++;
			t=data.top10[i].rx+data.top10[i].tx;
			tk=data.top10[i].rxk+data.top10[i].txk;
			if (tk>=1024) {
				t+=tk/1024;
				tk-=(tk/1024)*1024;
			}
			t=(t*1024)+tk;
			if (t>max) {
				max=t;
			}
		}
	}
	
	width = 500;
	height = 86 + (12 * lines);

	if (!showheader) {
		headermod = 26;
		height -= 22;
	} else {
		headermod = 0;
	}

	im = gdImageCreate(width, height);

	colorinit();
	
	if (strcmp(data.nick, data.interface)==0) {
		sprintf(buffer, "%s / top 10", data.interface);	
	} else {
		sprintf(buffer, "%s (%s) / top 10", data.nick, data.interface);
	}
	
	layoutinit(buffer, width, height, showheader, showedge);
	
	if (lines) {
		drawlegend(405, 40-headermod);
	}

	textx = 10;
	texty = 40-headermod;
	
	gdImageString(im, gdFontGetSmall(), textx, texty, (unsigned char*)"   #      day         rx            tx         total", ctext);
	gdImageLine(im, textx+2, texty+16, textx+338, texty+16, cline);
	if (lines) {
		gdImageLine(im, textx+180, texty+2, textx+180, texty+24+(12*lines), cline);
		gdImageLine(im, textx+264, texty+2, textx+264, texty+24+(12*lines), cline);
	}

	texty += 20;

	for (i = 0; i < 10; i++) {
		if (data.top10[i].used) {

			d = localtime(&data.top10[i].date);
			strftime(datebuff, 16, cfg.tformat, d);
			snprintf(buffer, 32, "  %2d   %8s   ", i+1, datebuff);
			strncat(buffer, getvalue(data.top10[i].rx, data.top10[i].rxk, 6, -1), 32);
			strcat(buffer, "    ");
			strncat(buffer, getvalue(data.top10[i].tx, data.top10[i].txk, 6, -1), 32);
			strcat(buffer, "    ");
			strncat(buffer, getvalue(data.top10[i].rx+data.top10[i].tx, data.top10[i].rxk+data.top10[i].txk, 6, -1), 32);
			gdImageString(im, gdFontGetSmall(), textx, texty, (unsigned char*)buffer, ctext);
			drawbar(textx+346, texty+4, 130, data.top10[i].rx, data.top10[i].rxk, data.top10[i].tx, data.top10[i].txk, max);
			texty += 12;
		}
	}

	if (lines==0) {
		gdImageString(im, gdFontGetSmall(), textx, texty, (unsigned char*)"                     no data available", ctext);
		texty += 12;
	}

	gdImageLine(im, textx+2, texty+5, textx+338, texty+5, cline);

}

void hextorgb(char *input, int *rgb)
{
	int offset;
	char hex[3], dec[4];

	if (input[0] == '#') {
		offset = 1;
	} else {
		offset = 0;
	}

	sprintf(hex, "%c%c", input[(0+offset)], input[(1+offset)]);
	sprintf(dec, "%d", (int)strtol(hex, NULL, 16));
	rgb[0] = atoi(dec);
	sprintf(hex, "%c%c", input[(2+offset)], input[(3+offset)]);
	sprintf(dec, "%d", (int)strtol(hex, NULL, 16));
	rgb[1] = atoi(dec);
	sprintf(hex, "%c%c", input[(4+offset)], input[(5+offset)]);
	sprintf(dec, "%d", (int)strtol(hex, NULL, 16));
	rgb[2] = atoi(dec);

	if (debug) {
		printf("%s -> %d, %d, %d\n", input, rgb[0], rgb[1], rgb[2]);
	}
}

void modcolor(int *rgb, int offset, int force)
{
	int i, overflow = 0;

	if (debug) {
		printf("m%d (%d): %d, %d, %d -> ", offset, force, rgb[0], rgb[1], rgb[2]);
	}

	for (i=0; i<3; i++) {
		if ((rgb[i]+offset)>255 || (rgb[i]+offset)<0) {
			overflow++;
		}
	}

	/* positive offset gives lighter color, negative darker if forced */
	/* otherwise the direction is changed depending on possible overflows */
	for (i=0; i<3; i++) {
		if (overflow<2 || force) {
			if ((rgb[i]+offset)>255) {
				rgb[i] = 255;
			} else if ((rgb[i]+offset)<0) {
				rgb[i] = 0;
			} else {
				rgb[i] += offset; 
			}
		} else {
			if ((rgb[i]-offset)<0) {
				rgb[i] = 0;
			} else if ((rgb[i]-offset)>255) {
				rgb[i] = 255;
			} else {
				rgb[i] -= offset; 
			}		
		}
	}

	if (debug) {
		printf("%d, %d, %d\n", rgb[0], rgb[1], rgb[2]);
	}
}

char *getimagevalue(uint64_t kb, int len, int rate)
{
	static char buffer[64];
	int declen=0;

	if (kb==0){
		sprintf(buffer, "%*s", len, "--");
	} else {
		/* try to figure out what unit to use */
		if (rate) {
			if (kb>=1000000000) { /* 1000*1000*1000 - value >=1000 Gbps -> show in Tbps */
				sprintf(buffer, "%*.*f", len, declen, kb/(float)1000000000); /* 1000*1000*1000 */
			} else if (kb>=1000000) { /* 1000*1000 - value >=1000 Mbps -> show in Gbps */
				sprintf(buffer, "%*.*f", len, declen, kb/(float)1000000); /* 1000*1000 */
			} else if (kb>=1000) {
				sprintf(buffer, "%*.*f", len, declen, kb/(float)1000);
			} else {
				sprintf(buffer, "%*"PRIu64"", len, kb);
			}		
		} else {
			if (kb>=1048576000) { /* 1024*1024*1000 - value >=1000 GiB -> show in TiB */
				sprintf(buffer, "%*.*f", len, declen, kb/(float)1073741824); /* 1024*1024*1024 */
			} else if (kb>=1024000) { /* 1024*1000 - value >=1000 MiB -> show in GiB */
				sprintf(buffer, "%*.*f", len, declen, kb/(float)1048576); /* 1024*1024 */
			} else if (kb>=1000) {
				sprintf(buffer, "%*.*f", len, declen, kb/(float)1024);
			} else {
				sprintf(buffer, "%*"PRIu64"", len, kb);
			}
		}
	}
	
	return buffer;
}

char *getimagescale(uint64_t kb, int rate)
{
	static char buffer[6];
	
	if (kb==0) {
		sprintf(buffer, "--");
	} else {
	
		if (rate) {
			if (kb>=1000000000) { /* 1000*1000*1000 - value >=1000 Gbps -> show in Tbps */
				sprintf(buffer, "%s/s", getbunit(2, 4));
			} else if (kb>=1000000) { /* 1000*1000 - value >=1000 Mbps -> show in Gbps */
				sprintf(buffer, "%s/s", getbunit(2, 3));
			} else if (kb>=1000) {
				sprintf(buffer, "%s/s", getbunit(2, 2));
			} else {
				sprintf(buffer, "%s/s", getbunit(2, 1));
			}		
		} else {
			if (kb>=1048576000) { /* 1024*1024*1000 - value >=1000 GiB -> show in TiB */
				sprintf(buffer, "%s", getunit(4));
			} else if (kb>=1024000) { /* 1024*1000 - value >=1000 MiB -> show in GiB */
				sprintf(buffer, "%s", getunit(3));
			} else if (kb>=1000) {
				sprintf(buffer, "%s", getunit(2));
			} else {
				sprintf(buffer, "%s", getunit(1));
			}		
		}
	
	}

	return buffer;
}
