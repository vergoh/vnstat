#include "common.h"
#include "dbsql.h"
#include "misc.h"
#include "image.h"
#include "image_support.h"
#include "vnstati.h"

void imageinit(IMAGECONTENT *ic, const int width, const int height)
{
	int rgb[3], invert = 1;

	ic->im = gdImageCreate(width, height);

	if (ic->invert > 0) {
		invert = -1;
	}

	/* text, edge and header colors */
	hextorgb(cfg.ctext, rgb);
	if (ic->invert > 0) { invertcolor(rgb); }
	ic->ctext = gdImageColorAllocate(ic->im, rgb[0], rgb[1], rgb[2]);
	colorinitcheck("ctext", ic->ctext, cfg.ctext, rgb);
	hextorgb(cfg.cedge, rgb);
	if (ic->invert > 0) { invertcolor(rgb); }
	ic->cedge = gdImageColorAllocate(ic->im, rgb[0], rgb[1], rgb[2]);
	colorinitcheck("cedge", ic->cedge, cfg.cedge, rgb);
	hextorgb(cfg.cheader, rgb);
	if (ic->invert > 0) { invertcolor(rgb); }
	ic->cheader = gdImageColorAllocate(ic->im, rgb[0], rgb[1], rgb[2]);
	colorinitcheck("cheader", ic->cheader, cfg.cheader, rgb);
	hextorgb(cfg.cheadertitle, rgb);
	if (ic->invert > 0) { invertcolor(rgb); }
	ic->cheadertitle = gdImageColorAllocate(ic->im, rgb[0], rgb[1], rgb[2]);
	colorinitcheck("cheadertitle", ic->cheadertitle, cfg.cheadertitle, rgb);
	hextorgb(cfg.cheaderdate, rgb);
	if (ic->invert > 0) { invertcolor(rgb); }
	ic->cheaderdate = gdImageColorAllocate(ic->im, rgb[0], rgb[1], rgb[2]);
	colorinitcheck("cheaderdate", ic->cheaderdate, cfg.cheaderdate, rgb);

	/* lines */
	hextorgb(cfg.cline, rgb);
	if (ic->invert > 0) { invertcolor(rgb); }
	ic->cline = gdImageColorAllocate(ic->im, rgb[0], rgb[1], rgb[2]);
	colorinitcheck("cline", ic->cline, cfg.cline, rgb);
	if (cfg.clinel[0] == '-') {
		modcolor(rgb, 50 * invert, 1);
	} else {
		hextorgb(cfg.clinel, rgb);
		if (ic->invert > 0) { invertcolor(rgb); }
	}
	ic->clinel = gdImageColorAllocate(ic->im, rgb[0], rgb[1], rgb[2]);
	colorinitcheck("clinel", ic->clinel, cfg.clinel, rgb);
	hextorgb(cfg.cpercentileline, rgb);
	if (ic->invert > 0) { invertcolor(rgb); }
	ic->cpercentileline = gdImageColorAllocate(ic->im, rgb[0], rgb[1], rgb[2]);
	colorinitcheck("cpercentileline", ic->cpercentileline, cfg.cpercentileline, rgb);

	/* background */
	hextorgb(cfg.cbg, rgb);
	if (ic->invert > 0) { invertcolor(rgb); }
	ic->cbackground = gdImageColorAllocate(ic->im, rgb[0], rgb[1], rgb[2]);
	colorinitcheck("cbackground", ic->cbackground, cfg.cbg, rgb);
	modcolor(rgb, -35 * invert, 0);
	ic->cvnstat = gdImageColorAllocate(ic->im, rgb[0], rgb[1], rgb[2]);
	colorinitcheck("cvnstat", ic->cvnstat, cfg.cbg, rgb);
	hextorgb(cfg.cbg, rgb);
	if (ic->invert > 0) { invertcolor(rgb); }
	modcolor(rgb, -15 * invert, 0);
	ic->cbgoffset = gdImageColorAllocate(ic->im, rgb[0], rgb[1], rgb[2]);
	colorinitcheck("cbgoffset", ic->cbgoffset, cfg.cbg, rgb);
	hextorgb(cfg.cbg, rgb);
	if (ic->invert > 0) { invertcolor(rgb); }
	modcolor(rgb, -40 * invert, 0);
	ic->cbgoffsetmore = gdImageColorAllocate(ic->im, rgb[0], rgb[1], rgb[2]);
	colorinitcheck("cbgoffsetmore", ic->cbgoffsetmore, cfg.cbg, rgb);

	/* rx */
	hextorgb(cfg.crx, rgb);
	if (ic->invert > 1) { invertcolor(rgb); }
	ic->crx = gdImageColorAllocate(ic->im, rgb[0], rgb[1], rgb[2]);
	colorinitcheck("crx", ic->crx, cfg.crx, rgb);
	if (cfg.crxd[0] == '-') {
		modcolor(rgb, -50 * invert, 1);
	} else {
		hextorgb(cfg.crxd, rgb);
		if (ic->invert > 1) { invertcolor(rgb); }
	}
	ic->crxd = gdImageColorAllocate(ic->im, rgb[0], rgb[1], rgb[2]);
	colorinitcheck("crxd", ic->crxd, cfg.crxd, rgb);

	/* tx */
	hextorgb(cfg.ctx, rgb);
	if (ic->invert > 1) { invertcolor(rgb); }
	ic->ctx = gdImageColorAllocate(ic->im, rgb[0], rgb[1], rgb[2]);
	colorinitcheck("ctx", ic->ctx, cfg.ctx, rgb);
	if (cfg.ctxd[0] == '-') {
		modcolor(rgb, -50 * invert, 1);
	} else {
		hextorgb(cfg.ctxd, rgb);
		if (ic->invert > 1) { invertcolor(rgb); }
	}
	ic->ctxd = gdImageColorAllocate(ic->im, rgb[0], rgb[1], rgb[2]);
	colorinitcheck("ctxd", ic->ctxd, cfg.ctxd, rgb);

	/* total */
	hextorgb(cfg.ctotal, rgb);
	if (ic->invert > 1) { invertcolor(rgb); }
	ic->ctotal = gdImageColorAllocate(ic->im, rgb[0], rgb[1], rgb[2]);
	colorinitcheck("ctotal", ic->ctotal, cfg.ctotal, rgb);
}

void colorinitcheck(const char *color, const int value, const char *cfgtext, const int *rgb)
{
	if (value == -1) {
		printf("Error: ImageColorAllocate failed.\n");
		printf("       C: \"%s\" T: \"%s\" RGB: %d/%d/%d\n", color, cfgtext, rgb[0], rgb[1], rgb[2]);
		exit(EXIT_FAILURE);
	}
}

void layoutinit(IMAGECONTENT *ic, const char *title, const int width, const int height)
{
	const struct tm *d;
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

void drawpercentilelegend(IMAGECONTENT *ic, const int x, const int y, const int mode, const uint64_t percentile)
{
	int color, xoffset = 0;
	char modetext[6], percentiletext[64];

	if (mode == 0) {
		snprintf(modetext, 6, "rx");
		color = ic->crx;
	} else if (mode == 1) {
		snprintf(modetext, 6, "tx");
		color = ic->ctx;
	} else {
		snprintf(modetext, 6, "total");
		color = ic->ctotal;
		xoffset = 18 + (ic->large * 6);
	}

	snprintf(percentiletext, 64, "%s     95th percentile: %s", modetext, gettrafficrate(percentile, 300, 0));
	gdImageString(ic->im, ic->font, x, y, (unsigned char *)percentiletext, ic->ctext);

	gdImageFilledRectangle(ic->im, x - 12 - (ic->large * 2), y + 4, x - 12 + ic->font->w - (ic->large * 2), y + 4 + ic->font->w, color);
	gdImageRectangle(ic->im, x - 12 - (ic->large * 2), y + 4, x - 12 + ic->font->w - (ic->large * 2), y + 4 + ic->font->w, ic->ctext);

	gdImageFilledRectangle(ic->im, x + 30 + (ic->large * 12) + xoffset, y + 4, x + 30 + ic->font->w + (ic->large * 12) + xoffset, y + 4 + ic->font->w, ic->cpercentileline);
	gdImageRectangle(ic->im, x + 30 + (ic->large * 12) + xoffset, y + 4, x + 30 + ic->font->w + (ic->large * 12) + xoffset, y + 4 + ic->font->w, ic->ctext);
}

void drawbar(IMAGECONTENT *ic, const int x, const int y, const int len, const uint64_t rx, const uint64_t tx, const uint64_t max, const short isestimate)
{
	int rxl, txl, width = len, overlap = 0;
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
		if (debug && (rx + tx) > max) {
			printf("Warning: Bar rx + tx sum exceeds given maximum, no bar shown\n");
		}
		return;
	}

	if (width <= 0) {
		return;
	}

	if (tx > rx) {
		rxl = (int)lrint(((double)rx / (double)(rx + tx) * width));
		txl = width - rxl;
	} else {
		txl = (int)lrint(((double)tx / (double)(rx + tx) * width));
		rxl = width - txl;
	}

	if (rxl) {
		if (txl > 0) {
			overlap = 1;
		}
		gdImageFilledRectangle(ic->im, x, y + ybeginoffset, x + rxl - 1 + overlap, y + yendoffset, crx);
		gdImageRectangle(ic->im, x, y + ybeginoffset, x + rxl - 1 + overlap, y + yendoffset, crxd);
	}

	if (txl) {
		gdImageFilledRectangle(ic->im, x + rxl, y + ybeginoffset, x + rxl + txl - 1, y + yendoffset, ctx);
		gdImageRectangle(ic->im, x + rxl, y + ybeginoffset, x + rxl + txl - 1, y + yendoffset, ctxd);
	}
}

void drawpoles(IMAGECONTENT *ic, const int x, const int y, const int len, const uint64_t rx, const uint64_t tx, const uint64_t max)
{
	int l;

	if (rx > 0) {
		l = (int)lrint(((double)rx / (double)max) * len);
		if (l > 0) {
			gdImageFilledRectangle(ic->im, x - (ic->large * 2), y + (len - l), x + 7 + (ic->large * 0), y + len, ic->crx);
		}
	}

	if (tx > 0) {
		l = (int)lrint(((double)tx / (double)max) * len);
		if (l > 0) {
			gdImageFilledRectangle(ic->im, x + 5 - (ic->large * 0), y + (len - l), x + 12 + (ic->large * 2), y + len, ic->ctx);
		}
	}
}

void drawdonut(IMAGECONTENT *ic, const int x, const int y, const float rxp, const float txp, const int size, const int holesize)
{
	// libgd versions 2.2.3 - 2.2.5 have bug in gdImageFilledArc() https://github.com/libgd/libgd/issues/351
	// so workaround needs to be used, 2.2 version series ends with 2.2.5 and the bug is fixed starting from 2.3.0
	if (GD_MAJOR_VERSION == 2 && GD_MINOR_VERSION == 2 && GD_RELEASE_VERSION >= 3) {
		drawdonut_libgd_bug_workaround(ic, x, y, rxp, txp, size, holesize);
	} else {
		drawdonut_libgd_native(ic, x, y, rxp, txp, size, holesize);
	}
}

void drawdonut_libgd_bug_workaround(IMAGECONTENT *ic, const int x, const int y, const float rxp, const float txp, const int size, const int holesize)
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
	gdImageLine(ic->im, x, y, x, y + 2, ic->ctext);
}

void drawarrowright(IMAGECONTENT *ic, const int x, const int y)
{
	gdImageLine(ic->im, x, y, x - 3, y - 2, ic->ctext);
	gdImageLine(ic->im, x, y, x - 3, y + 2, ic->ctext);
	gdImageLine(ic->im, x - 3, y - 2, x - 3, y + 2, ic->ctext);
	gdImageLine(ic->im, x + 1, y, x - 1, y, ic->ctext);
}

void hextorgb(const char *input, int *rgb)
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

void invertcolor(int *rgb)
{
	int i;

	if (debug) {
		printf("invert: %d, %d, %d -> ", rgb[0], rgb[1], rgb[2]);
	}

	for (i = 0; i < 3; i++) {
		rgb[i] = 255 - rgb[i];
	}

	if (debug) {
		printf("%d, %d, %d\n", rgb[0], rgb[1], rgb[2]);
	}
}

char *getimagevalue(const uint64_t b, const int len, const int israte)
{
	static char buffer[64];
	int i, declen = 0, unit = 0, p = 1024;
	uint64_t limit;

	if (b == 0) {
		snprintf(buffer, 64, "%*s", len, "--");
	} else {
		if (israte && (getunit() == 2 || getunit() == 4)) {
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

char *getimagescale(const uint64_t b, const int israte)
{
	static char buffer[8];
	int unit, div = 1, p = 1024;

	unit = getunit();

	if (b == 0) {
		snprintf(buffer, 8, "--");
	} else {
		if (israte) {
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

uint64_t getscale(const uint64_t input, const int israte)
{
	int i, unit;
	unsigned int div = 1024;
	uint64_t result = input;

	unit = getunit();

	if (israte && (unit == 2 || unit == 4)) {
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
