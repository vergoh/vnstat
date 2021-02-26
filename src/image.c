#include "common.h"
#include "dbsql.h"
#include "misc.h"
#include "image.h"
#include "vnstati.h"

void initimagecontent(IMAGECONTENT *ic)
{
	ic->im = NULL;
	ic->font = gdFontGetSmall();
	ic->lineheight = 12;
	ic->large = 0;
	ic->showheader = 1;
	ic->showedge = 1;
	ic->showlegend = 1;
	ic->altdate = 0;
	ic->headertext[0] = '\0';
	ic->databegin[0] = '\0';
	ic->dataend[0] = '\0';
	ic->interface.name[0] = '\0';
	ic->interface.alias[0] = '\0';
}

void drawimage(IMAGECONTENT *ic)
{
	// TODO: add checks somewhere that configuration has suitable retention times for selected output?
	switch (cfg.qmode) {
		case 1:
			drawlist(ic, "day");
			break;
		case 2:
			drawlist(ic, "month");
			break;
		case 3:
			drawlist(ic, "top");
			break;
		case 4:
			drawlist(ic, "year");
			break;
		case 5:
			drawsummary(ic, 0, 0);
			break;
		case 51:
			drawsummary(ic, 1, cfg.hourlyrate); // horizontal
			break;
		case 52:
			drawsummary(ic, 2, cfg.hourlyrate); // vertical
			break;
		case 7:
			drawhourly(ic, cfg.hourlyrate);
			break;
		case 8:
			drawlist(ic, "hour");
			break;
		case 9:
			drawlist(ic, "fiveminute");
			break;
		case 10:
			drawfivegraph(ic, cfg.hourlyrate);
			break;
		default:
			printf("Error: No such query mode: %d\n", cfg.qmode);
			exit(EXIT_FAILURE);
	}

	/* enable background transparency if needed */
	if (cfg.transbg) {
		gdImageColorTransparent(ic->im, ic->cbackground);
	}
}

#if HAVE_DECL_GD_NEAREST_NEIGHBOUR
void scaleimage(IMAGECONTENT *ic)
{
	gdImagePtr im_scaled;
	unsigned int width = 0, height = 0;

	if (cfg.imagescale == 100 || ic->im == NULL) {
		return;
	}

	width = (unsigned int)((float)gdImageSX(ic->im) * ((float)cfg.imagescale / (float)100));
	height = (unsigned int)((float)gdImageSY(ic->im) * ((float)cfg.imagescale / (float)100));

	if (width < 100 || height < 100) {
		return;
	}

	if (width > 5000 || height > 5000) {
		return;
	}

	/* keep output sharp when percent is an exact multiplier */
	if (cfg.imagescale % 100 == 0) {
		gdImageSetInterpolationMethod(ic->im, GD_NEAREST_NEIGHBOUR);
	}

	im_scaled = gdImageScale(ic->im, width, height);
	if (im_scaled == NULL) {
		return;
	}

	gdImageDestroy(ic->im);
	ic->im = im_scaled;
}
#endif

void imageinit(IMAGECONTENT *ic, const int width, const int height)
{
	int rgb[3];

	ic->im = gdImageCreate(width, height);

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
	hextorgb(cfg.cbg, rgb);
	modcolor(rgb, -40, 0);
	ic->cbgoffsetmore = gdImageColorAllocate(ic->im, rgb[0], rgb[1], rgb[2]);
	colorinitcheck("cbgoffsetmore", ic->cbgoffsetmore, cfg.cbg, rgb);

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
	if (value == -1) {
		printf("Error: ImageColorAllocate failed.\n");
		printf("       C: \"%s\" T: \"%s\" RGB: %d/%d/%d\n", color, cfgtext, rgb[0], rgb[1], rgb[2]);
		exit(EXIT_FAILURE);
	}
}

void layoutinit(IMAGECONTENT *ic, char *title, const int width, const int height)
{
	struct tm *d;
	char datestring[64], buffer[512];
	gdFontPtr datefont;

	if (ic->large) {
		datefont = gdFontGetSmall();
	} else {
		datefont = gdFontGetTiny();
	}

	/* get time in given format */
	d = localtime(&ic->interface.updated);
	strftime(datestring, 64, cfg.hformat, d);

	/* background, edges */
	gdImageFill(ic->im, 0, 0, ic->cbackground);
	if (ic->showedge) {
		gdImageRectangle(ic->im, 0, 0, width - 1, height - 1, ic->cedge);
	}

	/* titlebox with title */
	if (ic->showheader) {

		if (strlen(ic->headertext)) {
			strncpy_nt(buffer, ic->headertext, 65);
		} else {
			if (strcmp(ic->interface.name, ic->interface.alias) == 0 || strlen(ic->interface.alias) == 0) {
				snprintf(buffer, 512, "%s%s", ic->interface.name, title);
			} else {
				snprintf(buffer, 512, "%s (%s)%s", ic->interface.alias, ic->interface.name, title);
			}
		}

		gdImageFilledRectangle(ic->im, 2 + ic->showedge, 2 + ic->showedge, width - 3 - ic->showedge, 24, ic->cheader);
		gdImageString(ic->im, gdFontGetGiant(), 12, 5 + ic->showedge, (unsigned char *)buffer, ic->cheadertitle);
	}

	/* date */
	if (!ic->showheader || ic->altdate) {
		gdImageString(ic->im, datefont, 5 + ic->showedge, height - 12 - ic->showedge - (ic->large * 3), (unsigned char *)datestring, ic->cvnstat);
	} else {
		gdImageString(ic->im, datefont, width - (((int)strlen(datestring)) * datefont->w + 12), 9 + ic->showedge - (ic->large * 3), (unsigned char *)datestring, ic->cheaderdate);
	}

	/* generator */
	gdImageString(ic->im, gdFontGetTiny(), width - 114 - ic->showedge, height - 12 - ic->showedge, (unsigned char *)"vnStat / Teemu Toivola", ic->cvnstat);
}

void drawlegend(IMAGECONTENT *ic, const int x, const int y, const short israte)
{
	if (!ic->showlegend) {
		return;
	}

	if (!israte) {
		gdImageString(ic->im, ic->font, x, y, (unsigned char *)"rx     tx", ic->ctext);

		gdImageFilledRectangle(ic->im, x - 12 - (ic->large * 2), y + 4, x - 12 + ic->font->w - (ic->large * 2), y + 4 + ic->font->w, ic->crx);
		gdImageRectangle(ic->im, x - 12 - (ic->large * 2), y + 4, x - 12 + ic->font->w - (ic->large * 2), y + 4 + ic->font->w, ic->ctext);

		gdImageFilledRectangle(ic->im, x + 30 + (ic->large * 12), y + 4, x + 30 + ic->font->w + (ic->large * 12), y + 4 + ic->font->w, ic->ctx);
		gdImageRectangle(ic->im, x + 30 + (ic->large * 12), y + 4, x + 30 + ic->font->w + (ic->large * 12), y + 4 + ic->font->w, ic->ctext);
	} else {
		gdImageString(ic->im, ic->font, x - 12, y, (unsigned char *)"rx   tx rate", ic->ctext);

		gdImageFilledRectangle(ic->im, x - 22 - (ic->large * 3), y + 4, x - 22 + ic->font->w - (ic->large * 3), y + 4 + ic->font->w, ic->crx);
		gdImageRectangle(ic->im, x - 22 - (ic->large * 3), y + 4, x - 22 + ic->font->w - (ic->large * 3), y + 4 + ic->font->w, ic->ctext);

		gdImageFilledRectangle(ic->im, x + 8 + (ic->large * 7), y + 4, x + 8 + ic->font->w + (ic->large * 7), y + 4 + ic->font->w, ic->ctx);
		gdImageRectangle(ic->im, x + 8 + (ic->large * 7), y + 4, x + 8 + ic->font->w + (ic->large * 7), y + 4 + ic->font->w, ic->ctext);
	}
}

void drawbar(IMAGECONTENT *ic, const int x, const int y, const int len, const uint64_t rx, const uint64_t tx, const uint64_t max, const short isestimate)
{
	int l, width = len;
	int crx = ic->crx, ctx = ic->ctx, crxd = ic->crxd, ctxd = ic->ctxd;
	int ybeginoffset = YBEGINOFFSET, yendoffset = YBEGINOFFSET + ic->font->h - 6 - ic->large;

	if (isestimate) {

		switch (cfg.estimatestyle) {
			case 0:
				return;
			case 1:
				crx = ic->cbgoffsetmore;
				ctx = ic->cbgoffsetmore;
				crxd = ic->cbgoffsetmore;
				ctxd = ic->cbgoffsetmore;
				break;
			case 2:
				ybeginoffset += 19;
				yendoffset += 19;
				crxd = ic->crx;
				ctxd = ic->ctx;
				crx = ic->cbgoffset;
				ctx = ic->cbgoffset;
				break;
			default:
				return;
		}
	}

	if ((rx + tx) < max) {
		width = (int)lrint(((double)(rx + tx) / (double)max) * len);
	} else if ((rx + tx) > max || max == 0) {
		return;
	}

	if (width <= 0) {
		return;
	}

	if (tx > rx) {
		l = (int)lrint(((double)rx / (double)(rx + tx) * width));

		if (l > 0) {
			gdImageFilledRectangle(ic->im, x, y + ybeginoffset, x + l, y + yendoffset, crx);
			gdImageRectangle(ic->im, x, y + ybeginoffset, x + l, y + yendoffset, crxd);
		}

		gdImageFilledRectangle(ic->im, x + l, y + ybeginoffset, x + width, y + yendoffset, ctx);
		gdImageRectangle(ic->im, x + l, y + ybeginoffset, x + width, y + yendoffset, ctxd);

	} else {
		l = (int)(lrint(((double)tx / (double)(rx + tx) * width)));

		gdImageFilledRectangle(ic->im, x, y + ybeginoffset, x + (width - l), y + yendoffset, crx);
		gdImageRectangle(ic->im, x, y + ybeginoffset, x + (width - l), y + yendoffset, crxd);

		if (l > 0) {
			gdImageFilledRectangle(ic->im, x + (width - l), y + ybeginoffset, x + width, y + yendoffset, ctx);
			gdImageRectangle(ic->im, x + (width - l), y + ybeginoffset, x + width, y + yendoffset, ctxd);
		}
	}
}

void drawpoles(IMAGECONTENT *ic, const int x, const int y, const int len, const uint64_t rx, const uint64_t tx, const uint64_t max)
{
	int l;

	l = (int)lrint(((double)rx / (double)max) * len);
	if (l > 0) {
		gdImageFilledRectangle(ic->im, x - (ic->large * 2), y + (len - l), x + 7 + (ic->large * 0), y + len, ic->crx);
	}

	l = (int)lrint(((double)tx / (double)max) * len);
	if (l > 0) {
		gdImageFilledRectangle(ic->im, x + 5 - (ic->large * 0), y + (len - l), x + 12 + (ic->large * 2), y + len, ic->ctx);
	}
}

void drawdonut(IMAGECONTENT *ic, const int x, const int y, const float rxp, const float txp, const int size, const int holesize)
{
	int rxarc = 0, txarc = 0;

	if ((int)(rxp + txp) > 0) {
		rxarc = (int)lrintf(360 * (rxp / (float)100));
		if ((int)(rxp + txp) == 100) {
			txarc = 360 - rxarc;
		} else {
			txarc = (int)lrintf(360 * (txp / (float)100));
		}
	}

	// background filled circle
	gdImageFilledArc(ic->im, x, y, size, size, 0, 360, ic->cbgoffset, 0);

	if (txarc) {
		gdImageFilledArc(ic->im, x, y, size, size, 270, 270 + txarc, ic->ctxd, gdEdged | gdNoFill);
		if (txarc >= 5) {
			gdImageFill(ic->im, x + 1, y - (size / 2 - 3), ic->ctx);
		}
		gdImageFilledArc(ic->im, x, y, holesize, holesize, 270, 270 + txarc, ic->ctxd, gdEdged | gdNoFill);
	}

	if (rxarc) {
		gdImageFilledArc(ic->im, x, y, size, size, 270 + txarc, 270 + txarc + rxarc, ic->crxd, gdEdged | gdNoFill);
		if (rxarc >= 5) {
			gdImageFill(ic->im, (int)(x + (size / 2 - 3) * cos((int)((270 * 2 + 2 * txarc + rxarc) / 2) * M_PI / 180)), (int)(y + (size / 2 - 3) * sin((int)((270 * 2 + 2 * txarc + rxarc) / 2) * M_PI / 180)), ic->crx);
		}
		gdImageFilledArc(ic->im, x, y, holesize, holesize, 270 + txarc, 270 + txarc + rxarc, ic->crxd, gdEdged | gdNoFill);
	}

	// remove center from background filled circle, making it a donut
	gdImageFilledArc(ic->im, x, y, holesize - 2, holesize - 2, 0, 360, ic->cbackground, 0);
}

#ifdef CHECK_VNSTAT
void drawdonut_libgd_native(IMAGECONTENT *ic, const int x, const int y, const float rxp, const float txp, const int size, const int holesize)
{
	int rxarc = 0, txarc = 0;

	if ((int)(rxp + txp) > 0) {
		rxarc = (int)(360 * (rxp / (float)100));
		if ((int)(rxp + txp) == 100) {
			txarc = 360 - rxarc;
		} else {
			txarc = (int)(360 * (txp / (float)100));
		}
	}

	// background filled circle
	gdImageFilledArc(ic->im, x, y, size, size, 0, 360, ic->cbgoffset, 0);

	if (txarc) {
		gdImageFilledArc(ic->im, x, y, size, size, 270, 270 + txarc, ic->ctx, 0);
		gdImageFilledArc(ic->im, x, y, size, size, 270, 270 + txarc, ic->ctxd, gdEdged | gdNoFill);
		gdImageFilledArc(ic->im, x, y, holesize, holesize, 270, 270 + txarc, ic->ctxd, gdEdged | gdNoFill);
	}

	if (rxarc) {
		gdImageFilledArc(ic->im, x, y, size, size, 270 + txarc, 270 + txarc + rxarc, ic->crx, 0);
		gdImageFilledArc(ic->im, x, y, size, size, 270 + txarc, 270 + txarc + rxarc, ic->crxd, gdEdged | gdNoFill);
		gdImageFilledArc(ic->im, x, y, holesize, holesize, 270 + txarc, 270 + txarc + rxarc, ic->crxd, gdEdged | gdNoFill);
	}

	// remove center from background filled circle, making it a donut
	gdImageFilledArc(ic->im, x, y, holesize - 2, holesize - 2, 0, 360, ic->cbackground, 0);
}
#endif

int drawhours(IMAGECONTENT *ic, const int xpos, const int ypos, const int rate)
{
	int i, tmax = 0, s = 0, step, prev = 0, diff = 0, chour;
	int x = xpos, y = ypos, extrax = 0, extray = 0;
	double ratediv;
	uint64_t max = 1, scaleunit = 0;
	char buffer[32];
	struct tm *d;
	dbdatalist *datalist = NULL, *datalist_i = NULL;
	dbdatalistinfo datainfo;
	HOURDATA hourdata[24];
	gdFontPtr font;

	if (ic->large) {
		font = gdFontGetSmall();
	} else {
		font = gdFontGetTiny();
	}

	for (i = 0; i < 24; i++) {
		hourdata[i].rx = hourdata[i].tx = 0;
		hourdata[i].date = 0;
	}

	if (!db_getdata(&datalist, &datainfo, ic->interface.name, "hour", 24) || datainfo.count == 0) {
		gdImageString(ic->im, ic->font, x + (32 * ic->font->w), y + 54, (unsigned char *)"no data available", ic->ctext);
		return 0;
	}

	datalist_i = datalist;

	while (datalist_i != NULL) {
		d = localtime(&datalist_i->timestamp);
		if (hourdata[d->tm_hour].date != 0 || ic->interface.updated - datalist_i->timestamp > 86400) {
			datalist_i = datalist_i->next;
			continue;
		}
		hourdata[d->tm_hour].rx = datalist_i->rx;
		hourdata[d->tm_hour].tx = datalist_i->tx;
		hourdata[d->tm_hour].date = datalist_i->timestamp;
		datalist_i = datalist_i->next;
	}
	dbdatalistfree(&datalist);

	ic->current = ic->interface.updated;
	chour = localtime(&ic->current)->tm_hour;

	if (cfg.rateunit) {
		ratediv = 450; /* x * 8 / 3600 */
	} else {
		ratediv = 3600;
	}

	/* tmax (time max) = current hour */
	/* max = transfer max */

	for (i = 0; i < 24; i++) {
		/* convert hourly transfer to hourly rate if needed */
		if (rate) {
			if ((ic->current - hourdata[i].date) > 3600) {
				hourdata[i].rx = (uint64_t)((double)hourdata[i].rx / ratediv);
				hourdata[i].tx = (uint64_t)((double)hourdata[i].tx / ratediv);
			} else {
				/* scale ongoing hour properly */
				if (chour != i) {
					hourdata[i].rx = (uint64_t)((double)hourdata[i].rx / ratediv);
					hourdata[i].tx = (uint64_t)((double)hourdata[i].tx / ratediv);
				} else {
					d = localtime(&ic->current);
					diff = d->tm_min * 60;
					if (!diff) {
						diff = 60;
					}
					if (cfg.rateunit == 1) {
						hourdata[i].rx *=  8;
						hourdata[i].tx *=  8;
					}
					hourdata[i].rx = (uint64_t)((double)hourdata[i].rx / (double)diff);
					hourdata[i].tx = (uint64_t)((double)hourdata[i].tx / (double)diff);
				}
			}
		}

		if (hourdata[i].date >= hourdata[tmax].date) {
			tmax = i;
		}
		if (hourdata[i].rx >= max) {
			max = hourdata[i].rx;
		}
		if (hourdata[i].tx >= max) {
			max = hourdata[i].tx;
		}
	}

	/* scale values */
	scaleunit = getscale(max, rate);
	if (max / scaleunit > 4) {
		step = 2;
	} else {
		step = 1;
	}

	if (ic->large) {
		x += 14;
		extrax = 145;
		extray = 35;
	}

	for (i = step; (uint64_t)(scaleunit * (unsigned int)i) <= max; i = i + step) {
		s = (int)((121 + extray) * (((double)scaleunit * (unsigned int)i) / (double)max));
		gdImageDashedLine(ic->im, x + 36, y + 124 - s, x + 460 + extrax, y + 124 - s, ic->cline);
		gdImageDashedLine(ic->im, x + 36, y + 124 - ((s + prev) / 2), x + 460 + extrax, y + 124 - ((s + prev) / 2), ic->clinel);
		gdImageString(ic->im, font, x + 16 - (ic->large * 3), y + 121 - s - (ic->large * 3), (unsigned char *)getimagevalue(scaleunit * (unsigned int)i, 3, rate), ic->ctext);
		prev = s;
	}
	s = (int)((121 + extray) * (((double)scaleunit * (unsigned int)i) / (double)max));
	if (((s + prev) / 2) <= (128 + extray)) {
		gdImageDashedLine(ic->im, x + 36, y + 124 - ((s + prev) / 2), x + 460 + extrax, y + 124 - ((s + prev) / 2), ic->clinel);
	} else {
		i = i - step;
	}

	/* scale text */
	gdImageStringUp(ic->im, font, x - 2 - (ic->large * 14), y + 60 + (rate * 10) - (extray / 2), (unsigned char *)getimagescale(scaleunit * (unsigned int)i, rate), ic->ctext);

	/* x-axis values and poles */
	for (i = 0; i < 24; i++) {
		s = tmax - i;
		if (s < 0) {
			s += 24;
		}
		snprintf(buffer, 32, "%02d ", s);
		gdImageString(ic->im, font, x + 440 - (i * (17 + ic->large * 6)) + extrax, y + 128, (unsigned char *)buffer, ic->ctext);
		drawpoles(ic, x + 438 - (i * (17 + ic->large * 6)) + extrax, y - extray, 124 + extray, hourdata[s].rx, hourdata[s].tx, max);
	}

	/* axis */
	gdImageLine(ic->im, x + 36 - 4, y + 124, x + 466 + extrax, y + 124, ic->ctext);
	gdImageLine(ic->im, x + 36, y - 10 - extray, x + 36, y + 124 + 4, ic->ctext);

	/* arrows */
	drawarrowup(ic, x + 36, y - 9 - extray);
	drawarrowright(ic, x + 465 + extrax, y + 124);

	return 1;
}

void drawhourly(IMAGECONTENT *ic, const int rate)
{
	int width, height, headermod = 0;

	width = 500 + (ic->large * 168);
	height = 200 + (ic->large * 48);

	if (!ic->showheader) {
		headermod = 26;
		height -= 22;
	}

	imageinit(ic, width, height);
	layoutinit(ic, " / hourly", width, height);

	if (drawhours(ic, 12, 46 - headermod + (ic->large * 40), rate)) {
		drawlegend(ic, 242 + (ic->large * 84), 183 - headermod + (ic->large * 46), 0);
	}
}

void drawlist(IMAGECONTENT *ic, const char *listname)
{
	ListType listtype = LT_None;
	int textx, texty, offsetx = 0;
	int width, height, headermod, i = 1, rowcount = 0;
	int estimateavailable = 0, estimatevisible = 0;
	int32_t limit;
	uint64_t e_rx = 0, e_tx = 0, e_secs = 0;
	char buffer[512], datebuff[16], daybuff[16];
	char stampformat[64], titlename[16], colname[8];
	struct tm *d;
	time_t current;
	dbdatalist *datalist = NULL, *datalist_i = NULL;
	dbdatalistinfo datainfo;

	if (strcmp(listname, "day") == 0) {
		listtype = LT_Day;
		strncpy_nt(colname, listname, 8);
		snprintf(titlename, 16, "daily");
		strncpy_nt(stampformat, cfg.dformat, 64);
		limit = cfg.listdays;
	} else if (strcmp(listname, "month") == 0) {
		listtype = LT_Month;
		strncpy_nt(colname, listname, 8);
		snprintf(titlename, 16, "monthly");
		strncpy_nt(stampformat, cfg.mformat, 64);
		limit = cfg.listmonths;
	} else if (strcmp(listname, "year") == 0) {
		listtype = LT_Year;
		strncpy_nt(colname, listname, 8);
		snprintf(titlename, 16, "yearly");
		strncpy_nt(stampformat, "%Y", 64);
		limit = cfg.listyears;
	} else if (strcmp(listname, "top") == 0) {
		listtype = LT_Top;
		snprintf(colname, 8, "day");
		strncpy_nt(stampformat, cfg.tformat, 64);
		limit = cfg.listtop;
		offsetx = 5 * ic->font->w;
	} else if (strcmp(listname, "hour") == 0) {
		listtype = LT_Hour;
		strncpy_nt(colname, listname, 8);
		snprintf(titlename, 16, "hourly");
		strncpy_nt(stampformat, "%H:%M", 64);
		limit = cfg.listhours;
	} else if (strcmp(listname, "fiveminute") == 0) {
		listtype = LT_5min;
		strncpy_nt(colname, "time", 8);
		snprintf(titlename, 16, "5 minute");
		strncpy_nt(stampformat, "%H:%M", 64);
		limit = cfg.listfivemins;
	} else {
		return;
	}

	if (limit < 0) {
		limit = 0;
	}

	daybuff[0] = '\0';

	db_getdata_range(&datalist, &datainfo, ic->interface.name, listname, (uint32_t)limit, ic->databegin, ic->dataend);

	datalist_i = datalist;

	if (strlen(ic->dataend) == 0 && datainfo.count > 0 && listtype != LT_Top) {
		getestimates(&e_rx, &e_tx, listtype, ic->interface.updated, &datalist);
		if (cfg.estimatestyle > 0 && e_rx + e_tx > datainfo.max) {
			datainfo.max = e_rx + e_tx;
		}
		estimateavailable = 1;
		if (listtype == LT_Day || listtype == LT_Month || listtype == LT_Year) {
			estimatevisible = 1;
		}
	}

	if (listtype == LT_Top) {
		if (limit > 0 && datainfo.count < (uint32_t)limit) {
			limit = (int32_t)datainfo.count;
		}
		if (limit <= 0 || datainfo.count > 999) {
			snprintf(titlename, 16, "top");
		} else {
			snprintf(titlename, 16, "top %d", limit);
		}
	}

	if (listtype == LT_Hour || listtype == LT_5min) {
		while (datalist_i != NULL) {
			d = localtime(&datalist_i->timestamp);
			strftime(datebuff, 16, cfg.dformat, d);
			if (strcmp(daybuff, datebuff) != 0) {
				rowcount += 1;
				strcpy(daybuff, datebuff);
			}
			datalist_i = datalist_i->next;
		}
		datalist_i = datalist;
		daybuff[0] = '\0';
	}
	rowcount += datainfo.count;

	width = 83 * ic->font->w + 2 + (ic->large * 2);
	height = 62 + 3 * ic->lineheight;

	// less space needed when no estimate or sum is shown (Top, 5min and Hour never have estimate)
	if ((!estimatevisible && datainfo.count < 2) || (listtype == LT_Top || listtype == LT_Hour || listtype == LT_5min)) {
		height = 62 + 2 * ic->lineheight;
	}

	// exception for 5min and Hour when having sum shown
	if ((listtype == LT_5min || listtype == LT_Hour) && datainfo.count > 1 && strlen(ic->dataend) > 0) {
		height = 62 + 3 * ic->lineheight;
	}

	height += (ic->lineheight + cfg.linespaceadjust) * rowcount - cfg.linespaceadjust;

	// "no data available"
	if (!datainfo.count) {
		height = 98 + (ic->large * 12);
	}

	if (!ic->showheader) {
		headermod = 26;
		height -= 22;
	} else {
		headermod = 0;
	}

	snprintf(buffer, 512, " / %s", titlename);

	imageinit(ic, width, height);
	layoutinit(ic, buffer, width, height);

	if (datainfo.count) {
		if (listtype == LT_Top) {
			if (cfg.ostyle <= 2) {
				drawlegend(ic, 66 * ic->font->w + 2, 40 - headermod, 0);
			}
			current = time(NULL);
			d = localtime(&current);
			strftime(daybuff, 16, stampformat, d);
		} else { // everything else
			if (cfg.ostyle > 2) {
				if (estimateavailable && cfg.barshowsrate) {
					drawlegend(ic, 72 * ic->font->w, 40 - headermod, 1);
				} else {
					drawlegend(ic, 72 * ic->font->w, 40 - headermod, 0);
				}
			} else {
				drawlegend(ic, 64 * ic->font->w + 1, 40 - headermod, 0);
			}
		}
	}

	textx = 10;
	texty = 40 - headermod;

	if (listtype == LT_Top) { // top
		snprintf(buffer, 512, "   #      day        rx           tx          total");
	} else { // everything else
		snprintf(buffer, 512, " %8s       rx           tx          total", colname);
	}
	if (cfg.ostyle > 2) {
		strcat(buffer, "       avg. rate");
		gdImageString(ic->im, ic->font, textx, texty, (unsigned char *)buffer, ic->ctext);
		gdImageLine(ic->im, textx + 2, texty + ic->lineheight + 4, textx + (65 * ic->font->w) + offsetx + 2, texty + ic->lineheight + 4, ic->cline);
	} else {
		gdImageString(ic->im, ic->font, textx, texty, (unsigned char *)buffer, ic->ctext);
		gdImageLine(ic->im, textx + 2, texty + ic->lineheight + 4, textx + (50 * ic->font->w) + offsetx - 4, texty + ic->lineheight + 4, ic->cline);
	}

	texty += ic->lineheight + 8;

	if (datainfo.count) {
		gdImageLine(ic->im, textx + (24 * ic->font->w) + offsetx, texty - 6 - ic->lineheight, textx + (24 * ic->font->w) + offsetx, texty + ((ic->lineheight + cfg.linespaceadjust) * rowcount) - cfg.linespaceadjust + 5 - (ic->large * 2), ic->cline);
		gdImageLine(ic->im, textx + (37 * ic->font->w) + offsetx, texty - 6 - ic->lineheight, textx + (37 * ic->font->w) + offsetx, texty + ((ic->lineheight + cfg.linespaceadjust) * rowcount) - cfg.linespaceadjust + 5 - (ic->large * 2), ic->cline);
		if (cfg.ostyle > 2) {
			gdImageLine(ic->im, textx + (50 * ic->font->w) + offsetx, texty - 6 - ic->lineheight, textx + (50 * ic->font->w) + offsetx, texty + ((ic->lineheight + cfg.linespaceadjust) * rowcount) - cfg.linespaceadjust + 5 - (ic->large * 2), ic->cline);
		}
	} else {
		gdImageLine(ic->im, textx + (24 * ic->font->w) + offsetx, texty - 6 - ic->lineheight, textx + (24 * ic->font->w) + offsetx, texty - 4, ic->cline);
		gdImageLine(ic->im, textx + (37 * ic->font->w) + offsetx, texty - 6 - ic->lineheight, textx + (37 * ic->font->w) + offsetx, texty - 4, ic->cline);
		if (cfg.ostyle > 2) {
			gdImageLine(ic->im, textx + (50 * ic->font->w) + offsetx, texty - 6 - ic->lineheight, textx + (50 * ic->font->w) + offsetx, texty - 4, ic->cline);
		}
	}

	while (datalist_i != NULL) {
		d = localtime(&datalist_i->timestamp);

		if (listtype == LT_5min || listtype == LT_Hour) {
			strftime(datebuff, 16, cfg.dformat, d);
			if (strcmp(daybuff, datebuff) != 0) {
				snprintf(buffer, 32, " %s", datebuff);
				gdImageString(ic->im, ic->font, textx, texty, (unsigned char *)buffer, ic->ctext);
				texty += ic->lineheight + cfg.linespaceadjust;
				strcpy(daybuff, datebuff);
			}
		}

		if (listtype == LT_Top) {
			if (strftime(datebuff, 16, stampformat, d) <= 8) {
				snprintf(buffer, 32, "  %2d   %*s", i, getpadding(8, datebuff), datebuff);
				strcat(buffer, "   ");
			} else {
				snprintf(buffer, 32, "  %2d  %-*s ", i, getpadding(11, datebuff), datebuff);
			}
			if (strcmp(datebuff, daybuff) == 0) {
				if (cfg.ostyle > 2) {
					gdImageFilledRectangle(ic->im, textx + 2, texty + 2 - (ic->large * 1), textx + (65 * ic->font->w) + offsetx + 2, texty + 11 + (ic->large * 3), ic->cbgoffset);
				} else {
					gdImageFilledRectangle(ic->im, textx + 2, texty + 2 - (ic->large * 1), textx + (50 * ic->font->w) + offsetx - 4, texty + 11 + (ic->large * 3), ic->cbgoffset);
				}
			}
		} else {
			if (strftime(datebuff, 16, stampformat, d) <= 8) {
				snprintf(buffer, 32, "  %*s", getpadding(8, datebuff), datebuff);
				strcat(buffer, "   ");
			} else {
				snprintf(buffer, 32, " %-*s ", getpadding(11, datebuff), datebuff);
			}
		}
		strncat(buffer, getvalue(datalist_i->rx, 10, RT_Normal), 32);
		strcat(buffer, "   ");
		strncat(buffer, getvalue(datalist_i->tx, 10, RT_Normal), 32);
		strcat(buffer, "   ");
		strncat(buffer, getvalue(datalist_i->rx + datalist_i->tx, 10, RT_Normal), 32);
		if (cfg.ostyle > 2) {
			strcat(buffer, "  ");
			if (datalist_i->next == NULL && issametimeslot(listtype, datalist_i->timestamp, ic->interface.updated)) {
				e_secs = getperiodseconds(listtype, datalist_i->timestamp, ic->interface.updated, 1);
			} else {
				e_secs = getperiodseconds(listtype, datalist_i->timestamp, ic->interface.updated, 0);
			}
			strncat(buffer, gettrafficrate(datalist_i->rx + datalist_i->tx, (time_t)e_secs, 14), 32);
		}
		gdImageString(ic->im, ic->font, textx, texty, (unsigned char *)buffer, ic->ctext);
		if (listtype == LT_Top) {
			if (cfg.ostyle > 2) {
				drawbar(ic, textx + (71 * ic->font->w) + 2, texty + 4, 9 * ic->font->w - 2, datalist_i->rx, datalist_i->tx, datainfo.max, 0);
			} else {
				drawbar(ic, textx + (56 * ic->font->w), texty + 4, 23 * ic->font->w + 2, datalist_i->rx, datalist_i->tx, datainfo.max, 0);
			}
		} else { // everything else
			if (cfg.ostyle > 2) {
				if (datalist_i->next == NULL && estimateavailable && cfg.barshowsrate) {
					drawbar(ic, textx + (67 * ic->font->w) - 2, texty + 4, 13 * ic->font->w, e_rx, e_tx, datainfo.max, 0);
				} else {
					drawbar(ic, textx + (67 * ic->font->w) - 2, texty + 4, 13 * ic->font->w, datalist_i->rx, datalist_i->tx, datainfo.max, 0);
				}
			} else {
				drawbar(ic, textx + (51 * ic->font->w) - 2, texty + 4, 28 * ic->font->w + 2, datalist_i->rx, datalist_i->tx, datainfo.max, 0);
			}
		}
		texty += ic->lineheight + cfg.linespaceadjust;
		if (datalist_i->next == NULL) {
			texty -= cfg.linespaceadjust;
			break;
		}
		datalist_i = datalist_i->next;
		i++;
	}

	if (!datainfo.count) {
		i = 17 * ic->font->w;
		if (cfg.ostyle > 2) {
			i += 8 * ic->font->w;
		}
		gdImageString(ic->im, ic->font, textx + i, texty, (unsigned char *)"no data available", ic->ctext);
		texty += ic->lineheight;
	}

	if (cfg.ostyle > 2) {
		gdImageLine(ic->im, textx + 2, texty + 5 - (ic->large * 2), textx + (65 * ic->font->w) + offsetx + 2, texty + 5 - (ic->large * 2), ic->cline);
	} else {
		gdImageLine(ic->im, textx + 2, texty + 5 - (ic->large * 2), textx + (50 * ic->font->w) + offsetx - 4, texty + 5 - (ic->large * 2), ic->cline);
	}

	buffer[0] = '\0';

	/* estimate visible */
	if (estimatevisible) {
		snprintf(buffer, 32, " estimated   ");
		strncat(buffer, getvalue(e_rx, 10, RT_Estimate), 32);
		strcat(buffer, "   ");
		strncat(buffer, getvalue(e_tx, 10, RT_Estimate), 32);
		strcat(buffer, "   ");
		strncat(buffer, getvalue(e_rx + e_tx, 10, RT_Estimate), 32);

		if (cfg.estimatestyle) {
			if (cfg.ostyle > 2) {
				drawbar(ic, textx + (67 * ic->font->w) - 2, texty - ic->lineheight + 4, 13 * ic->font->w, e_rx, e_tx, datainfo.max, 1);
				drawbar(ic, textx + (67 * ic->font->w) - 2, texty - ic->lineheight + 4, 13 * ic->font->w, datalist_i->rx, datalist_i->tx, datainfo.max, 0);
			} else {
				drawbar(ic, textx + (51 * ic->font->w) - 2, texty - ic->lineheight + 4, 28 * ic->font->w + 2, e_rx, e_tx, datainfo.max, 1);
				drawbar(ic, textx + (51 * ic->font->w) - 2, texty - ic->lineheight + 4, 28 * ic->font->w + 2, datalist_i->rx, datalist_i->tx, datainfo.max, 0);
			}
		}

	/* sum visible */
	} else if (strlen(ic->dataend) > 0 && datainfo.count > 1 && listtype != LT_Top) {
		if (datainfo.count < 100) {
			snprintf(datebuff, 16, "sum of %" PRIu32 "", datainfo.count);
		} else {
			snprintf(datebuff, 16, "sum");
		}
		snprintf(buffer, 32, " %9s   ", datebuff);
		strncat(buffer, getvalue(datainfo.sumrx, 10, RT_Normal), 32);
		strcat(buffer, "   ");
		strncat(buffer, getvalue(datainfo.sumtx, 10, RT_Normal), 32);
		strcat(buffer, "   ");
		strncat(buffer, getvalue(datainfo.sumrx + datainfo.sumtx, 10, RT_Normal), 32);
	}

	if (strlen(buffer) > 0) {
		texty += 8;
		gdImageString(ic->im, ic->font, textx, texty, (unsigned char *)buffer, ic->ctext);

		gdImageLine(ic->im, textx + (24 * ic->font->w) + offsetx, texty - 6, textx + (24 * ic->font->w) + offsetx, texty + ic->lineheight - (ic->large * 2), ic->cline);
		gdImageLine(ic->im, textx + (37 * ic->font->w) + offsetx, texty - 6, textx + (37 * ic->font->w) + offsetx, texty + ic->lineheight - (ic->large * 2), ic->cline);
		if (cfg.ostyle > 2) {
			gdImageLine(ic->im, textx + (50 * ic->font->w) + offsetx, texty - 6, textx + (50 * ic->font->w) + offsetx, texty + ic->lineheight - (ic->large * 2), ic->cline);
		}
	}

	dbdatalistfree(&datalist);
}

void drawsummary(IMAGECONTENT *ic, const int layout, const int rate)
{
	int width, height, headermod;

	switch (layout) {
		// horizontal
		case 1:
			width = 163 * ic->font->w + 2 + (ic->large * 2);
			height = 56 + 12 * ic->lineheight;
			break;
		// vertical
		case 2:
			width = 83 * ic->font->w + 2 + (ic->large * 2);
			height = 370 + (ic->large * 90);
			break;
		// no hours
		default:
			width = 83 * ic->font->w + 2 + (ic->large * 2);
			height = 56 + 12 * ic->lineheight;
			break;
	}

	if (!ic->showheader) {
		headermod = 26;
		height -= 22;
	} else {
		headermod = 0;
	}

	imageinit(ic, width, height);
	layoutinit(ic, "", width, height);

	if (ic->interface.rxtotal == 0 && ic->interface.txtotal == 0) {
		gdImageString(ic->im, ic->font, 33 * ic->font->w, 100, (unsigned char *)"no data available", ic->ctext);
		return;
	}

	drawsummary_alltime(ic, 385 + (ic->large * 125), 57 - headermod + (ic->large * 10));
	drawlegend(ic, 410 + (ic->large * 132), 155 - headermod + (ic->large * 40), 0);

	drawsummary_digest(ic, 100, 30 - headermod, "day");
	drawsummary_digest(ic, 100, 29 + 7 * ic->lineheight - headermod, "month");

	switch (layout) {
		// horizontal
		case 1:
			drawhours(ic, 500 + (ic->large * 160), 46 + (ic->large * 40) - headermod, rate);
			break;
		// vertical
		case 2:
			drawhours(ic, 12, 215 + (ic->large * 84) - headermod, rate);
			break;
		default:
			break;
	}
}

void drawsummary_alltime(IMAGECONTENT *ic, const int x, const int y)
{
	struct tm *d;
	char buffer[512], datebuff[16], daytemp[32];
	gdFontPtr titlefont;

	if (ic->large) {
		titlefont = gdFontGetGiant();
	} else {
		titlefont = gdFontGetLarge();
	}

	gdImageString(ic->im, titlefont, x + 12 + (ic->large * 10), y, (unsigned char *)"all time", ic->ctext);
	snprintf(buffer, 4, "rx ");
	strncat(buffer, getvalue(ic->interface.rxtotal, 12, RT_Normal), 32);
	gdImageString(ic->im, ic->font, x, y + (2 * ic->lineheight), (unsigned char *)buffer, ic->ctext);
	snprintf(buffer, 4, "tx ");
	strncat(buffer, getvalue(ic->interface.txtotal, 12, RT_Normal), 32);
	gdImageString(ic->im, ic->font, x, y + (3 * ic->lineheight), (unsigned char *)buffer, ic->ctext);
	snprintf(buffer, 4, " = ");
	strncat(buffer, getvalue(ic->interface.rxtotal + ic->interface.txtotal, 12, RT_Normal), 32);
	gdImageString(ic->im, ic->font, x, y + (4 * ic->lineheight) + 2 + (ic->large * 4), (unsigned char *)buffer, ic->ctext);
	d = localtime(&ic->interface.created);
	strftime(datebuff, 16, cfg.tformat, d);
	snprintf(daytemp, 24, "since %s", datebuff);
	snprintf(buffer, 32, "%23s", daytemp);
	gdImageString(ic->im, ic->font, x - 8 * ic->font->w, y + (5 * ic->lineheight) + 10 + (ic->large * 4), (unsigned char *)buffer, ic->ctext);
}

void drawsummary_digest(IMAGECONTENT *ic, const int x, const int y, const char *mode)
{
	int textx, texty, offset = 0;
	double rxp = 50, txp = 50, mod;
	char buffer[512], datebuff[16], daytemp[32];
	time_t yesterday;
	struct tm *d = NULL;
	dbdatalist *datalist = NULL;
	dbdatalist *data_current = NULL, *data_previous = NULL;
	dbdatalistinfo datainfo;
	gdFontPtr titlefont;

	if (ic->large) {
		titlefont = gdFontGetGiant();
	} else {
		titlefont = gdFontGetLarge();
	}

	yesterday = ic->current - 86400;

	switch(mode[0]) {
		case 'd':
			break;
		case 'm':
			break;
		default:
			printf("Error: Unsupported mode %s for summary digest\n", mode);
			return;
	}

	if (!db_getdata(&datalist, &datainfo, ic->interface.name, mode, 2) || datalist == NULL) {
		gdImageString(ic->im, ic->font, 25 * ic->font->w, y + 30, (unsigned char *)"no data available", ic->ctext);
		return;
	} else if (datalist->next == NULL) {
		data_current = datalist;
	} else {
		data_previous = datalist;
		data_current = datalist->next;
	}

	/* latest entry */
	if (data_current->rx + data_current->tx == 0) {
		rxp = txp = 0;
	} else {
		rxp = (double)data_current->rx / (double)(data_current->rx + data_current->tx) * 100;
		txp = (double)100 - rxp;
	}

	/* do scaling if needed */
	if (data_previous != NULL && (data_current->rx + data_current->tx) < (data_previous->rx + data_previous->tx)) {
		mod = (double)(data_current->rx + data_current->tx) / (double)(data_previous->rx + data_previous->tx);
		rxp = rxp * mod;
		txp = txp * mod;
	}

	/* move graph to center if there's only one to draw for this line */
	if (data_previous == NULL) {
		offset = 85 + (ic->large * 25);
	}

	textx = x + offset;
	texty = y;

	drawdonut(ic, textx + 50 + (ic->large * 40), texty + 45 + (ic->large * 10), (float)rxp, (float)txp, 49 + (ic->large * 10), 15 + (ic->large * 3));

	if (mode[0] == 'd') {
		/* get formatted date for today */
		d = localtime(&ic->current);
		strftime(datebuff, 16, cfg.dformat, d);

		/* get formatted date for current day in database */
		d = localtime(&data_current->timestamp);
		strftime(daytemp, 16, cfg.dformat, d);

		/* change daytemp to today if formatted days match */
		if (strcmp(datebuff, daytemp) == 0) {
			strncpy_nt(daytemp, "today", 32);
		}
	} else if (mode[0] == 'm') {
		d = localtime(&data_current->timestamp);
		strftime(daytemp, 16, cfg.mformat, d);
	}

	snprintf(buffer, 32, "%*s", getpadding(12, daytemp), daytemp);
	gdImageString(ic->im, titlefont, textx - 54 + (ic->large * (ic->font->w * 3 - 4)), texty - 1, (unsigned char *)buffer, ic->ctext);

	if (cfg.summaryrate) {
		d = localtime(&ic->interface.updated);
		if (mode[0] == 'd') {
			snprintf(buffer, 16, "%15s", gettrafficrate(data_current->rx + data_current->tx, d->tm_sec + (d->tm_min * 60) + (d->tm_hour * 3600), 15));
		} else if (mode[0] == 'm') {
			snprintf(buffer, 16, "%15s", gettrafficrate(data_current->rx + data_current->tx, mosecs(data_current->timestamp, ic->interface.updated), 15));
		}
		gdImageString(ic->im, ic->font, textx - 74, texty + 4 * ic->lineheight + 10, (unsigned char *)buffer, ic->ctext);
	} else {
		texty += 7;
	}

	snprintf(buffer, 4, "rx ");
	strncat(buffer, getvalue(data_current->rx, 12, RT_Normal), 32);
	gdImageString(ic->im, ic->font, textx - 74, texty + ic->lineheight + 6, (unsigned char *)buffer, ic->ctext);
	snprintf(buffer, 4, "tx ");
	strncat(buffer, getvalue(data_current->tx, 12, RT_Normal), 32);
	gdImageString(ic->im, ic->font, textx - 74, texty + 2 * ic->lineheight + 6, (unsigned char *)buffer, ic->ctext);
	snprintf(buffer, 4, " = ");
	strncat(buffer, getvalue(data_current->rx + data_current->tx, 12, RT_Normal), 32);
	gdImageString(ic->im, ic->font, textx - 74, texty + 3 * ic->lineheight + 8, (unsigned char *)buffer, ic->ctext);

	/* previous entry */
	if (data_previous != NULL) {
		if (data_previous->rx + data_previous->tx == 0) {
			rxp = txp = 0;
		} else {
			rxp = (double)data_previous->rx / (double)(data_previous->rx + data_previous->tx) * 100;
			txp = (double)100 - rxp;
		}

		/* do scaling if needed */
		if ((data_previous->rx + data_previous->tx) < (data_current->rx + data_current->tx)) {
			mod = (double)(data_previous->rx + data_previous->tx) / (double)(data_current->rx + data_current->tx);
			rxp = rxp * mod;
			txp = txp * mod;
		}

		textx += 180 + (ic->large * 60);

		drawdonut(ic, textx + 50 + (ic->large * 40), texty + 45 + (ic->large * 10), (float)rxp, (float)txp, 49 + (ic->large * 10), 15 + (ic->large * 3));

		if (mode[0] == 'd') {
			/* get formatted date for yesterday */
			d = localtime(&yesterday);
			strftime(datebuff, 16, cfg.dformat, d);

			/* get formatted date for previous day in database */
			d = localtime(&data_previous->timestamp);
			strftime(daytemp, 16, cfg.dformat, d);

			/* change daytemp to yesterday if formatted days match */
			if (strcmp(datebuff, daytemp) == 0) {
				strncpy_nt(daytemp, "yesterday", 32);
			}
		} else if (mode[0] == 'm') {
			d = localtime(&data_previous->timestamp);
			strftime(daytemp, 16, cfg.mformat, d);
		}

		snprintf(buffer, 32, "%*s", getpadding(12, daytemp), daytemp);
		gdImageString(ic->im, titlefont, textx - 54 + (ic->large * (ic->font->w * 3 - 4)), texty - 1, (unsigned char *)buffer, ic->ctext);

		if (cfg.summaryrate) {
			if (mode[0] == 'd') {
				snprintf(buffer, 16, "%15s", gettrafficrate(data_previous->rx + data_previous->tx, 86400, 15));
			} else if (mode[0] == 'm') {
				snprintf(buffer, 16, "%15s", gettrafficrate(data_previous->rx + data_previous->tx, dmonth(d->tm_mon) * 86400, 15));
			}
			gdImageString(ic->im, ic->font, textx - 74, texty + 4 * ic->lineheight + 10, (unsigned char *)buffer, ic->ctext);
		} else {
			texty += 7;
		}

		snprintf(buffer, 4, "rx ");
		strncat(buffer, getvalue(data_previous->rx, 12, RT_Normal), 32);
		gdImageString(ic->im, ic->font, textx - 74, texty + ic->lineheight + 6, (unsigned char *)buffer, ic->ctext);
		snprintf(buffer, 4, "tx ");
		strncat(buffer, getvalue(data_previous->tx, 12, RT_Normal), 32);
		gdImageString(ic->im, ic->font, textx - 74, texty + 2 * ic->lineheight + 6, (unsigned char *)buffer, ic->ctext);
		snprintf(buffer, 4, " = ");
		strncat(buffer, getvalue(data_previous->rx + data_previous->tx, 12, RT_Normal), 32);
		gdImageString(ic->im, ic->font, textx - 74, texty + 3 * ic->lineheight + 8, (unsigned char *)buffer, ic->ctext);
	}

	data_current = NULL;
	data_previous = NULL;
	dbdatalistfree(&datalist);
}

void drawfivegraph(IMAGECONTENT *ic, const int rate)
{
	int width, height, headermod = 0;

	width = 668;
	height = 300; // TODO: this could probably be made configurable

	if (!ic->showheader) {
		headermod = 26;
		height -= 22;
	}

	imageinit(ic, width, height);
	layoutinit(ic, " / 5 minute", width, height);

	if (drawfiveminutes(ic, 16, height - 37 - headermod, rate, height - 80)) {
		drawlegend(ic, width / 2 - (ic->large * 10), height - 19 - headermod, 0);
	}
}

int drawfiveminutes(IMAGECONTENT *ic, const int xpos, const int ypos, const int rate, const int height)
{
	int x = xpos, y = ypos, i = 0, t = 0, rxh = 0, txh = 0, step, s = 0, prev = 0;
	uint64_t scaleunit, max;
	time_t timestamp;
	double ratediv;
	char buffer[32];
	struct tm *d;
	dbdatalist *datalist = NULL, *datalist_i = NULL;
	dbdatalistinfo datainfo;
	gdFontPtr font;

	if (ic->large) {
		font = gdFontGetSmall();
	} else {
		font = gdFontGetTiny();
	}

	if (!db_getdata(&datalist, &datainfo, ic->interface.name, "fiveminute", FIVEMINRESULTCOUNT) || datainfo.count == 0) {
		gdImageString(ic->im, ic->font, x + 330 - (8 * ic->font->w), y - (height / 2) - ic->font->h, (unsigned char *)"no data available", ic->ctext);
		return 0;
	}

	datalist_i = datalist;

	if (cfg.rateunit) {
		ratediv = 37.5; /* x * 8 / 300 */
	} else {
		ratediv = 300;
	}

	/* axis */
	x += 36;
	gdImageLine(ic->im, x, y, x + FIVEMINWIDTHFULL, y, ic->ctext);
	gdImageLine(ic->im, x + 4, y + 4, x + 4, y - height, ic->ctext);

	/* arrows */
	drawarrowup(ic, x + 4, y - 1 - height);
	drawarrowright(ic, x + 1 + FIVEMINWIDTHFULL, y);

	max = datainfo.maxrx + datainfo.maxtx;

	if (datainfo.maxrx > datainfo.maxtx) {
		txh = (int)lrint(((double)datainfo.maxtx / (double)max) * (height - FIVEMINHEIGHTOFFSET * 2));
		rxh = height - FIVEMINHEIGHTOFFSET * 2 - txh;
		max = (uint64_t)((double)datainfo.maxrx / ratediv);
		t = rxh;
	} else {
		rxh = (int)lrint(((double)datainfo.maxrx / (double)max) * (height - FIVEMINHEIGHTOFFSET * 2));
		txh = height - FIVEMINHEIGHTOFFSET * 2 - rxh;
		max = (uint64_t)((double)datainfo.maxtx / ratediv);
		t = txh;
	}

	/* center line */
	x += 5;
	y -= txh + FIVEMINHEIGHTOFFSET;
	gdImageLine(ic->im, x, y, x + FIVEMINWIDTH - 1, y, ic->ctext);
	gdImageString(ic->im, font, x - 21 - (ic->large * 3), y - 4 - (ic->large * 3), (unsigned char *)"  0", ic->ctext);

	/* scale values */
	scaleunit = getscale(max, rate);

	s = (int)lrint(((double)scaleunit / (double)max) * t);
	if (s < FIVEMINSCALEMINPIXELS) {
		step = 2;
	} else {
		step = 1;
	}

	if (debug) {
		printf("maxrx: %lu\n", datainfo.maxrx);
		printf("maxtx: %lu\n", datainfo.maxtx);
		printf("rxh: %d     txh: %d\n", rxh, txh);
		printf("max divided: %lu\n", max);
		printf("scaleunit:   %lu\nstep: %d\n", scaleunit, step);
		printf("pixels per step: %d\n", s);
		printf("mintime: %lu\nmaxtime: %lu\n", (uint64_t)datainfo.mintime, (uint64_t)datainfo.maxtime);
		printf("count: %u\n", datainfo.count);
	}

	/* upper part scale values */
	y--; // adjust to start above center line
	for (i = 1 * step; i * s <= rxh; i = i + step) {
		gdImageDashedLine(ic->im, x, y - (i * s), x + FIVEMINWIDTH - 1, y - (i * s), ic->cline);
		gdImageDashedLine(ic->im, x, y - prev - (step * s) / 2, x + FIVEMINWIDTH - 1, y - prev - (step * s) / 2, ic->clinel);
		gdImageString(ic->im, font, x - 21 - (ic->large * 3), y - 3 - (i * s) - (ic->large * 3), (unsigned char *)getimagevalue(scaleunit * (unsigned int)i, 3, rate), ic->ctext);
		prev = i * s;
	}
	if ((prev + (step * s) / 2) <= rxh) {
		gdImageDashedLine(ic->im, x, y - prev - (step * s) / 2, x + FIVEMINWIDTH - 1, y - prev - (step * s) / 2, ic->clinel);
	}

	y += 2; // adjust to start below center line
	prev = 0;

	/* lower part scale values */
	for (i = 1 * step; i * s <= txh; i = i + step) {
		gdImageDashedLine(ic->im, x, y + (i * s), x + FIVEMINWIDTH - 1, y + (i * s), ic->cline);
		gdImageDashedLine(ic->im, x, y + prev + (step * s) / 2, x + FIVEMINWIDTH - 1, y + prev + (step * s) / 2, ic->clinel);
		gdImageString(ic->im, font, x - 21 - (ic->large * 3), y - 3 + (i * s) - (ic->large * 3), (unsigned char *)getimagevalue(scaleunit * (unsigned int)i, 3, rate), ic->ctext);
		prev = i * s;
	}
	if ((prev + (step * s) / 2) <= txh) {
		gdImageDashedLine(ic->im, x, y + prev + (step * s) / 2, x + FIVEMINWIDTH - 1, y + prev + (step * s) / 2, ic->clinel);
	}

	y--; // y is now back on center line

	/* scale text */
	gdImageStringUp(ic->im, font, x - 44 - (ic->large * 5), ypos - height / 2 + (rate * 10), (unsigned char *)getimagescale(scaleunit * (unsigned int)i, rate), ic->ctext);

	/* TODO
		- last value needs to be scaled if not full 5 minute has passed
		- indicate somehow areas where the database didn't provide any data?
	*/

	timestamp = datainfo.maxtime - (FIVEMINRESULTCOUNT * 300);

	while (datalist_i != NULL && datalist_i->timestamp < timestamp + 300) {
		if (debug) {
			printf("Skip data, %lu < %lu\n", datalist_i->timestamp, timestamp + 300);
		}
		datalist_i = datalist_i->next;
	}

	for (i = 0; i < FIVEMINRESULTCOUNT; i++) {

		if (datalist_i == NULL) {
			break;
		}

		timestamp += 300;
		d = localtime(&timestamp);

		if (d->tm_min == 0 && i > 2) {
			if (d->tm_hour % 2 == 0) {
				if (d->tm_hour == 0) {
					gdImageLine(ic->im, x + i, y + txh - 1 + FIVEMINHEIGHTOFFSET, x + i, y - rxh - 2 - FIVEMINHEIGHTOFFSET, ic->cline);
				} else {
					gdImageLine(ic->im, x + i, y + txh - 1 + FIVEMINHEIGHTOFFSET, x + i, y - rxh - 2 - FIVEMINHEIGHTOFFSET, ic->cbgoffset);
				}

				if (i > font->w) {
					snprintf(buffer, 32, "%02d", d->tm_hour);
					gdImageString(ic->im, font, x + i - font->w + 1, y + txh + font->h - (ic->large * 5), (unsigned char *)buffer, ic->ctext);
				}
			} else {
				gdImageLine(ic->im, x + i, y + txh - 1 + FIVEMINHEIGHTOFFSET, x + i, y - rxh - 2 - FIVEMINHEIGHTOFFSET, ic->cbgoffset);
			}
		}

		if (datalist_i->timestamp > timestamp) {
			continue;
		}

		t = (int)lrint(((double)datalist_i->rx / (double)datainfo.maxrx) * rxh);
		drawpole(ic, x + i, y - 1, t, 1, ic->crx);

		t = (int)lrint(((double)datalist_i->tx / (double)datainfo.maxtx) * txh);
		drawpole(ic, x + i, y + 1, t, 2, ic->ctx);

		datalist_i = datalist_i->next;
	}

	dbdatalistfree(&datalist);

	/* redraw center line */
	x = xpos + 40;
	gdImageLine(ic->im, x, y, x + FIVEMINWIDTH, y, ic->ctext);

	return 1;
}

void drawpole(IMAGECONTENT *ic, const int x, const int y, const int length, const int direction, const int color)
{
	int len = length - 1;

	if (length > 0) {
		switch (direction) {
			case 1:
				gdImageLine(ic->im, x, y, x, y - len, color);
				break;
			case 2:
				gdImageLine(ic->im, x, y, x, y + len, color);
				break;
			default:
				break;
		}
	}
}

void drawarrowup(IMAGECONTENT *ic, const int x, const int y)
{
	gdImageLine(ic->im, x, y, x + 2, y + 3, ic->ctext);
	gdImageLine(ic->im, x, y, x - 2, y + 3, ic->ctext);
	gdImageLine(ic->im, x - 2, y + 3, x + 2, y + 3, ic->ctext);
	gdImageLine(ic->im, x, y + 1, x, y - 1, ic->ctext);
}

void drawarrowright(IMAGECONTENT *ic, const int x, const int y)
{
	gdImageLine(ic->im, x, y, x - 3, y - 2, ic->ctext);
	gdImageLine(ic->im, x, y, x - 3, y + 2, ic->ctext);
	gdImageLine(ic->im, x - 3, y - 2, x - 3, y + 2, ic->ctext);
	gdImageLine(ic->im, x + 1, y, x - 1, y, ic->ctext);
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

	snprintf(hex, 3, "%c%c", input[(0 + offset)], input[(1 + offset)]);
	snprintf(dec, 4, "%d", (int)strtol(hex, NULL, 16));
	rgb[0] = atoi(dec);
	snprintf(hex, 3, "%c%c", input[(2 + offset)], input[(3 + offset)]);
	snprintf(dec, 4, "%d", (int)strtol(hex, NULL, 16));
	rgb[1] = atoi(dec);
	snprintf(hex, 3, "%c%c", input[(4 + offset)], input[(5 + offset)]);
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

	for (i = 0; i < 3; i++) {
		if ((rgb[i] + offset) > 255 || (rgb[i] + offset) < 0) {
			overflow++;
		}
	}

	/* positive offset gives lighter color, negative darker if forced */
	/* otherwise the direction is changed depending on possible overflows */
	for (i = 0; i < 3; i++) {
		if (overflow < 2 || force) {
			if ((rgb[i] + offset) > 255) {
				rgb[i] = 255;
			} else if ((rgb[i] + offset) < 0) {
				rgb[i] = 0;
			} else {
				rgb[i] += offset;
			}
		} else {
			if ((rgb[i] - offset) < 0) {
				rgb[i] = 0;
			} else if ((rgb[i] - offset) > 255) {
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
	int i, declen = 0, unit = 0, p = 1024;
	uint64_t limit;

	if (b == 0) {
		snprintf(buffer, 64, "%*s", len, "--");
	} else {
		if (rate && (getunit() == 2 || getunit() == 4)) {
			p = 1000;
			unit = getunit();
		}
		for (i = UNITPREFIXCOUNT - 1; i > 0; i--) {
			limit = (uint64_t)(pow(p, i - 1)) * 1000;
			if (b >= limit) {
				snprintf(buffer, 64, "%*.*f", len, declen, (double)b / (double)(getunitdivisor(unit, i + 1)));
				return buffer;
			}
		}
		snprintf(buffer, 64, "%*" PRIu64 "", len, b);
	}

	return buffer;
}

char *getimagescale(const uint64_t b, const int rate)
{
	static char buffer[8];
	int unit, div = 1, p = 1024;

	unit = getunit();

	if (b == 0) {
		snprintf(buffer, 8, "--");
	} else {
		if (rate) {
			if (unit == 2 || unit == 4) {
				p = 1000;
			}
			while (div < UNITPREFIXCOUNT && (double)b >= (pow(p, div - 1) * 1000)) {
				div++;
			}
			snprintf(buffer, 8, "%s", getrateunitprefix(unit, div));
		} else {
			while (div < UNITPREFIXCOUNT && (double)b >= (pow(p, div - 1) * 1000)) {
				div++;
			}
			snprintf(buffer, 8, "%s", getunitprefix(div));
		}
	}
	return buffer;
}

uint64_t getscale(const uint64_t input, const int rate)
{
	int i, unit;
	unsigned int div = 1024;
	uint64_t result = input;

	unit = getunit();

	if (rate && (unit == 2 || unit == 4)) {
		div = 1000;
	}

	/* get unit */
	for (i = 0; result >= div; i++) {
		result = result / div;
	}

	/* round result depending of scale */
	if (result >= 300) {
		result = result / 4 + (100 - ((result / 4) % 100));
	} else if (result > 20) {
		result = result / 4 + (10 - ((result / 4) % 10));
	} else {
		result = result / 4;
	}

	/* put unit back */
	if (i) {
		result = result * (uint64_t)(pow(div, i));
	}

	/* make sure result isn't zero */
	if (!result) {
		if (i) {
			result = (uint64_t)(pow(div, i));
		} else {
			result = 1;
		}
	}

	return result;
}
