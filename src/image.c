#include "common.h"
#include "dbsql.h"
#include "misc.h"
#include "image.h"
#include "vnstati.h"

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
/*		case 2:
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
			break; */
		default:
			printf("Error: Not such query mode: %d\n", cfg.qmode);
			exit(EXIT_FAILURE);
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

void colorinitcheck(const char *color, const int value, const char *cfgtext, const int *rgb)
{
	if (value==-1) {
		printf("Error: ImageColorAllocate failed.\n");
		printf("       C: \"%s\" T: \"%s\" RGB: %d/%d/%d\n", color, cfgtext, rgb[0], rgb[1], rgb[2]);
		exit(EXIT_FAILURE);
	}
}

void layoutinit(IMAGECONTENT *ic, const char *title, const int width, const int height)
{
	struct tm *d;
	char datestring[64];

	/* get time in given format */
	d = localtime(&ic->interface.updated);
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

void drawlegend(IMAGECONTENT *ic, const int x, const int y)
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

void drawbar(IMAGECONTENT *ic, const int x, const int y, const int len, const uint64_t rx, const uint64_t tx, const uint64_t max)
{
	int l, width = len;

	if ((rx+tx)!=max) {
		width=((rx+tx)/(float)max)*len;
	}

	if (width!=0) {

		if (tx>rx) {
			l=rintf((rx/(float)(rx+tx)*width));

			gdImageFilledRectangle(ic->im, x, y+YBEGINOFFSET, x+l, y+YENDOFFSET, ic->crx);
			gdImageRectangle(ic->im, x, y+YBEGINOFFSET, x+l, y+YENDOFFSET, ic->crxd);

			gdImageFilledRectangle(ic->im, x+l, y+YBEGINOFFSET, x+width, y+YENDOFFSET, ic->ctx);
			gdImageRectangle(ic->im, x+l, y+YBEGINOFFSET, x+width, y+YENDOFFSET, ic->ctxd);

		} else {
			l=rintf((tx/(float)(rx+tx)*width));

			gdImageFilledRectangle(ic->im, x, y+YBEGINOFFSET, x+(width-l), y+YENDOFFSET, ic->crx);
			gdImageRectangle(ic->im, x, y+YBEGINOFFSET, x+(width-l), y+YENDOFFSET, ic->crxd);

			gdImageFilledRectangle(ic->im, x+(width-l), y+YBEGINOFFSET, x+width, y+YENDOFFSET, ic->ctx);
			gdImageRectangle(ic->im, x+(width-l), y+YBEGINOFFSET, x+width, y+YENDOFFSET, ic->ctxd);
		}
	}
}

void drawpole(IMAGECONTENT *ic, const int x, const int y, const int len, const uint64_t rx, const uint64_t tx, const uint64_t max)
{
	int l;

	l = (rx/(float)max)*len;
	gdImageFilledRectangle(ic->im, x, y+(len-l), x+7, y+len, ic->crx);

	l = (tx/(float)max)*len;
	gdImageFilledRectangle(ic->im, x+5, y+(len-l), x+12, y+len, ic->ctx);
}

void drawdonut(IMAGECONTENT *ic, const int x, const int y, const float rxp, const float txp)
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

void drawdaily(IMAGECONTENT *ic)
{
	int textx, texty;
	int width, height, headermod;
	uint64_t e_rx, e_tx;
	char buffer[512], datebuff[16];
	struct tm *d;
	dbdatalist *datalist = NULL, *datalist_i = NULL;
	dbdatalistinfo datainfo;

	if (!db_getdata(&datalist, &datainfo, ic->interface.name, "day", 30)) {
		printf("Error: Failed to fetch %s data.\n", "day");
		return;
	}

	datalist_i = datalist;

	width = 500;
	height = 98 + (12 * datainfo.count);

	if (!ic->showheader) {
		headermod = 26;
		height -= 22;
	} else {
		headermod = 0;
	}

	ic->im = gdImageCreate(width, height);

	colorinit(ic);

	if (strcmp(ic->interface.name, ic->interface.alias) == 0 || strlen(ic->interface.alias) == 0) {
		snprintf(buffer, 512, "%s / daily", ic->interface.name);
	} else {
		snprintf(buffer, 512, "%s (%s) / daily", ic->interface.alias, ic->interface.name);
	}

	layoutinit(ic, buffer, width, height);

	if (datainfo.count) {
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
		gdImageLine(ic->im, textx+144, texty+2, textx+144, texty+40+(12*datainfo.count), ic->cline);
		gdImageLine(ic->im, textx+222, texty+2, textx+222, texty+40+(12*datainfo.count), ic->cline);
		gdImageLine(ic->im, textx+300, texty+2, textx+300, texty+40+(12*datainfo.count), ic->cline);
	} else {
		gdImageString(ic->im, gdFontGetSmall(), textx, texty, (unsigned char*)"     day        rx           tx          total", ic->ctext);
		gdImageLine(ic->im, textx+2, texty+16, textx+296, texty+16, ic->cline);
		gdImageLine(ic->im, textx+144, texty+2, textx+144, texty+40+(12*datainfo.count), ic->cline);
		gdImageLine(ic->im, textx+222, texty+2, textx+222, texty+40+(12*datainfo.count), ic->cline);
	}

	texty += 20;

	while (datalist_i != NULL) {

		d = localtime(&datalist_i->timestamp);
		if (strftime(datebuff, 16, cfg.dformat, d)<=8) {
			snprintf(buffer, 32, "  %*s   ", getpadding(8, datebuff), datebuff);
		} else {
			snprintf(buffer, 32, " %-*s ", getpadding(11, datebuff), datebuff);
		}
		strncat(buffer, getvalue(datalist_i->rx, 10, 1), 32);
		strcat(buffer, "   ");
		strncat(buffer, getvalue(datalist_i->tx, 10, 1), 32);
		strcat(buffer, "   ");
		strncat(buffer, getvalue(datalist_i->rx+datalist_i->tx, 10, 1), 32);
		if (cfg.ostyle>2) {
			strcat(buffer, "  ");
			if (datalist_i->next == NULL) {
				d = localtime(&ic->interface.updated);
				strncat(buffer, gettrafficrate(datalist_i->rx+datalist_i->tx, d->tm_sec+(d->tm_min*60)+(d->tm_hour*3600), 14), 32);
			} else {
				strncat(buffer, gettrafficrate(datalist_i->rx+datalist_i->tx, 86400, 14), 32);
			}
		}
		gdImageString(ic->im, gdFontGetSmall(), textx, texty, (unsigned char*)buffer, ic->ctext);
		if (cfg.ostyle>2) {
			drawbar(ic, textx+400, texty+4, 78, datalist_i->rx, datalist_i->tx, datainfo.max);
		} else {
			drawbar(ic, textx+304, texty+4, 170, datalist_i->rx, datalist_i->tx, datainfo.max);
		}
		texty += 12;
		if (datalist_i->next == NULL) {
			break;
		}
		datalist_i = datalist_i->next;
	}

	if (!datainfo.count) {
		gdImageString(ic->im, gdFontGetSmall(), textx, texty, (unsigned char*)"                 no data available", ic->ctext);
		texty += 12;
	}

	if (cfg.ostyle>2) {
		gdImageLine(ic->im, textx+2, texty+5, textx+392, texty+5, ic->cline);
	} else {
		gdImageLine(ic->im, textx+2, texty+5, textx+296, texty+5, ic->cline);
	}

	if (datainfo.count) {

		d=localtime(&ic->interface.updated);
		if ( datalist_i->rx==0 || datalist_i->tx==0 || (d->tm_hour*60+d->tm_min)==0 ) {
			e_rx=e_tx=0;
		} else {
			e_rx=((datalist_i->rx)/(float)(d->tm_hour*60+d->tm_min))*1440;
			e_tx=((datalist_i->tx)/(float)(d->tm_hour*60+d->tm_min))*1440;
		}
		snprintf(buffer, 32, " estimated   ");
		strncat(buffer, getvalue(e_rx, 10, 2), 32);
		strcat(buffer, "   ");
		strncat(buffer, getvalue(e_tx, 10, 2), 32);
		strcat(buffer, "   ");
		strncat(buffer, getvalue(e_rx+e_tx, 10, 2), 32);

		gdImageString(ic->im, gdFontGetSmall(), textx, texty+8, (unsigned char*)buffer, ic->ctext);
	}

	dbdatalistfree(&datalist);
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

void modcolor(int *rgb, const int offset, const int force)
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

char *getimagevalue(const uint64_t b, const int len, const int rate)
{
	static char buffer[64];
	int declen=0;

	if (b==0){
		snprintf(buffer, 64, "%*s", len, "--");
	} else {
		/* try to figure out what unit to use */
		if (rate) {
			if (b>=1000000000) { /* 1000*1000*1000 - value >=1000 Gbps -> show in Tbps */
				snprintf(buffer, 64, "%*.*f", len, declen, b/(float)1000000000); /* 1000*1000*1000 */
			} else if (b>=1000000) { /* 1000*1000 - value >=1000 Mbps -> show in Gbps */
				snprintf(buffer, 64, "%*.*f", len, declen, b/(float)1000000); /* 1000*1000 */
			} else if (b>=1000) {
				snprintf(buffer, 64, "%*.*f", len, declen, b/(float)1000);
			} else {
				snprintf(buffer, 64, "%*"PRIu64"", len, b);
			}
		} else {
			if (b>=1048576000) { /* 1024*1024*1000 - value >=1000 GiB -> show in TiB */
				snprintf(buffer, 64, "%*.*f", len, declen, b/(float)1073741824); /* 1024*1024*1024 */
			} else if (b>=1024000) { /* 1024*1000 - value >=1000 MiB -> show in GiB */
				snprintf(buffer, 64, "%*.*f", len, declen, b/(float)1048576); /* 1024*1024 */
			} else if (b>=1000) {
				snprintf(buffer, 64, "%*.*f", len, declen, b/(float)1024);
			} else {
				snprintf(buffer, 64, "%*"PRIu64"", len, b);
			}
		}
	}

	return buffer;
}

char *getimagescale(const uint64_t b, const int rate)
{
	static char buffer[8];
	uint32_t limit[3];
	int unit;

	if (b==0) {
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

			if (b>=limit[2]) {
				snprintf(buffer, 8, "%s", getrateunitprefix(unit, 4));
			} else if (b>=limit[1]) {
				snprintf(buffer, 8, "%s", getrateunitprefix(unit, 3));
			} else if (b>=limit[0]) {
				snprintf(buffer, 8, "%s", getrateunitprefix(unit, 2));
			} else {
				snprintf(buffer, 8, "%s", getrateunitprefix(unit, 1));
			}
		} else {
			if (b>=1048576000) { /* 1024*1024*1000 - value >=1000 GiB -> show in TiB */
				snprintf(buffer, 8, "%s", getunitprefix(4));
			} else if (b>=1024000) { /* 1024*1000 - value >=1000 MiB -> show in GiB */
				snprintf(buffer, 8, "%s", getunitprefix(3));
			} else if (b>=1000) {
				snprintf(buffer, 8, "%s", getunitprefix(2));
			} else {
				snprintf(buffer, 8, "%s", getunitprefix(1));
			}
		}

	}

	return buffer;
}
