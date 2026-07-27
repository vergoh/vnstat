#include "common.h"
#include "dbsql.h"
#include "misc.h"
#include "image_support.h"
#include "image_font.h"

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


void imagedrawhline(IMAGECONTENT *ic, const int x1, const int x2, const int y, const int color)
{
	int t, y1, y2, xa, xb;

	t = imageuipx(ic, 1);
	/* center on y; use t/2 so even thicknesses do not grow only downward */
	y1 = y - t / 2;
	y2 = y1 + t - 1;
	if (x1 <= x2) {
		xa = x1;
		xb = x2;
	} else {
		xa = x2;
		xb = x1;
	}
	gdImageFilledRectangle(ic->im, xa, y1, xb, y2, color);
}

void imagedrawvline(IMAGECONTENT *ic, const int x, const int y1, const int y2, const int color)
{
	int t, x1, x2, ya, yb;

	t = imageuipx(ic, 1);
	x1 = x - t / 2;
	x2 = x1 + t - 1;
	if (y1 <= y2) {
		ya = y1;
		yb = y2;
	} else {
		ya = y2;
		yb = y1;
	}
	gdImageFilledRectangle(ic->im, x1, ya, x2, yb, color);
}

void imagedrawrect(IMAGECONTENT *ic, const int x1, const int y1, const int x2, const int y2, const int color)
{
	int t, xa, xb, ya, yb;

	t = imageuipx(ic, 1);
	if (x1 <= x2) {
		xa = x1;
		xb = x2;
	} else {
		xa = x2;
		xb = x1;
	}
	if (y1 <= y2) {
		ya = y1;
		yb = y2;
	} else {
		ya = y2;
		yb = y1;
	}

	/* outer edge of the border sits on the given rectangle */
	gdImageFilledRectangle(ic->im, xa, ya, xb, ya + t - 1, color);
	gdImageFilledRectangle(ic->im, xa, yb - t + 1, xb, yb, color);
	gdImageFilledRectangle(ic->im, xa, ya, xa + t - 1, yb, color);
	gdImageFilledRectangle(ic->im, xb - t + 1, ya, xb, yb, color);
}

void imagedrawdashedhline(IMAGECONTENT *ic, const int x1, const int x2, const int y, const int color)
{
	int t, i, y0;

	t = imageuipx(ic, 1);
	y0 = y - t / 2;
	for (i = 0; i < t; i++) {
		gdImageDashedLine(ic->im, x1, y0 + i, x2, y0 + i, color);
	}
}

/* pixels from graph xpos to axis base (built-in historical GRAPH_AXIS_BASE) */
int graph_axis_left(const IMAGECONTENT *ic)
{
	if (ic->fontctx.mode == FONT_BUILTIN) {
		return GRAPH_AXIS_BASE;
	}

	/* 5-digit field + gap to y-axis; unit text sits at xpos in the left margin
	 * and uses the spare column vs 4-char scale values */
	return ic->fontctx.axis_num5_w + imageuipx(ic, GRAPH_AXIS_LABEL_GAP);
}

/* half of UI stroke thickness; grid lines inset by this so they do not overlap axes */
int graph_stroke_half(const IMAGECONTENT *ic)
{
	return imageuipx(ic, 1) / 2;
}

/* symmetric left/right inset for standalone 5-minute / percentile graphs,
 * redistributes design chrome so unit text and arrow tip have matching margins */
static int graph_side_pad(const IMAGECONTENT *ic)
{
	int cross, pad_full, after_tip, side;

	if (ic->fontctx.mode == FONT_BUILTIN) {
		return 8 + imageextrapx(ic, 14);
	}

	cross = imageuipx(ic, GRAPH_AXIS_CROSS);
	pad_full = imageuipx(ic, FIVEMINWIDTHFULLPADDING);
	after_tip = cross + 1 + imageuipx(ic, GRAPH_EXTRA_RIGHT) - pad_full;
	if (after_tip < 8) {
		after_tip = 8;
	}
	side = (8 + after_tip) / 2;
	return side;
}

int graph_xpos_margin(const IMAGECONTENT *ic)
{
	return graph_side_pad(ic);
}

/* non-plot width for 5-minute / percentile style graphs (left margin + chrome + right) */
int graph_extra_space(const IMAGECONTENT *ic)
{
	int side, cross, pad_full, right;

	if (ic->fontctx.mode == FONT_BUILTIN) {
		return FIVEMINEXTRASPACE + imageextrapx(ic, 14);
	}

	/* balance side pads: left margin == space past axis tip (total width unchanged vs design) */
	side = graph_side_pad(ic);
	cross = imageuipx(ic, GRAPH_AXIS_CROSS);
	pad_full = imageuipx(ic, FIVEMINWIDTHFULLPADDING);
	right = side - cross - 1 + pad_full;
	if (right < imageuipx(ic, 8)) {
		right = imageuipx(ic, 8);
	}
	return side + graph_axis_left(ic) + cross + 1 + right;
}

/* extra plot width matching the 23 hour-to-hour gaps (keeps bars aligned with axis) */
int hourly_plot_extrax(const IMAGECONTENT *ic)
{
	return HOURLY_HOUR_GAPS * imageextrapx(ic, 6);
}

/* pitch between hour columns; poles and ticks scale from this (same curve as plot width) */
int hourly_hour_step(const IMAGECONTENT *ic)
{
	return HOURLY_HOUR_STEP + imageextrapx(ic, 6);
}

/* map a design time hourly pole/tick pixel through the current hour-column pitch */
int hourly_map_px(const IMAGECONTENT *ic, const int design)
{
	const int step = hourly_hour_step(ic);

	if (design == 0) {
		return 0;
	}
	return (design * step + HOURLY_HOUR_STEP / 2) / HOURLY_HOUR_STEP;
}

/* left inset for standalone hourly graph (balanced with right tip room for TTF) */
int hourly_graph_left(const IMAGECONTENT *ic)
{
	const int extrax = hourly_plot_extrax(ic);
	const int pole_pad = imageextrapx(ic, 2);
	const int left_grow = (ic->fontctx.mode == FONT_TTF) ? (imageuipx(ic, 13) - 13) : 0;
	const int axis_past = (ic->fontctx.mode == FONT_TTF) ? imageuipx(ic, HOURLY_AXIS_PAST) : HOURLY_AXIS_PAST;
	const int left_design = 12 + (ic->fontctx.mode == FONT_BUILTIN ? imageextrapx(ic, 14) : 0);
	int right, tip_room, tip_need, side;

	right = (HOURLY_CANVAS_BASE - 12 - GRAPH_AXIS_BASE - HOURLY_PLOT_SPAN)
		+ imageextrapx(ic, 168) - imageextrapx(ic, 14) - extrax + pole_pad
		- left_grow;
	tip_room = right - axis_past - pole_pad;
	tip_need = imageuipx(ic, 8);
	if (tip_room < tip_need) {
		tip_room = tip_need;
	}

	if (ic->fontctx.mode == FONT_BUILTIN) {
		return left_design;
	}

	side = (left_design + tip_room) / 2;
	if (side < left_design) {
		side = left_design;
	}
	return side;
}

/* standalone hourly canvas width: left margin + axis gutter + plot + right pad */
int hourly_graph_width(const IMAGECONTENT *ic)
{
	const int left = hourly_graph_left(ic);
	const int extrax = hourly_plot_extrax(ic);
	const int pole_pad = imageextrapx(ic, 2);
	const int left_grow = (ic->fontctx.mode == FONT_TTF) ? (imageuipx(ic, 13) - 13) : 0;
	const int axis_past = (ic->fontctx.mode == FONT_TTF) ? imageuipx(ic, HOURLY_AXIS_PAST) : HOURLY_AXIS_PAST;
	/* +pole_pad / +left_grow shift hours right so widened poles clear the y-axis */
	const int plot = HOURLY_PLOT_SPAN + extrax + pole_pad + left_grow;
	/* right pad: tip room past axis end matches left inset (TTF centering) */
	int right = left + axis_past + pole_pad;

	if (ic->fontctx.mode == FONT_BUILTIN) {
		right = (HOURLY_CANVAS_BASE - 12 - GRAPH_AXIS_BASE - HOURLY_PLOT_SPAN)
			+ imageextrapx(ic, 168) - imageextrapx(ic, 14) - extrax + pole_pad
			- left_grow;
	}

	return left + graph_axis_left(ic) + plot + right;
}

int image_list_width(const IMAGECONTENT *ic)
{
	return 83 * ic->fontctx.cw + imageuipx(ic, 2) + imageextrapx(ic, 2);
}

/* clamp canvas-vs-natural delta applied to list bars to +/-50% of design bar length */
int image_list_bar_extra(const IMAGECONTENT *ic, const int natural_width, const int design_bar_len)
{
	int delta, max_adjust;

	if (ic->commonwidth <= 0 || natural_width <= 0 || design_bar_len <= 0) {
		return 0;
	}

	delta = ic->commonwidth - natural_width;
	max_adjust = design_bar_len / 2;
	if (max_adjust < 1) {
		max_adjust = 1;
	}
	if (delta > max_adjust) {
		return max_adjust;
	}
	if (delta < -max_adjust) {
		return -max_adjust;
	}
	return delta;
}

void layoutinit(IMAGECONTENT *ic, const char *title, const int width, const int height)
{
	const struct tm *d;
	char datestring[64], buffer[512];
	int rect_top, rect_bottom, title_y, date_y;
	int pad, edge_t, inset, bottom_margin;

	/* get time in given format */
	d = localtime(&ic->interface.updated);
	strftime(datestring, 64, cfg.hformat, d);

	pad = imageuipx(ic, 2);
	edge_t = imageuipx(ic, 1);
	inset = pad + ic->showedge * edge_t;
	bottom_margin = 4 + ic->showedge * edge_t;

	/* background, edges */
	gdImageFill(ic->im, 0, 0, ic->cbackground);
	if (ic->showedge) {
		imagedrawrect(ic, 0, 0, width - 1, height - 1, ic->cedge);
	}

	rect_top = inset;
	rect_bottom = ic->fontctx.header_h;
	title_y = imageuipx(ic, 5) + ic->showedge * edge_t;
	date_y = imageuipx(ic, 9) + ic->showedge * edge_t - imageextrapx(ic, 3);

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

		if (ic->fontctx.mode == FONT_TTF) {
			title_y = imagecentery(ic, FONT_ROLE_HEADER, buffer, rect_top, rect_bottom);
			date_y = imagecentery(ic, FONT_ROLE_TIMESTAMP, datestring, rect_top, rect_bottom);
		}

		gdImageFilledRectangle(ic->im, inset, rect_top, width - 1 - inset, rect_bottom, ic->cheader);
		imagestring(ic, FONT_ROLE_HEADER, imageuipx(ic, 12), title_y, buffer, ic->cheadertitle);
	}

	/* date */
	if (!ic->showheader || ic->altdate) {
		int date_x_alt, date_y_alt;

		if (ic->fontctx.mode == FONT_TTF) {
			/* equal left/bottom clearance; both scale with FontSize via imageuipx() */
			int alt_margin = imageuipx(ic, 1) + ic->showedge * edge_t;

			date_x_alt = alt_margin + imageuipx(ic, 2);
			date_y_alt = height - imagefontheight(ic, FONT_ROLE_TIMESTAMP) - alt_margin;
		} else {
			date_x_alt = imageuipx(ic, 5) + ic->showedge * edge_t;
			date_y_alt = height - imagefontheight(ic, FONT_ROLE_TIMESTAMP) - bottom_margin - imageextrapx(ic, 3);
		}
		imagestring(ic, FONT_ROLE_TIMESTAMP, date_x_alt, date_y_alt, datestring, ic->cvnstat);
	} else {
		imagestring(ic, FONT_ROLE_TIMESTAMP, width - (imagetextwidth(ic, FONT_ROLE_TIMESTAMP, datestring) + imageuipx(ic, 12)), date_y, datestring, ic->cheaderdate);
	}

	/* generator, always using built-in tiny font */
	{
		const char *generator = "vnStat / Teemu Toivola";
		int generator_x = width - imagetextwidth(ic, FONT_ROLE_FOOTER, generator) - bottom_margin;
		int generator_y = height - imagefontheight(ic, FONT_ROLE_FOOTER) - bottom_margin;
		imagestring(ic, FONT_ROLE_FOOTER, generator_x, generator_y, generator, ic->cvnstat);
	}
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

void rtrimspaces(char *s)
{
	size_t n;

	if (s == NULL || s[0] == '\0') {
		return;
	}
	n = strlen(s);
	while (n > 0 && s[n - 1] == ' ') {
		s[--n] = '\0';
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
