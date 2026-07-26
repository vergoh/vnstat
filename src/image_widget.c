#include "common.h"
#include "misc.h"
#include "image_widget.h"
#include "image_font.h"
#include "image_support.h"

/* draw an edged arc outline of UI stroke thickness
 * step = -2: outer edge at diameter, grow inward (into ring)
 * step = +2: edge at diameter, grow outward (into ring from hole) */
static void imagedrawedgedarc(IMAGECONTENT *ic, const int x, const int y, const int diameter, const int s, const int e, const int color, const int step)
{
	int t, i, d;

	t = imageuipx(ic, 1);
	for (i = 0; i < t; i++) {
		d = diameter + step * i;
		if (d < 1) {
			break;
		}
		gdImageFilledArc(ic->im, x, y, d, d, s, e, color, gdEdged | gdNoFill);
	}
}


/* numeric label on a horizontal scale line (TTF right-align + ascent center;
 * built-in keeps historical x/y via builtin_x / builtin_y) */
void graph_draw_axis_value(IMAGECONTENT *ic, const int axis_x, const int line_y, const char *val, const int builtin_x, const int builtin_y)
{
	int label_y;

	if (val == NULL || val[0] == '\0') {
		return;
	}

	if (ic->fontctx.mode == FONT_TTF) {
		while (*val == ' ') {
			val++;
		}
		label_y = line_y - ic->fontctx.axis_ascent / 2;
		imagestring(ic, FONT_ROLE_AXIS, axis_x - imageuipx(ic, GRAPH_AXIS_LABEL_GAP) - imagetextwidth(ic, FONT_ROLE_AXIS, val), label_y, val, ic->ctext);
	} else {
		imagestring(ic, FONT_ROLE_AXIS, builtin_x, builtin_y, val, ic->ctext);
	}
}

/* vertical unit label; callers pass mode-specific x (anchors differ per graph) */
void graph_draw_axis_unit(IMAGECONTENT *ic, const int x_ttf, const int x_builtin, const int y, const char *text)
{
	if (ic->fontctx.mode == FONT_TTF) {
		imagestringup(ic, FONT_ROLE_AXIS, x_ttf, y, text, ic->ctext);
	} else {
		imagestringup(ic, FONT_ROLE_AXIS, x_builtin, y, text, ic->ctext);
	}
}



void drawlegend(IMAGECONTENT *ic, const int x, const int y, const short israte)
{
	int sq, sq_y;

	if (!ic->showlegend) {
		return;
	}

	sq = ic->fontctx.cw;
	if (ic->fontctx.mode == FONT_TTF) {
		int gap, sep, x_cur, label_w;

		sq_y = y + (ic->fontctx.ch - sq) / 2;
		if (sq_y < y) {
			sq_y = y;
		}
		gap = imageuipx(ic, 4);
		sep = sq + 2 * gap;
		x_cur = israte ? (x - imageuipx(ic, 12)) : x;

		/* [sq][gap][rx][sep][sq][gap][tx]: both labels share y */
		gdImageFilledRectangle(ic->im, x_cur, sq_y, x_cur + sq - 1, sq_y + sq - 1, ic->crx);
		imagedrawrect(ic, x_cur, sq_y, x_cur + sq - 1, sq_y + sq - 1, ic->ctext);
		x_cur += sq + gap;
		imagestring(ic, FONT_ROLE_BODY, x_cur, y, "rx", ic->ctext);
		label_w = imagetextwidth(ic, FONT_ROLE_BODY, "rx");
		x_cur += label_w + sep;

		gdImageFilledRectangle(ic->im, x_cur, sq_y, x_cur + sq - 1, sq_y + sq - 1, ic->ctx);
		imagedrawrect(ic, x_cur, sq_y, x_cur + sq - 1, sq_y + sq - 1, ic->ctext);
		x_cur += sq + gap;
		imagestring(ic, FONT_ROLE_BODY, x_cur, y, "tx", ic->ctext);

		if (israte) {
			label_w = imagetextwidth(ic, FONT_ROLE_BODY, "tx");
			imagestring(ic, FONT_ROLE_BODY, x_cur + label_w + gap, y, "rate", ic->ctext);
		}
		return;
	}

	sq_y = y + 4;

	if (!israte) {
		imagestring(ic, FONT_ROLE_BODY, x, y, "rx     tx", ic->ctext);

		gdImageFilledRectangle(ic->im, x - 12 - imageextrapx(ic, 2), sq_y, x - 12 + sq - imageextrapx(ic, 2), sq_y + sq, ic->crx);
		gdImageRectangle(ic->im, x - 12 - imageextrapx(ic, 2), sq_y, x - 12 + sq - imageextrapx(ic, 2), sq_y + sq, ic->ctext);

		gdImageFilledRectangle(ic->im, x + 30 + imageextrapx(ic, 12), sq_y, x + 30 + sq + imageextrapx(ic, 12), sq_y + sq, ic->ctx);
		gdImageRectangle(ic->im, x + 30 + imageextrapx(ic, 12), sq_y, x + 30 + sq + imageextrapx(ic, 12), sq_y + sq, ic->ctext);
	} else {
		imagestring(ic, FONT_ROLE_BODY, x - 12, y, "rx   tx rate", ic->ctext);

		gdImageFilledRectangle(ic->im, x - 22 - imageextrapx(ic, 3), sq_y, x - 22 + sq - imageextrapx(ic, 3), sq_y + sq, ic->crx);
		gdImageRectangle(ic->im, x - 22 - imageextrapx(ic, 3), sq_y, x - 22 + sq - imageextrapx(ic, 3), sq_y + sq, ic->ctext);

		gdImageFilledRectangle(ic->im, x + 8 + imageextrapx(ic, 7), sq_y, x + 8 + sq + imageextrapx(ic, 7), sq_y + sq, ic->ctx);
		gdImageRectangle(ic->im, x + 8 + imageextrapx(ic, 7), sq_y, x + 8 + sq + imageextrapx(ic, 7), sq_y + sq, ic->ctext);
	}
}

/* TTF legend: [sq][gap][mode][sep][sq][gap][95th percentile: rate] */
static int percentile_legend_ttf_width(IMAGECONTENT *ic, const char *modetext, const char *percentiletext)
{
	const int sq = ic->fontctx.cw;
	const int gap = imageuipx(ic, 4);
	const int sep = sq + 2 * gap;

	return 2 * (sq + gap) + imagetextwidth(ic, FONT_ROLE_BODY, modetext) + sep + imagetextwidth(ic, FONT_ROLE_BODY, percentiletext);
}

static void percentile_legend_texts(const int mode, const uint64_t percentile, char *modetext, const size_t modelen, char *percentiletext, const size_t perclen)
{
	if (mode == 0) {
		snprintf(modetext, modelen, "rx");
	} else if (mode == 1) {
		snprintf(modetext, modelen, "tx");
	} else {
		snprintf(modetext, modelen, "total");
	}
	snprintf(percentiletext, perclen, "95th percentile: %s", gettrafficrate(percentile, 300, 0));
}

int percentilelegendwidth(IMAGECONTENT *ic, const int mode, const uint64_t percentile)
{
	char modetext[6], percentiletext[64];

	percentile_legend_texts(mode, percentile, modetext, sizeof(modetext), percentiletext, sizeof(percentiletext));
	return percentile_legend_ttf_width(ic, modetext, percentiletext);
}

void drawpercentilelegend(IMAGECONTENT *ic, const int x, const int y, const int mode, const uint64_t percentile)
{
	int color, xoffset = 0, sq, sq_y;
	char modetext[6], percentiletext[64];

	if (mode == 0) {
		color = ic->crx;
	} else if (mode == 1) {
		color = ic->ctx;
	} else {
		color = ic->ctotal;
		xoffset = 18 + imageextrapx(ic, 6);
	}

	sq = ic->fontctx.cw;
	if (ic->fontctx.mode == FONT_TTF) {
		int gap, sep, x_cur, label_w;

		percentile_legend_texts(mode, percentile, modetext, sizeof(modetext), percentiletext, sizeof(percentiletext));

		sq_y = y + (ic->fontctx.ch - sq) / 2;
		if (sq_y < y) {
			sq_y = y;
		}
		gap = imageuipx(ic, 4);
		sep = sq + 2 * gap;
		x_cur = x;

		/* [sq][gap][mode][sep][sq][gap][95th percentile: rate] */
		gdImageFilledRectangle(ic->im, x_cur, sq_y, x_cur + sq - 1, sq_y + sq - 1, color);
		imagedrawrect(ic, x_cur, sq_y, x_cur + sq - 1, sq_y + sq - 1, ic->ctext);
		x_cur += sq + gap;
		imagestring(ic, FONT_ROLE_BODY, x_cur, y, modetext, ic->ctext);
		label_w = imagetextwidth(ic, FONT_ROLE_BODY, modetext);
		x_cur += label_w + sep;

		gdImageFilledRectangle(ic->im, x_cur, sq_y, x_cur + sq - 1, sq_y + sq - 1, ic->cpercentileline);
		imagedrawrect(ic, x_cur, sq_y, x_cur + sq - 1, sq_y + sq - 1, ic->ctext);
		x_cur += sq + gap;
		imagestring(ic, FONT_ROLE_BODY, x_cur, y, percentiletext, ic->ctext);
		return;
	}

	if (mode == 0) {
		snprintf(modetext, 6, "rx");
	} else if (mode == 1) {
		snprintf(modetext, 6, "tx");
	} else {
		snprintf(modetext, 6, "total");
	}

	snprintf(percentiletext, 64, "%s     95th percentile: %s", modetext, gettrafficrate(percentile, 300, 0));
	imagestring(ic, FONT_ROLE_BODY, x, y, percentiletext, ic->ctext);

	sq_y = y + 4;

	gdImageFilledRectangle(ic->im, x - 12 - imageextrapx(ic, 2), sq_y, x - 12 + sq - imageextrapx(ic, 2), sq_y + sq, color);
	gdImageRectangle(ic->im, x - 12 - imageextrapx(ic, 2), sq_y, x - 12 + sq - imageextrapx(ic, 2), sq_y + sq, ic->ctext);

	gdImageFilledRectangle(ic->im, x + 30 + imageextrapx(ic, 12) + xoffset, sq_y, x + 30 + sq + imageextrapx(ic, 12) + xoffset, sq_y + sq, ic->cpercentileline);
	gdImageRectangle(ic->im, x + 30 + imageextrapx(ic, 12) + xoffset, sq_y, x + 30 + sq + imageextrapx(ic, 12) + xoffset, sq_y + sq, ic->ctext);
}

void drawbar(IMAGECONTENT *ic, const int x, const int y, const int len, const uint64_t rx, const uint64_t tx, const uint64_t max, const short isestimate)
{
	int rxl, txl, width = len, overlap = 0;
	int crx = ic->crx, ctx = ic->ctx, crxd = ic->crxd, ctxd = ic->ctxd;
	int ybeginoffset = YBEGINOFFSET, yendoffset;

	if (ic->fontctx.mode == FONT_TTF) {
		ybeginoffset = 0;
		yendoffset = ic->fontctx.ascent - 1; /* match digit/value text height */
	} else {
		yendoffset = YBEGINOFFSET + ic->fontctx.ch - 6 - imageextrapx(ic, 1);
	}

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
		int rx2, ya, yb;

		if (txl > 0) {
			overlap = 1;
		}
		rx2 = x + rxl - 1 + overlap;
		ya = y + ybeginoffset;
		yb = y + yendoffset;
		gdImageFilledRectangle(ic->im, x, ya, rx2, yb, crx);
		if (txl > 0) {
			/* omit right edge so the tx left outline forms the join, matching
			 * historical 1px gdImageRectangle behaviour (tx overwrote that edge) */
			int t = imageuipx(ic, 1);

			gdImageFilledRectangle(ic->im, x, ya, rx2, ya + t - 1, crxd);
			gdImageFilledRectangle(ic->im, x, yb - t + 1, rx2, yb, crxd);
			gdImageFilledRectangle(ic->im, x, ya, x + t - 1, yb, crxd);
		} else {
			imagedrawrect(ic, x, ya, rx2, yb, crxd);
		}
	}

	if (txl) {
		gdImageFilledRectangle(ic->im, x + rxl, y + ybeginoffset, x + rxl + txl - 1, y + yendoffset, ctx);
		imagedrawrect(ic, x + rxl, y + ybeginoffset, x + rxl + txl - 1, y + yendoffset, ctxd);
	}
}

void drawpoles(IMAGECONTENT *ic, const int x, const int y, const int len, const uint64_t rx, const uint64_t tx, const uint64_t max)
{
	int l;
	/* scale pole spans with hour-column pitch (same imageextrapx curve as plot width),
	 * imageuipx() grows with ptsize faster than column pitch and clips neighbors */
	const int m5 = hourly_map_px(ic, 5);
	const int m7 = hourly_map_px(ic, 7);
	const int m12 = hourly_map_px(ic, 12);

	if (rx > 0) {
		l = (int)lrint(((double)rx / (double)max) * len);
		if (l > 0) {
			gdImageFilledRectangle(ic->im, x, y + (len - l), x + m7, y + len, ic->crx);
		}
	}

	if (tx > 0) {
		l = (int)lrint(((double)tx / (double)max) * len);
		if (l > 0) {
			gdImageFilledRectangle(ic->im, x + m5, y + (len - l), x + m12, y + len, ic->ctx);
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
		imagedrawedgedarc(ic, x, y, size, 270, 270 + txarc, ic->ctxd, -2);
		if (txarc >= 5) {
			gdImageFill(ic->im, x + 1, y - (size / 2 - 3), ic->ctx);
		}
		imagedrawedgedarc(ic, x, y, holesize, 270, 270 + txarc, ic->ctxd, 2);
	}

	if (rxarc) {
		imagedrawedgedarc(ic, x, y, size, 270 + txarc, 270 + txarc + rxarc, ic->crxd, -2);
		if (rxarc >= 5) {
			gdImageFill(ic->im, (int)(x + (size / 2.0 - 3) * cos((int)((270 * 2 + 2 * txarc + rxarc) / 2) * M_PI / 180)), (int)(y + (size / 2.0 - 3) * sin((int)((270 * 2 + 2 * txarc + rxarc) / 2) * M_PI / 180)), ic->crx);
		}
		imagedrawedgedarc(ic, x, y, holesize, 270 + txarc, 270 + txarc + rxarc, ic->crxd, 2);
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
		imagedrawedgedarc(ic, x, y, size, 270, 270 + txarc, ic->ctxd, -2);
		imagedrawedgedarc(ic, x, y, holesize, 270, 270 + txarc, ic->ctxd, 2);
	}

	if (rxarc) {
		gdImageFilledArc(ic->im, x, y, size, size, 270 + txarc, 270 + txarc + rxarc, ic->crx, 0);
		imagedrawedgedarc(ic, x, y, size, 270 + txarc, 270 + txarc + rxarc, ic->crxd, -2);
		imagedrawedgedarc(ic, x, y, holesize, 270 + txarc, 270 + txarc + rxarc, ic->crxd, 2);
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
	int t, wing, half, tip_x, tip_y;
	gdPoint pts[3];

	t = imageuipx(ic, 1);
	wing = imageuipx(ic, 2);
	half = (t - 1) / 2 + wing;
	/* tip on the integer center of the thick vline stroke */
	tip_x = (x - t / 2) + (t - 1) / 2;
	/* one pixel past the axis end so the tip is the peak */
	tip_y = y - 1;

	pts[0].x = tip_x;
	pts[0].y = tip_y;
	pts[1].x = tip_x - half;
	pts[1].y = tip_y + imageuipx(ic, 3);
	pts[2].x = tip_x + half;
	pts[2].y = tip_y + imageuipx(ic, 3);
	gdImageFilledPolygon(ic->im, pts, 3, ic->ctext);
}

void drawarrowright(IMAGECONTENT *ic, const int x, const int y)
{
	int t, wing, half, tip_x, tip_y;
	gdPoint pts[3];

	t = imageuipx(ic, 1);
	wing = imageuipx(ic, 2);
	half = (t - 1) / 2 + wing;
	/* tip on the integer center of the thick hline stroke */
	tip_y = (y - t / 2) + (t - 1) / 2;
	/* one pixel past the axis end so the tip terminates the line */
	tip_x = x + 1;

	pts[0].x = tip_x;
	pts[0].y = tip_y;
	pts[1].x = tip_x - imageuipx(ic, 3);
	pts[1].y = tip_y - half;
	pts[2].x = tip_x - imageuipx(ic, 3);
	pts[2].y = tip_y + half;
	gdImageFilledPolygon(ic->im, pts, 3, ic->ctext);
}

