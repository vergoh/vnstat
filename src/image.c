#include "common.h"
#include "vnstati.h"
#include "misc.h"
#include "image.h"

void initimagecontent(IMAGECONTENT *ic)
{
	ic->showheader = 1;
	ic->showedge = 1;
	ic->showlegend = 1;
	ic->altdate = 0;
	ic->headertext[0] = '\0';
}

void drawimage(IMAGECONTENT *ic)
{
	switch (cfg.qmode) {
		case 1:
			drawdaily(ic);
			break;
		case 2:
			drawmonthly(ic);
			break;
		case 3:
			drawtop(ic);
			break;
		case 5:
			if (cfg.slayout) {
				drawsummary(ic, 0, 0);
			} else {
				drawoldsummary(ic, 0, 0);
			}
			break;
		case 51:
			if (cfg.slayout) {
				drawsummary(ic, 1, cfg.hourlyrate);
			} else {
				drawoldsummary(ic, 1, cfg.hourlyrate);
			}
			break;
		case 52:
			if (cfg.slayout) {
				drawsummary(ic, 2, cfg.hourlyrate);
			} else {
				drawoldsummary(ic, 2, cfg.hourlyrate);
			}
			break;
		case 7:
			drawhourly(ic, cfg.hourlyrate);
			break;
		default:
			break;
	}

	/* enable background transparency if needed */
	if (cfg.transbg) {
		gdImageColorTransparent(ic->im, ic->cbackground);
	}
}

void colorinit(IMAGECONTENT *ic)
{
	int rgb[3];

	/* text, edge and header colors */
	hextorgb(cfg.ctext, rgb);
	ic->ctext = gdImageColorAllocate(ic->im, rgb[0], rgb[1], rgb[2]);
	colorinitcheck("ctext", ic->ctext, cfg.ctext, rgb);
	hextorgb(cfg.cedge, rgb);
	ic->cedge = gdImageColorAllocate(ic->im, rgb[0], rgb[1], rgb[2]);
	colorinitcheck("cedge", ic->cedge, cfg.cedge, rgb);
	hextorgb(cfg.cheader, rgb);
	ic->cheader = gdImageColorAllocate(ic->im, rgb[0], rgb[1], rgb[2]);
	colorinitcheck("cheader", ic->cheader, cfg.cheader, rgb);
	hextorgb(cfg.cheadertitle, rgb);
	ic->cheadertitle = gdImageColorAllocate(ic->im, rgb[0], rgb[1], rgb[2]);
	colorinitcheck("cheadertitle", ic->cheadertitle, cfg.cheadertitle, rgb);
	hextorgb(cfg.cheaderdate, rgb);
	ic->cheaderdate = gdImageColorAllocate(ic->im, rgb[0], rgb[1], rgb[2]);
	colorinitcheck("cheaderdate", ic->cheaderdate, cfg.cheaderdate, rgb);

	/* lines */
	hextorgb(cfg.cline, rgb);
	ic->cline = gdImageColorAllocate(ic->im, rgb[0], rgb[1], rgb[2]);
	colorinitcheck("cline", ic->cline, cfg.cline, rgb);
	if (cfg.clinel[0] == '-') {
		modcolor(rgb, 50, 1);
	} else {
		hextorgb(cfg.clinel, rgb);
	}
	ic->clinel = gdImageColorAllocate(ic->im, rgb[0], rgb[1], rgb[2]);
	colorinitcheck("clinel", ic->clinel, cfg.clinel, rgb);

	/* background */
	hextorgb(cfg.cbg, rgb);
	ic->cbackground = gdImageColorAllocate(ic->im, rgb[0], rgb[1], rgb[2]);
	colorinitcheck("cbackground", ic->cbackground, cfg.cbg, rgb);
	modcolor(rgb, -35, 0);
	ic->cvnstat = gdImageColorAllocate(ic->im, rgb[0], rgb[1], rgb[2]);
	colorinitcheck("cvnstat", ic->cvnstat, cfg.cbg, rgb);
	hextorgb(cfg.cbg, rgb);
	modcolor(rgb, -15, 0);
	ic->cbgoffset = gdImageColorAllocate(ic->im, rgb[0], rgb[1], rgb[2]);
	colorinitcheck("cbgoffset", ic->cbgoffset, cfg.cbg, rgb);

	/* rx */
	hextorgb(cfg.crx, rgb);
	ic->crx = gdImageColorAllocate(ic->im, rgb[0], rgb[1], rgb[2]);
	colorinitcheck("crx", ic->crx, cfg.crx, rgb);
	if (cfg.crxd[0] == '-') {
		modcolor(rgb, -50, 1);
	} else {
		hextorgb(cfg.crxd, rgb);
	}
	ic->crxd = gdImageColorAllocate(ic->im, rgb[0], rgb[1], rgb[2]);
	colorinitcheck("crxd", ic->crxd, cfg.crxd, rgb);

	/* tx */
	hextorgb(cfg.ctx, rgb);
	ic->ctx = gdImageColorAllocate(ic->im, rgb[0], rgb[1], rgb[2]);
	colorinitcheck("ctx", ic->ctx, cfg.ctx, rgb);
	if (cfg.ctxd[0] == '-') {
		modcolor(rgb, -50, 1);
	} else {
		hextorgb(cfg.ctxd, rgb);
	}
	ic->ctxd = gdImageColorAllocate(ic->im, rgb[0], rgb[1], rgb[2]);
	colorinitcheck("ctxd", ic->ctxd, cfg.ctxd, rgb);
}

void colorinitcheck(const char *color, int value, const char *cfgtext, const int *rgb)
{
	if (value==-1) {
		printf("Error: ImageColorAllocate failed.\n");
		printf("       C: \"%s\" T: \"%s\" RGB: %d/%d/%d\n", color, cfgtext, rgb[0], rgb[1], rgb[2]);
		exit(EXIT_FAILURE);
	}
}

void layoutinit(IMAGECONTENT *ic, char *title, int width, int height)
{
	struct tm *d;
	char datestring[64];

	/* get time in given format */
	d = localtime(&data.lastupdated);
	strftime(datestring, 64, cfg.hformat, d);

	/* background, edges */
	gdImageFill(ic->im, 0, 0, ic->cbackground);
	if (ic->showedge) {
		gdImageRectangle(ic->im, 0, 0, width-1, height-1, ic->cedge);
	}

	/* titlebox with title */
	if (ic->showheader) {
		gdImageFilledRectangle(ic->im, 2+ic->showedge, 2+ic->showedge, width-3-ic->showedge, 24, ic->cheader);
		gdImageString(ic->im, gdFontGetGiant(), 12, 5+ic->showedge, (unsigned char*)title, ic->cheadertitle);
	}

	/* date */
	if (!ic->showheader || ic->altdate) {
		gdImageString(ic->im, gdFontGetTiny(), 5+ic->showedge, height-12-ic->showedge, (unsigned char*)datestring, ic->cvnstat);
	} else {
		gdImageString(ic->im, gdFontGetTiny(), width-(strlen(datestring)*gdFontGetTiny()->w+12), 9+ic->showedge, (unsigned char*)datestring, ic->cheaderdate);
	}

	/* generator */
	gdImageString(ic->im, gdFontGetTiny(), width-114-ic->showedge, height-12-ic->showedge, (unsigned char*)"vnStat / Teemu Toivola", ic->cvnstat);
}

void drawlegend(IMAGECONTENT *ic, int x, int y)
{
	if (!ic->showlegend) {
		return;
	}

	/* color legend */
	gdImageString(ic->im, gdFontGetSmall(), x, y, (unsigned char*)"rx     tx", ic->ctext);
	gdImageFilledRectangle(ic->im, x-12, y+4, x-6, y+10, ic->crx);
	gdImageRectangle(ic->im, x-12, y+4, x-6, y+10, ic->ctext);
	gdImageFilledRectangle(ic->im, x+30, y+4, x+36, y+10, ic->ctx);
	gdImageRectangle(ic->im, x+30, y+4, x+36, y+10, ic->ctext);
}

void drawbar(IMAGECONTENT *ic, int x, int y, int len, uint64_t rx, int rxk, uint64_t tx, int txk, uint64_t max)
{
	int l;

	rx=mbkbtokb(rx, rxk);
	tx=mbkbtokb(tx, txk);

	if ((rx+tx)!=max) {
		len=((rx+tx)/(float)max)*len;
	}

	if (len!=0) {

		if (tx>rx) {
			l=rintf((rx/(float)(rx+tx)*len));

			gdImageFilledRectangle(ic->im, x, y+YBEGINOFFSET, x+l, y+YENDOFFSET, ic->crx);
			gdImageRectangle(ic->im, x, y+YBEGINOFFSET, x+l, y+YENDOFFSET, ic->crxd);

			gdImageFilledRectangle(ic->im, x+l, y+YBEGINOFFSET, x+len, y+YENDOFFSET, ic->ctx);
			gdImageRectangle(ic->im, x+l, y+YBEGINOFFSET, x+len, y+YENDOFFSET, ic->ctxd);

		} else {
			l=rintf((tx/(float)(rx+tx)*len));

			gdImageFilledRectangle(ic->im, x, y+YBEGINOFFSET, x+(len-l), y+YENDOFFSET, ic->crx);
			gdImageRectangle(ic->im, x, y+YBEGINOFFSET, x+(len-l), y+YENDOFFSET, ic->crxd);

			gdImageFilledRectangle(ic->im, x+(len-l), y+YBEGINOFFSET, x+len, y+YENDOFFSET, ic->ctx);
			gdImageRectangle(ic->im, x+(len-l), y+YBEGINOFFSET, x+len, y+YENDOFFSET, ic->ctxd);
		}
	}
}

void drawpole(IMAGECONTENT *ic, int x, int y, int len, uint64_t rx, uint64_t tx, uint64_t max)
{
	int l;

	l = (rx/(float)max)*len;
	gdImageFilledRectangle(ic->im, x, y+(len-l), x+7, y+len, ic->crx);

	l = (tx/(float)max)*len;
	gdImageFilledRectangle(ic->im, x+5, y+(len-l), x+12, y+len, ic->ctx);
}

void drawdonut(IMAGECONTENT *ic, int x, int y, float rxp, float txp)
{
	int rxarc = 0, txarc = 0;

	if ( (int)(rxp + txp) > 0 ) {
		rxarc = 360 * (rxp / (float)100);
		if ( (int)(rxp + txp) == 100 ) {
			txarc = 360 - rxarc;
		} else {
			txarc = 360 * (txp / (float)100);
		}

		/* fix possible graphical glitch */
		if (!rxarc) {
			rxarc = 1;
		}

		if (!txarc) {
			txarc = 1;
		}
	}

	gdImageFilledArc(ic->im, x, y, DOUTRAD, DOUTRAD, 0, 360, ic->cbgoffset, 0);

	if ( (int)(rxp + txp) > 0 ) {
		gdImageFilledArc(ic->im, x, y, DOUTRAD, DOUTRAD, 270, 270+txarc, ic->ctx, 0);
		gdImageFilledArc(ic->im, x, y, DOUTRAD, DOUTRAD, 270, 270+txarc, ic->ctxd, gdEdged|gdNoFill);
		gdImageFilledArc(ic->im, x, y, DOUTRAD, DOUTRAD, 270+txarc, 270+txarc+rxarc, ic->crx, 0);
		gdImageFilledArc(ic->im, x, y, DOUTRAD, DOUTRAD, 270+txarc, 270+txarc+rxarc, ic->crxd, gdEdged|gdNoFill);

		gdImageFilledArc(ic->im, x, y, DINRAD, DINRAD, 270, 270+txarc, ic->ctxd, gdEdged|gdNoFill);
		gdImageFilledArc(ic->im, x, y, DINRAD, DINRAD, 270+txarc, 270+txarc+rxarc, ic->crxd, gdEdged|gdNoFill);
	}

	gdImageFilledArc(ic->im, x, y, DINRAD-2, DINRAD-2, 0, 360, ic->cbackground, 0);
}

void drawhours(IMAGECONTENT *ic, int x, int y, int rate)
{
	int i, tmax=0, s=0, step, prev=0, diff=0, chour;
	float ratediv;
	uint64_t max=1, scaleunit=0;
	char buffer[32];
	struct tm *d;

	ic->current = time(NULL);
	chour = localtime(&ic->current)->tm_hour;

	if (cfg.rateunit) {
		ratediv = 450;      /* x * 8 / 3600 */
	} else {
		ratediv = 3600;
	}

	/* tmax (time max) = current hour */
	/* max = transfer max */

	for (i = 0; i < 24; i++) {

		/* convert hourly transfer to hourly rate if needed */
		if (rate) {
			if ((ic->current-data.hour[i].date) > 3600) {
				data.hour[i].rx = data.hour[i].rx / ratediv;
				data.hour[i].tx = data.hour[i].tx / ratediv;
			} else {
				/* scale ongoing hour properly */
				d = localtime(&data.hour[i].date);
				if (chour != d->tm_hour) {
					data.hour[i].rx = data.hour[i].rx / ratediv;
					data.hour[i].tx = data.hour[i].tx / ratediv;
				} else {
					diff = d->tm_min*60;
					if (!diff) {
						diff = 1;
					}
					if (cfg.rateunit==1) {
						data.hour[i].rx = data.hour[i].rx * 8 / (float)diff;
						data.hour[i].tx = data.hour[i].tx * 8 / (float)diff;
					} else {
						data.hour[i].rx = data.hour[i].rx / (float)diff;
						data.hour[i].tx = data.hour[i].tx / (float)diff;
					}
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

	for (i=step; (uint64_t)(scaleunit*i) <= max; i=i+step) {
		s = 121 * ((scaleunit * i) / (float)max);
		gdImageLine(ic->im, x+36, y+124-s, x+460, y+124-s, ic->cline);
		gdImageLine(ic->im, x+36, y+124-((s+prev)/2), x+460, y+124-((s+prev)/2), ic->clinel);
		gdImageString(ic->im, gdFontGetTiny(), x+16, y+121-s, (unsigned char*)getimagevalue(scaleunit*i, 3, rate), ic->ctext);
		prev = s;
	}
	s = 121 * ((scaleunit * i) / (float)max);
	if ( ((s+prev)/2) <= 128 ) {
		gdImageLine(ic->im, x+36, y+124-((s+prev)/2), x+460, y+124-((s+prev)/2), ic->clinel);
	}

	/* scale text */
	if (rate) {
		gdImageStringUp(ic->im, gdFontGetTiny(), x-2, y+70, (unsigned char*)getimagescale(scaleunit, 1), ic->ctext);
	} else {
		gdImageStringUp(ic->im, gdFontGetTiny(), x-2, y+60, (unsigned char*)getimagescale(scaleunit, 0), ic->ctext);
	}

	/* x-axis values and poles */
	for (i = 0; i < 24; i++) {
		s=tmax-i;
		if (s<0) {
			s+=24;
		}
		snprintf(buffer, 32, "%02d ", s);
		gdImageString(ic->im, gdFontGetTiny(), x+440-(i*17), y+128, (unsigned char*)buffer, ic->ctext);
		drawpole(ic, x+438-(i*17), y, 124, data.hour[s].rx, data.hour[s].tx, max);
	}

	/* axis */
	gdImageLine(ic->im, x+36-4, y+124, x+466, y+124, ic->ctext);
	gdImageLine(ic->im, x+36, y-10, x+36, y+124+4, ic->ctext);

	/* arrows */
	gdImageLine(ic->im, x+465, y+124, x+462, y+122, ic->ctext);
	gdImageLine(ic->im, x+465, y+124, x+462, y+126, ic->ctext);
	gdImageLine(ic->im, x+462, y+122, x+462, y+126, ic->ctext);
	gdImageLine(ic->im, x+36, y-9, x+38, y-6, ic->ctext);
	gdImageLine(ic->im, x+36, y-9, x+34, y-6, ic->ctext);
	gdImageLine(ic->im, x+34, y-6, x+38, y-6, ic->ctext);

}

void drawsummary(IMAGECONTENT *ic, int type, int rate)
{
	int textx, texty, offset = 0;
	int width, height, headermod;
	float rxp = 50, txp = 50, mod;
	char buffer[512], datebuff[16], daytemp[32];
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

	if (!ic->showheader) {
		headermod = 26;
		height -= 22;
	} else {
		headermod = 0;
	}

	yesterday=ic->current-86400;

	ic->im = gdImageCreate(width, height);

	colorinit(ic);

	if (strlen(ic->headertext)) {
		strncpy_nt(buffer, ic->headertext, 65);
	} else {
		if (strcmp(data.nick, data.interface)==0) {
			snprintf(buffer, 512, "%s", data.interface);
		} else {
			snprintf(buffer, 512, "%s (%s)", data.nick, data.interface);
		}
	}

	layoutinit(ic, buffer, width, height);

	/* today */
	if (data.day[0].rx>1024 || data.day[0].tx>1024)	{
		rxp = (data.day[0].rx/(float)(data.day[0].rx+data.day[0].tx))*100;
		txp = (float)100 - rxp;
	} else {
		if ( (data.day[0].rx*+data.day[0].rxk)+(data.day[0].tx*+data.day[0].txk) == 0 ) {
			rxp = txp = 0;
		} else {
			rxp = ( ((data.day[0].rx*1024)+data.day[0].rxk) / (float)(((data.day[0].rx*1024)+data.day[0].rxk)+((data.day[0].tx*1024)+data.day[0].txk)) )*100;
			txp = (float)100 - rxp;
		}
	}

	/* do scaling if needed */
	if ( (data.day[0].rx+data.day[0].tx) < (data.day[1].rx+data.day[1].tx) ) {
		if ( (data.day[0].rx+data.day[0].tx)>1024 || (data.day[1].rx+data.day[1].tx)>1024 ) {
			mod = (data.day[0].rx+data.day[0].tx) / (float)(data.day[1].rx+data.day[1].tx);
		} else {
			mod = (((data.day[0].rx*1024)+data.day[0].rxk)+((data.day[0].tx*1024)+data.day[0].txk)) / (float)(((data.day[1].rx*1024)+data.day[1].rxk)+((data.day[1].tx*1024)+data.day[1].txk));
		}
		rxp = rxp * mod;
		txp = txp * mod;
	}

	/* move graph to center if there's only one to draw for this line */
	if (!data.day[1].date) {
		offset = 85;
	} else {
		offset = 0;
	}

	drawdonut(ic, 150+offset, 75-headermod, rxp, txp);

	textx = 100+offset;
	texty = 30-headermod;

	/* get formated date for today */
	d = localtime(&ic->current);
	strftime(datebuff, 16, cfg.dformat, d);

	/* get formated date for current day in database */
	d = localtime(&data.day[0].date);
	strftime(daytemp, 16, cfg.dformat, d);

	/* change daytemp to today if formated days match */
	if (strcmp(datebuff, daytemp)==0) {
		strncpy_nt(daytemp, "today", 32);
	}

	snprintf(buffer, 32, "%*s", getpadding(12, daytemp), daytemp);
	gdImageString(ic->im, gdFontGetLarge(), textx-54, texty-1, (unsigned char*)buffer, ic->ctext);

	if (cfg.summaryrate) {
		d = localtime(&data.lastupdated);
		snprintf(buffer, 16, "%15s", getrate(data.day[0].rx+data.day[0].tx, data.day[0].rxk+data.day[0].txk, d->tm_sec+(d->tm_min*60)+(d->tm_hour*3600), 15));
		gdImageString(ic->im, gdFontGetSmall(), textx-74, texty+58, (unsigned char*)buffer, ic->ctext);
	} else {
		texty += 7;
	}

	snprintf(buffer, 4, "rx ");
	strncat(buffer, getvalue(data.day[0].rx, data.day[0].rxk, 12, 1), 32);
	gdImageString(ic->im, gdFontGetSmall(), textx-74, texty+18, (unsigned char*)buffer, ic->ctext);
	snprintf(buffer, 4, "tx ");
	strncat(buffer, getvalue(data.day[0].tx, data.day[0].txk, 12, 1), 32);
	gdImageString(ic->im, gdFontGetSmall(), textx-74, texty+30, (unsigned char*)buffer, ic->ctext);
	snprintf(buffer, 4, " = ");
	strncat(buffer, getvalue(data.day[0].rx+data.day[0].tx, data.day[0].rxk+data.day[0].txk, 12, 1), 32);
	gdImageString(ic->im, gdFontGetSmall(), textx-74, texty+44, (unsigned char*)buffer, ic->ctext);


	/* yesterday */
	if (data.day[1].date) {
		if (data.day[1].rx>1024 || data.day[1].tx>1024)	{
			rxp = (data.day[1].rx/(float)(data.day[1].rx+data.day[1].tx))*100;
			txp = (float)100 - rxp;
		} else {
			if ( (data.day[1].rx*+data.day[1].rxk)+(data.day[1].tx*+data.day[1].txk) == 0 ) {
				rxp = txp = 0;
			} else {
				rxp = ( ((data.day[1].rx*1024)+data.day[1].rxk) / (float)(((data.day[1].rx*1024)+data.day[1].rxk)+((data.day[1].tx*1024)+data.day[1].txk)) )*100;
				txp = (float)100 - rxp;
			}
		}

		/* do scaling if needed */
		if ( (data.day[1].rx+data.day[1].tx) < (data.day[0].rx+data.day[0].tx) ) {
			if ( (data.day[1].rx+data.day[1].tx)>1024 || (data.day[0].rx+data.day[0].tx)>1024 ) {
				mod = (data.day[1].rx+data.day[1].tx) / (float)(data.day[0].rx+data.day[0].tx);
			} else {
				mod = (((data.day[1].rx*1024)+data.day[1].rxk)+((data.day[1].tx*1024)+data.day[1].txk)) / (float)(((data.day[0].rx*1024)+data.day[0].rxk)+((data.day[0].tx*1024)+data.day[0].txk));
			}
			rxp = rxp * mod;
			txp = txp * mod;
		}

		drawdonut(ic, 330, 75-headermod, rxp, txp);

		textx = 280;
		texty = 30-headermod;

		/* get formated date for yesterday */
		d = localtime(&yesterday);
		strftime(datebuff, 16, cfg.dformat, d);

		/* get formated date for previous day in database */
		d = localtime(&data.day[1].date);
		strftime(daytemp, 16, cfg.dformat, d);

		/* change daytemp to yesterday if formated days match */
		if (strcmp(datebuff, daytemp)==0) {
			strncpy_nt(daytemp, "yesterday", 32);
		}

		snprintf(buffer, 32, "%*s", getpadding(12, daytemp), daytemp);
		gdImageString(ic->im, gdFontGetLarge(), textx-54, texty-1, (unsigned char*)buffer, ic->ctext);

		if (cfg.summaryrate) {
			snprintf(buffer, 16, "%15s", getrate(data.day[1].rx+data.day[1].tx, data.day[1].rxk+data.day[1].txk, 86400, 15));
			gdImageString(ic->im, gdFontGetSmall(), textx-74, texty+58, (unsigned char*)buffer, ic->ctext);
		} else {
			texty += 7;
		}

		snprintf(buffer, 4, "rx ");
		strncat(buffer, getvalue(data.day[1].rx, data.day[1].rxk, 12, 1), 32);
		gdImageString(ic->im, gdFontGetSmall(), textx-74, texty+18, (unsigned char*)buffer, ic->ctext);
		snprintf(buffer, 4, "tx ");
		strncat(buffer, getvalue(data.day[1].tx, data.day[1].txk, 12, 1), 32);
		gdImageString(ic->im, gdFontGetSmall(), textx-74, texty+30, (unsigned char*)buffer, ic->ctext);
		snprintf(buffer, 4, " = ");
		strncat(buffer, getvalue(data.day[1].rx+data.day[1].tx, data.day[1].rxk+data.day[1].txk, 12, 1), 32);
		gdImageString(ic->im, gdFontGetSmall(), textx-74, texty+44, (unsigned char*)buffer, ic->ctext);
	}

	/* current month */
	if (data.month[0].rx>1024 || data.month[0].tx>1024)	{
		rxp = (data.month[0].rx/(float)(data.month[0].rx+data.month[0].tx))*100;
		txp = (float)100 - rxp;
	} else {
		if ( (data.month[0].rx+data.month[0].rxk)+(data.month[0].tx+data.month[0].txk) == 0 ) {
			rxp = txp = 0;
		} else {
			rxp = ( ((data.month[0].rx*1024)+data.month[0].rxk) / (float)(((data.month[0].rx*1024)+data.month[0].rxk)+((data.month[0].tx*1024)+data.month[0].txk)) )*100;
			txp = (float)100 - rxp;
		}
	}


	/* do scaling if needed */
	if ( (data.month[0].rx+data.month[0].tx) < (data.month[1].rx+data.month[1].tx) ) {
		if ( (data.month[0].rx+data.month[0].tx)>1024 || (data.month[1].rx+data.month[1].tx)>1024 ) {
			mod = (data.month[0].rx+data.month[0].tx) / (float)(data.month[1].rx+data.month[1].tx);
		} else {
			mod = (((data.month[0].rx*1024)+data.month[0].rxk)+((data.month[0].tx*1024)+data.month[0].txk)) / (float)(((data.month[1].rx*1024)+data.month[1].rxk)+((data.month[1].tx*1024)+data.month[1].txk));
		}
		rxp = rxp * mod;
		txp = txp * mod;
	}

	/* move graph to center if there's only one to draw for this line */
	if (!data.month[1].month) {
		offset = 85;
	} else {
		offset = 0;
	}

	drawdonut(ic, 150+offset, 163-headermod, rxp, txp);

	textx = 100+offset;
	texty = 118-headermod;

	d = localtime(&data.month[0].month);
	strftime(daytemp, 16, cfg.mformat, d);

	snprintf(buffer, 32, "%*s", getpadding(12, daytemp), daytemp);
	gdImageString(ic->im, gdFontGetLarge(), textx-54, texty-1, (unsigned char*)buffer, ic->ctext);

	if (cfg.summaryrate) {
		snprintf(buffer, 16, "%15s", getrate(data.month[0].rx+data.month[0].tx, data.month[0].rxk+data.month[0].txk, mosecs(), 15));
		gdImageString(ic->im, gdFontGetSmall(), textx-74, texty+58, (unsigned char*)buffer, ic->ctext);
	} else {
		texty += 7;
	}

	snprintf(buffer, 4, "rx ");
	strncat(buffer, getvalue(data.month[0].rx, data.month[0].rxk, 12, 1), 32);
	gdImageString(ic->im, gdFontGetSmall(), textx-74, texty+18, (unsigned char*)buffer, ic->ctext);
	snprintf(buffer, 4, "tx ");
	strncat(buffer, getvalue(data.month[0].tx, data.month[0].txk, 12, 1), 32);
	gdImageString(ic->im, gdFontGetSmall(), textx-74, texty+30, (unsigned char*)buffer, ic->ctext);
	snprintf(buffer, 4, " = ");
	strncat(buffer, getvalue(data.month[0].rx+data.month[0].tx, data.month[0].rxk+data.month[0].txk, 12, 1), 32);
	gdImageString(ic->im, gdFontGetSmall(), textx-74, texty+44, (unsigned char*)buffer, ic->ctext);


	/* previous month */
	if (data.month[1].month) {
		if (data.month[1].rx>1024 || data.month[1].tx>1024)	{
			rxp = (data.month[1].rx/(float)(data.month[1].rx+data.month[1].tx))*100;
			txp = (float)100 - rxp;
		} else {
			if ( (data.month[1].rx+data.month[1].rxk)+(data.month[1].tx+data.month[1].txk) == 0 ) {
				rxp = txp = 0;
			} else {
				rxp = ( ((data.month[1].rx*1024)+data.month[1].rxk) / (float)(((data.month[1].rx*1024)+data.month[1].rxk)+((data.month[1].tx*1024)+data.month[1].txk)) )*100;
				txp = (float)100 - rxp;
			}
		}

		/* do scaling if needed */
		if ( (data.month[1].rx+data.month[1].tx) < (data.month[0].rx+data.month[0].tx) ) {
			if ( (data.month[1].rx+data.month[1].tx)>1024 || (data.month[0].rx+data.month[0].tx)>1024 ) {
				mod = (data.month[1].rx+data.month[1].tx) / (float)(data.month[0].rx+data.month[0].tx);
			} else {
				mod = (((data.month[1].rx*1024)+data.month[1].rxk)+((data.month[1].tx*1024)+data.month[1].txk)) / (float)(((data.month[0].rx*1024)+data.month[0].rxk)+((data.month[0].tx*1024)+data.month[0].txk));
			}
			rxp = rxp * mod;
			txp = txp * mod;
		}

		drawdonut(ic, 330, 163-headermod, rxp, txp);

		textx = 280;
		texty = 118-headermod;

		d = localtime(&data.month[1].month);
		strftime(daytemp, 16, cfg.mformat, d);

		snprintf(buffer, 32, "%*s", getpadding(12, daytemp), daytemp);
		gdImageString(ic->im, gdFontGetLarge(), textx-54, texty-1, (unsigned char*)buffer, ic->ctext);

		if (cfg.summaryrate) {
			snprintf(buffer, 16, "%15s", getrate(data.month[1].rx+data.month[1].tx, data.month[1].rxk+data.month[1].txk, dmonth(d->tm_mon)*86400, 15));
			gdImageString(ic->im, gdFontGetSmall(), textx-74, texty+58, (unsigned char*)buffer, ic->ctext);
		} else {
			texty += 7;
		}

		snprintf(buffer, 4, "rx ");
		strncat(buffer, getvalue(data.month[1].rx, data.month[1].rxk, 12, 1), 32);
		gdImageString(ic->im, gdFontGetSmall(), textx-74, texty+18, (unsigned char*)buffer, ic->ctext);
		snprintf(buffer, 4, "tx ");
		strncat(buffer, getvalue(data.month[1].tx, data.month[1].txk, 12, 1), 32);
		gdImageString(ic->im, gdFontGetSmall(), textx-74, texty+30, (unsigned char*)buffer, ic->ctext);
		snprintf(buffer, 4, " = ");
		strncat(buffer, getvalue(data.month[1].rx+data.month[1].tx, data.month[1].rxk+data.month[1].txk, 12, 1), 32);
		gdImageString(ic->im, gdFontGetSmall(), textx-74, texty+44, (unsigned char*)buffer, ic->ctext);
	}

	/* all time */
	textx = 385;
	texty = 57-headermod;

	gdImageString(ic->im, gdFontGetLarge(), textx+12, texty, (unsigned char*)"all time", ic->ctext);
	snprintf(buffer, 4, "rx ");
	strncat(buffer, getvalue(data.totalrx, data.totalrxk, 12, 1), 32);
	gdImageString(ic->im, gdFontGetSmall(), textx, texty+24, (unsigned char*)buffer, ic->ctext);
	snprintf(buffer, 4, "tx ");
	strncat(buffer, getvalue(data.totaltx, data.totaltxk, 12, 1), 32);
	gdImageString(ic->im, gdFontGetSmall(), textx, texty+36, (unsigned char*)buffer, ic->ctext);
	snprintf(buffer, 4, " = ");
	strncat(buffer, getvalue(data.totalrx+data.totaltx, data.totalrxk+data.totaltxk, 12, 1), 32);
	gdImageString(ic->im, gdFontGetSmall(), textx, texty+50, (unsigned char*)buffer, ic->ctext);
	d = localtime(&data.created);
	strftime(datebuff, 16, cfg.tformat, d);
	snprintf(daytemp, 7, "since ");
	strncat(daytemp, datebuff, 16);
	snprintf(buffer, 20, "%19s", daytemp);
	gdImageString(ic->im, gdFontGetSmall(), textx-24, texty+70, (unsigned char*)buffer, ic->ctext);

	drawlegend(ic, 410, 155-headermod);

	/* hours if requested */
	switch (type) {
		case 1:
			drawhours(ic, 500, 46-headermod, rate);
			break;
		case 2:
			drawhours(ic, 6, 215-headermod, rate);
			break;
		default:
			break;
	}

}

void drawoldsummary(IMAGECONTENT *ic, int type, int rate)
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

	if (!ic->showheader) {
		headermod = 26;
		height -= 22;
	} else {
		headermod = 0;
	}

	yesterday=ic->current-86400;

	ic->im = gdImageCreate(width, height);

	colorinit(ic);

	if (strcmp(data.nick, data.interface)==0) {
		snprintf(buffer, 512, "%s", data.interface);
	} else {
		snprintf(buffer, 512, "%s (%s)", data.nick, data.interface);
	}

	layoutinit(ic, buffer, width, height);
	drawlegend(ic, 383, 110-headermod);

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
		gdImageFilledArc(ic->im, piex, piey+i, piew, pieh, 270, 270+arc, ic->ctxd, gdEdged|gdNoFill);
		gdImageFilledArc(ic->im, piex, piey+i, piew, pieh, 270+arc, 270, ic->crxd, gdEdged|gdNoFill);
	}

	gdImageFilledArc(ic->im, piex, piey, piew, pieh, 270, 270+arc, ic->ctx, 0);
	gdImageFilledArc(ic->im, piex, piey, piew, pieh, 270, 270+arc, ic->ctxd, gdEdged|gdNoFill);
	gdImageFilledArc(ic->im, piex, piey, piew, pieh, 270+arc, 270, ic->crx, 0);
	gdImageFilledArc(ic->im, piex, piey, piew, pieh, 270+arc, 270, ic->crxd, gdEdged|gdNoFill);

	textx = 30;
	texty = 48-headermod;

	/* totals */
	snprintf(buffer, 512, "   received: %s  (%.1f%%)", getvalue(data.totalrx, data.totalrxk, 14, 1), rxp);
	gdImageString(ic->im, gdFontGetLarge(), textx, texty, (unsigned char*)buffer, ic->ctext);
	snprintf(buffer, 512, "transmitted: %s  (%.1f%%)", getvalue(data.totaltx, data.totaltxk, 14, 1), txp);
	gdImageString(ic->im, gdFontGetLarge(), textx, texty+15, (unsigned char*)buffer, ic->ctext);
	snprintf(buffer, 512, "      total: %s", getvalue(data.totalrx+data.totaltx, data.totalrxk+data.totaltxk, 14, 1));
	gdImageString(ic->im, gdFontGetLarge(), textx, texty+30, (unsigned char*)buffer, ic->ctext);

	/* get formated date for yesterday */
	d=localtime(&yesterday);
	strftime(datebuff, 16, cfg.dformat, d);

	/* get formated date for previous day in database */
	d=localtime(&data.day[1].date);
	strftime(daytemp, 16, cfg.dformat, d);

	/* change daytemp to yesterday if formated days match */
	if (strcmp(datebuff, daytemp)==0) {
		strncpy_nt(daytemp, "yesterday", 32);
	}

	/* get formated date for today */
	d=localtime(&ic->current);
	strftime(datebuff, 16, cfg.dformat, d);

	/* get formated date for current day in database */
	d=localtime(&data.day[0].date);
	strftime(daytemp2, 16, cfg.dformat, d);

	/* change daytemp to today if formated days match */
	if (strcmp(datebuff, daytemp2)==0) {
		strncpy_nt(daytemp2, "today", 32);
	}

	textx = 20;
	texty = 118-headermod;

	/* yesterday & today */
	gdImageString(ic->im, gdFontGetSmall(), textx, texty, (unsigned char*)"                rx           tx        total", ic->ctext);
	gdImageLine(ic->im, textx-4, texty+16, textx+290, texty+16, ic->cline);
	gdImageLine(ic->im, textx-4, texty+49, textx+290, texty+49, ic->cline);
	gdImageLine(ic->im, textx+140, texty+4, textx+140, texty+64, ic->cline);
	gdImageLine(ic->im, textx+218, texty+4, textx+218, texty+64, ic->cline);

	if (data.day[1].date!=0) {
		snprintf(buffer, 32, "%*s   ", getpadding(9, daytemp), daytemp);
		strncat(buffer, getvalue(data.day[1].rx, data.day[1].rxk, 10, 1), 32);
		strcat(buffer, "   ");
		strncat(buffer, getvalue(data.day[1].tx, data.day[1].txk, 10, 1), 32);
		strcat(buffer, "   ");
		strncat(buffer, getvalue(data.day[1].rx+data.day[1].tx, data.day[1].rxk+data.day[1].txk, 10, 1), 32);
		gdImageString(ic->im, gdFontGetSmall(), textx, texty+20, (unsigned char*)buffer, ic->ctext);
	}

	snprintf(buffer, 32, "%*s   ", getpadding(9, daytemp2), daytemp2);
	strncat(buffer, getvalue(data.day[0].rx, data.day[0].rxk, 10, 1), 32);
	strcat(buffer, "   ");
	strncat(buffer, getvalue(data.day[0].tx, data.day[0].txk, 10, 1), 32);
	strcat(buffer, "   ");
	strncat(buffer, getvalue(data.day[0].rx+data.day[0].tx, data.day[0].rxk+data.day[0].txk, 10, 1), 32);
	gdImageString(ic->im, gdFontGetSmall(), textx, texty+32, (unsigned char*)buffer, ic->ctext);

	snprintf(buffer, 32, "estimated   ");
	strncat(buffer, getvalue(e_rx, 0, 10, 2), 32);
	strcat(buffer, "   ");
	strncat(buffer, getvalue(e_tx, 0, 10, 2), 32);
	strcat(buffer, "   ");
	strncat(buffer, getvalue(e_rx+e_tx, 0, 10, 2), 32);
	gdImageString(ic->im, gdFontGetSmall(), textx, texty+52, (unsigned char*)buffer, ic->ctext);

	/* search maximum */
	max=1;
	for (i = 1; i >= 0; i--) {
		if (data.day[i].used) {

			t=data.day[i].rx+data.day[i].tx;
			tk=data.day[i].rxk+data.day[i].txk;

			t=mbkbtokb(t, tk);

			if (t>max) {
				max=t;
			}
		}
	}

	/* bars for both */
	drawbar(ic, textx+300, texty+24, 165, data.day[1].rx, data.day[1].rxk, data.day[1].tx, data.day[1].txk, max);
	drawbar(ic, textx+300, texty+36, 165, data.day[0].rx, data.day[0].rxk, data.day[0].tx, data.day[0].txk, max);

	/* hours if requested */
	switch (type) {
		case 1:
			drawhours(ic, 500, 46-headermod, rate);
			break;
		case 2:
			drawhours(ic, 16, 215-headermod, rate);
			break;
		default:
			break;
	}

}

void drawhourly(IMAGECONTENT *ic, int rate)
{
	int width, height, headermod;
	char buffer[512];

	width = 500;
	height = 200;

	if (!ic->showheader) {
		headermod = 26;
		height -= 22;
	} else {
		headermod = 0;
	}

	ic->im = gdImageCreate(width, height);

	colorinit(ic);

	if (strcmp(data.nick, data.interface)==0) {
		snprintf(buffer, 512, "%s / hourly", data.interface);
	} else {
		snprintf(buffer, 512, "%s (%s) / hourly", data.nick, data.interface);
	}

	layoutinit(ic, buffer, width, height);
	drawlegend(ic, 242, 183-headermod);
	drawhours(ic, 12, 46-headermod, rate);
}

void drawdaily(IMAGECONTENT *ic)
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

	if (!ic->showheader) {
		headermod = 26;
		height -= 22;
	} else {
		headermod = 0;
	}

	ic->im = gdImageCreate(width, height);

	colorinit(ic);

	if (strcmp(data.nick, data.interface)==0) {
		snprintf(buffer, 512, "%s / daily", data.interface);
	} else {
		snprintf(buffer, 512, "%s (%s) / daily", data.nick, data.interface);
	}

	layoutinit(ic, buffer, width, height);

	if (lines) {
		if (cfg.ostyle>2) {
			drawlegend(ic, 432, 40-headermod);
		} else {
			drawlegend(ic, 385, 40-headermod);
		}
	}

	textx = 10;
	texty = 40-headermod;

	if (cfg.ostyle>2) {
		gdImageString(ic->im, gdFontGetSmall(), textx, texty, (unsigned char*)"     day        rx           tx          total       avg. rate", ic->ctext);
		gdImageLine(ic->im, textx+2, texty+16, textx+392, texty+16, ic->cline);
		gdImageLine(ic->im, textx+144, texty+2, textx+144, texty+40+(12*lines), ic->cline);
		gdImageLine(ic->im, textx+222, texty+2, textx+222, texty+40+(12*lines), ic->cline);
		gdImageLine(ic->im, textx+300, texty+2, textx+300, texty+40+(12*lines), ic->cline);
	} else {
		gdImageString(ic->im, gdFontGetSmall(), textx, texty, (unsigned char*)"     day        rx           tx          total", ic->ctext);
		gdImageLine(ic->im, textx+2, texty+16, textx+296, texty+16, ic->cline);
		gdImageLine(ic->im, textx+144, texty+2, textx+144, texty+40+(12*lines), ic->cline);
		gdImageLine(ic->im, textx+222, texty+2, textx+222, texty+40+(12*lines), ic->cline);
	}

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
			if (strftime(datebuff, 16, cfg.dformat, d)<=8) {
				snprintf(buffer, 32, "  %*s   ", getpadding(8, datebuff), datebuff);
			} else {
				snprintf(buffer, 32, " %-*s ", getpadding(11, datebuff), datebuff);
			}
			strncat(buffer, getvalue(data.day[i].rx, data.day[i].rxk, 10, 1), 32);
			strcat(buffer, "   ");
			strncat(buffer, getvalue(data.day[i].tx, data.day[i].txk, 10, 1), 32);
			strcat(buffer, "   ");
			strncat(buffer, getvalue(data.day[i].rx+data.day[i].tx, data.day[i].rxk+data.day[i].txk, 10, 1), 32);
			if (cfg.ostyle>2) {
				strcat(buffer, "  ");
				if (i==0) {
					d = localtime(&data.lastupdated);
					strncat(buffer, getrate(data.day[i].rx+data.day[i].tx, data.day[i].rxk+data.day[i].txk, d->tm_sec+(d->tm_min*60)+(d->tm_hour*3600), 14), 32);
				} else {
					strncat(buffer, getrate(data.day[i].rx+data.day[i].tx, data.day[i].rxk+data.day[i].txk, 86400, 14), 32);
				}
			}
			gdImageString(ic->im, gdFontGetSmall(), textx, texty, (unsigned char*)buffer, ic->ctext);
			if (cfg.ostyle>2) {
				drawbar(ic, textx+400, texty+4, 78, data.day[i].rx, data.day[i].rxk, data.day[i].tx, data.day[i].txk, max);
			} else {
				drawbar(ic, textx+304, texty+4, 170, data.day[i].rx, data.day[i].rxk, data.day[i].tx, data.day[i].txk, max);
			}
			texty += 12;
		}
	}

	if (lines==0) {
		gdImageString(ic->im, gdFontGetSmall(), textx, texty, (unsigned char*)"                 no data available", ic->ctext);
		texty += 12;
	}

	if (cfg.ostyle>2) {
		gdImageLine(ic->im, textx+2, texty+5, textx+392, texty+5, ic->cline);
	} else {
		gdImageLine(ic->im, textx+2, texty+5, textx+296, texty+5, ic->cline);
	}

	if (lines) {

		d=localtime(&data.lastupdated);
		if ( data.day[0].rx==0 || data.day[0].tx==0 || (d->tm_hour*60+d->tm_min)==0 ) {
			e_rx=e_tx=0;
		} else {
			e_rx=((data.day[0].rx)/(float)(d->tm_hour*60+d->tm_min))*1440;
			e_tx=((data.day[0].tx)/(float)(d->tm_hour*60+d->tm_min))*1440;
		}
		snprintf(buffer, 32, " estimated   ");
		strncat(buffer, getvalue(e_rx, 0, 10, 2), 32);
		strcat(buffer, "   ");
		strncat(buffer, getvalue(e_tx, 0, 10, 2), 32);
		strcat(buffer, "   ");
		strncat(buffer, getvalue(e_rx+e_tx, 0, 10, 2), 32);

		gdImageString(ic->im, gdFontGetSmall(), textx, texty+8, (unsigned char*)buffer, ic->ctext);
	}

}

void drawmonthly(IMAGECONTENT *ic)
{
	int textx, texty, lines;
	int i, tk, width, height, headermod;
	uint64_t t, max, e_rx, e_tx;
	char buffer[512], datebuff[16];
	struct tm *d;

	/* count how many months needs to be shown */
	lines = 0;
	for (i = 0; i < 12; i++) {
		if (data.month[i].used) {
			lines++;
		}
	}

	width = 500;
	height = 98 + (12 * lines);

	if (!ic->showheader) {
		headermod = 26;
		height -= 22;
	} else {
		headermod = 0;
	}

	ic->im = gdImageCreate(width, height);

	colorinit(ic);

	if (strcmp(data.nick, data.interface)==0) {
		snprintf(buffer, 512, "%s / monthly", data.interface);
	} else {
		snprintf(buffer, 512, "%s (%s) / monthly", data.nick, data.interface);
	}

	layoutinit(ic, buffer, width, height);

	if (lines) {
		if (cfg.ostyle>2) {
			drawlegend(ic, 432, 40-headermod);
		} else {
			drawlegend(ic, 385, 40-headermod);
		}
	}

	textx = 10;
	texty = 40-headermod;

	if (cfg.ostyle>2) {
		gdImageString(ic->im, gdFontGetSmall(), textx, texty, (unsigned char*)"    month       rx           tx          total       avg. rate", ic->ctext);
		gdImageLine(ic->im, textx+2, texty+16, textx+392, texty+16, ic->cline);
		gdImageLine(ic->im, textx+144, texty+2, textx+144, texty+40+(12*lines), ic->cline);
		gdImageLine(ic->im, textx+222, texty+2, textx+222, texty+40+(12*lines), ic->cline);
		gdImageLine(ic->im, textx+300, texty+2, textx+300, texty+40+(12*lines), ic->cline);
	} else {
		gdImageString(ic->im, gdFontGetSmall(), textx, texty, (unsigned char*)"    month       rx           tx          total", ic->ctext);
		gdImageLine(ic->im, textx+2, texty+16, textx+296, texty+16, ic->cline);
		gdImageLine(ic->im, textx+144, texty+2, textx+144, texty+40+(12*lines), ic->cline);
		gdImageLine(ic->im, textx+222, texty+2, textx+222, texty+40+(12*lines), ic->cline);
	}


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
			if (strftime(datebuff, 16, cfg.mformat, d)<=9) {
				snprintf(buffer, 32, " %*s   ", getpadding(9, datebuff), datebuff);
			} else {
				snprintf(buffer, 32, " %-*s ", getpadding(11, datebuff), datebuff);
			}
			strncat(buffer, getvalue(data.month[i].rx, data.month[i].rxk, 10, 1), 32);
			strcat(buffer, "   ");
			strncat(buffer, getvalue(data.month[i].tx, data.month[i].txk, 10, 1), 32);
			strcat(buffer, "   ");
			strncat(buffer, getvalue(data.month[i].rx+data.month[i].tx, data.month[i].rxk+data.month[i].txk, 10, 1), 32);
			if (cfg.ostyle>2) {
				strcat(buffer, "  ");
				if (i==0) {
					strncat(buffer, getrate(data.month[0].rx+data.month[0].tx, data.month[0].rxk+data.month[0].txk, mosecs(), 14), 32);
				} else {
					strncat(buffer, getrate(data.month[i].rx+data.month[i].tx, data.month[i].rxk+data.month[i].txk, dmonth(d->tm_mon)*86400, 14), 32);
				}
			}
			gdImageString(ic->im, gdFontGetSmall(), textx, texty, (unsigned char*)buffer, ic->ctext);
			if (cfg.ostyle>2) {
				drawbar(ic, textx+400, texty+4, 78, data.month[i].rx, data.month[i].rxk, data.month[i].tx, data.month[i].txk, max);
			} else {
				drawbar(ic, textx+304, texty+4, 170, data.month[i].rx, data.month[i].rxk, data.month[i].tx, data.month[i].txk, max);
			}
			texty += 12;
		}
	}

	if (lines==0) {
		gdImageString(ic->im, gdFontGetSmall(), textx, texty, (unsigned char*)"                 no data available", ic->ctext);
		texty += 12;
	}

	if (cfg.ostyle>2) {
		gdImageLine(ic->im, textx+2, texty+5, textx+392, texty+5, ic->cline);
	} else {
		gdImageLine(ic->im, textx+2, texty+5, textx+296, texty+5, ic->cline);
	}

	if (lines) {

		d=localtime(&data.month[0].month);
		if ( data.month[0].rx==0 || data.month[0].tx==0 || (data.lastupdated-data.month[0].month)==0 ) {
			e_rx=e_tx=0;
		} else {
			e_rx=(data.month[0].rx/(float)(mosecs()))*(dmonth(d->tm_mon)*86400);
			e_tx=(data.month[0].tx/(float)(mosecs()))*(dmonth(d->tm_mon)*86400);
		}
		snprintf(buffer, 32, " estimated   ");
		strncat(buffer, getvalue(e_rx, 0, 10, 2), 32);
		strcat(buffer, "   ");
		strncat(buffer, getvalue(e_tx, 0, 10, 2), 32);
		strcat(buffer, "   ");
		strncat(buffer, getvalue(e_rx+e_tx, 0, 10, 2), 32);

		gdImageString(ic->im, gdFontGetSmall(), textx, texty+8, (unsigned char*)buffer, ic->ctext);
	}
}

void drawtop(IMAGECONTENT *ic)
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

	if (lines) {
		height = 86 + (12 * lines);
	} else {
		height = 98;
	}

	if (!ic->showheader) {
		headermod = 26;
		height -= 22;
	} else {
		headermod = 0;
	}

	ic->im = gdImageCreate(width, height);

	colorinit(ic);

	if (strcmp(data.nick, data.interface)==0) {
		snprintf(buffer, 512, "%s / top 10", data.interface);
	} else {
		snprintf(buffer, 512, "%s (%s) / top 10", data.nick, data.interface);
	}

	layoutinit(ic, buffer, width, height);

	if (lines) {
		if (cfg.ostyle<=2) {
			drawlegend(ic, 398, 40-headermod);
		}
	}

	textx = 10;
	texty = 40-headermod;

	if (cfg.ostyle>2) {
		gdImageString(ic->im, gdFontGetSmall(), textx, texty, (unsigned char*)"   #      day        rx           tx          total       avg. rate", ic->ctext);
		gdImageLine(ic->im, textx+2, texty+16, textx+422, texty+16, ic->cline);
		if (lines) {
			gdImageLine(ic->im, textx+174, texty+2, textx+174, texty+24+(12*lines), ic->cline);
			gdImageLine(ic->im, textx+252, texty+2, textx+252, texty+24+(12*lines), ic->cline);
			gdImageLine(ic->im, textx+330, texty+2, textx+330, texty+24+(12*lines), ic->cline);
		}
	} else {
		gdImageString(ic->im, gdFontGetSmall(), textx, texty, (unsigned char*)"   #      day        rx           tx          total", ic->ctext);
		gdImageLine(ic->im, textx+2, texty+16, textx+326, texty+16, ic->cline);
		if (lines) {
			gdImageLine(ic->im, textx+174, texty+2, textx+174, texty+24+(12*lines), ic->cline);
			gdImageLine(ic->im, textx+252, texty+2, textx+252, texty+24+(12*lines), ic->cline);
		}
	}

	texty += 20;

	for (i = 0; i < 10; i++) {
		if (data.top10[i].used) {

			d = localtime(&data.top10[i].date);
			if (strftime(datebuff, 16, cfg.tformat, d)<=8) {
				snprintf(buffer, 32, "  %2d   %*s   ", i+1, getpadding(8, datebuff), datebuff);
			} else {
				snprintf(buffer, 32, "  %2d  %-*s ", i+1, getpadding(11, datebuff), datebuff);
			}
			strncat(buffer, getvalue(data.top10[i].rx, data.top10[i].rxk, 10, 1), 32);
			strcat(buffer, "   ");
			strncat(buffer, getvalue(data.top10[i].tx, data.top10[i].txk, 10, 1), 32);
			strcat(buffer, "   ");
			strncat(buffer, getvalue(data.top10[i].rx+data.top10[i].tx, data.top10[i].rxk+data.top10[i].txk, 10, 1), 32);
			if (cfg.ostyle>2) {
				strcat(buffer, "  ");
				strncat(buffer, getrate(data.top10[i].rx+data.top10[i].tx, data.top10[i].rxk+data.top10[i].txk, 86400, 14), 32);
			}
			gdImageString(ic->im, gdFontGetSmall(), textx, texty, (unsigned char*)buffer, ic->ctext);
			if (cfg.ostyle>2) {
				drawbar(ic, textx+428, texty+4, 52, data.top10[i].rx, data.top10[i].rxk, data.top10[i].tx, data.top10[i].txk, max);
			} else {
				drawbar(ic, textx+336, texty+4, 140, data.top10[i].rx, data.top10[i].rxk, data.top10[i].tx, data.top10[i].txk, max);
			}
			texty += 12;
		}
	}

	if (lines==0) {
		gdImageString(ic->im, gdFontGetSmall(), textx, texty, (unsigned char*)"                     no data available", ic->ctext);
		texty += 12;
	}

	if (cfg.ostyle>2) {
		gdImageLine(ic->im, textx+2, texty+5, textx+422, texty+5, ic->cline);
	} else {
		gdImageLine(ic->im, textx+2, texty+5, textx+326, texty+5, ic->cline);
	}
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

	snprintf(hex, 3, "%c%c", input[(0+offset)], input[(1+offset)]);
	snprintf(dec, 4, "%d", (int)strtol(hex, NULL, 16));
	rgb[0] = atoi(dec);
	snprintf(hex, 3, "%c%c", input[(2+offset)], input[(3+offset)]);
	snprintf(dec, 4, "%d", (int)strtol(hex, NULL, 16));
	rgb[1] = atoi(dec);
	snprintf(hex, 3, "%c%c", input[(4+offset)], input[(5+offset)]);
	snprintf(dec, 4, "%d", (int)strtol(hex, NULL, 16));
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
		snprintf(buffer, 64, "%*s", len, "--");
	} else {
		/* try to figure out what unit to use */
		if (rate) {
			if (kb>=1000000000) { /* 1000*1000*1000 - value >=1000 Gbps -> show in Tbps */
				snprintf(buffer, 64, "%*.*f", len, declen, kb/(float)1000000000); /* 1000*1000*1000 */
			} else if (kb>=1000000) { /* 1000*1000 - value >=1000 Mbps -> show in Gbps */
				snprintf(buffer, 64, "%*.*f", len, declen, kb/(float)1000000); /* 1000*1000 */
			} else if (kb>=1000) {
				snprintf(buffer, 64, "%*.*f", len, declen, kb/(float)1000);
			} else {
				snprintf(buffer, 64, "%*"PRIu64"", len, kb);
			}
		} else {
			if (kb>=1048576000) { /* 1024*1024*1000 - value >=1000 GiB -> show in TiB */
				snprintf(buffer, 64, "%*.*f", len, declen, kb/(float)1073741824); /* 1024*1024*1024 */
			} else if (kb>=1024000) { /* 1024*1000 - value >=1000 MiB -> show in GiB */
				snprintf(buffer, 64, "%*.*f", len, declen, kb/(float)1048576); /* 1024*1024 */
			} else if (kb>=1000) {
				snprintf(buffer, 64, "%*.*f", len, declen, kb/(float)1024);
			} else {
				snprintf(buffer, 64, "%*"PRIu64"", len, kb);
			}
		}
	}

	return buffer;
}

char *getimagescale(uint64_t kb, int rate)
{
	static char buffer[8];
	uint32_t limit[3];
	int unit;

	if (kb==0) {
		snprintf(buffer, 8, "--");
	} else {

		if (rate) {

			/* convert to proper unit */
			if (cfg.rateunit) {
				limit[0] = 1000;
				limit[1] = 1000000;
				limit[2] = 1000000000;
				unit = 2;
			} else {
				limit[0] = 1024;
				limit[1] = 1024000;
				limit[2] = 1048576000;
				unit = cfg.unitmode;
			}

			if (kb>=limit[2]) {
				snprintf(buffer, 8, "%s", getrateunitprefix(unit, 4));
			} else if (kb>=limit[1]) {
				snprintf(buffer, 8, "%s", getrateunitprefix(unit, 3));
			} else if (kb>=limit[0]) {
				snprintf(buffer, 8, "%s", getrateunitprefix(unit, 2));
			} else {
				snprintf(buffer, 8, "%s", getrateunitprefix(unit, 1));
			}
		} else {
			if (kb>=1048576000) { /* 1024*1024*1000 - value >=1000 GiB -> show in TiB */
				snprintf(buffer, 8, "%s", getunitprefix(4));
			} else if (kb>=1024000) { /* 1024*1000 - value >=1000 MiB -> show in GiB */
				snprintf(buffer, 8, "%s", getunitprefix(3));
			} else if (kb>=1000) {
				snprintf(buffer, 8, "%s", getunitprefix(2));
			} else {
				snprintf(buffer, 8, "%s", getunitprefix(1));
			}
		}

	}

	return buffer;
}
