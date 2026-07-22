#include "common.h"
#include "dbsql.h"
#include "misc.h"
#include "image.h"
#include "image_support.h"

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

static gdFontPtr imagerolefont(const IMAGECONTENT *ic, const fontrole_t role)
{
	switch (role) {
		case FONT_ROLE_AXIS:
			return ic->fontctx.axis;
		case FONT_ROLE_TITLE:
			return ic->fontctx.title;
		case FONT_ROLE_HEADER:
			return ic->fontctx.header;
		case FONT_ROLE_FOOTER:
			return ic->fontctx.footer;
		case FONT_ROLE_BODY:
		default:
			return ic->fontctx.body;
	}
}

static int imageroleusesbuiltin(const IMAGECONTENT *ic, const fontrole_t role)
{
	if (ic->fontctx.mode != FONT_TTF || role == FONT_ROLE_FOOTER) {
		return 1;
	}
	return 0;
}

static double imageroleptsize(const IMAGECONTENT *ic, const fontrole_t role)
{
	switch (role) {
		case FONT_ROLE_AXIS:
			return ic->fontctx.ptsize * ic->fontctx.axis_scale;
		case FONT_ROLE_TITLE:
		case FONT_ROLE_HEADER:
			return ic->fontctx.ptsize * ic->fontctx.title_scale;
		case FONT_ROLE_BODY:
		case FONT_ROLE_FOOTER:
		default:
			return ic->fontctx.ptsize;
	}
}

#if HAVE_DECL_GDIMAGESTRINGFT
static int fontcache_ready = 0;

static char *imagettfbbox(const IMAGECONTENT *ic, const double ptsize, const double angle, const char *text, int *brect)
{
	return gdImageStringFT(NULL, brect, 0, (char *)ic->fontctx.ttfpath, ptsize, angle, 0, 0, (char *)text);
}

static int imagettftextwidth(const IMAGECONTENT *ic, const double ptsize, const char *text)
{
	int brect[8];
	char *err;

	if (text == NULL || text[0] == '\0') {
		return 0;
	}

	err = imagettfbbox(ic, ptsize, 0.0, text, brect);
	if (err != NULL) {
		return 0;
	}

	/* Pen-origin to right edge. Do not use brect[2]-brect[0]: a positive left
	 * bearing (common on digit-leading strings) would shrink the width and
	 * shift right-aligned values like "155.27 GiB" off the unit column. */
	return brect[2];
}

static int imagettfinitmetrics(IMAGECONTENT *ic)
{
	int brect[8], template_cw, digit_cw, value_w;
	char *err;
	const char *errprefix = "Error: Unable to use FontFile";

	if (gdFontCacheSetup() != 0) {
		fprintf(stderr, "Error: gdFontCacheSetup failed.\n");
		return 0;
	}
	fontcache_ready = 1;

	/* Template-average cell width for space-padded layouts. */
	err = imagettfbbox(ic, ic->fontctx.ptsize, 0.0, "  0000-00-00   000.00 GiB   000.00 GiB   000.00 GiB", brect);
	if (err != NULL) {
		fprintf(stderr, "%s \"%s\": %s\n", errprefix, ic->fontctx.ttfpath, err);
		imagefontcleanup();
		return 0;
	}
	template_cw = (brect[2] - brect[0] + 50) / 51; /* ceil average, template length 51 */

	err = imagettfbbox(ic, ic->fontctx.ptsize, 0.0, "0123456789", brect);
	if (err != NULL) {
		fprintf(stderr, "%s \"%s\": %s\n", errprefix, ic->fontctx.ttfpath, err);
		imagefontcleanup();
		return 0;
	}
	digit_cw = (brect[2] - brect[0] + 9) / 10;

	ic->fontctx.cw = template_cw;
	if (digit_cw > ic->fontctx.cw) {
		ic->fontctx.cw = digit_cw;
	}
	if (ic->fontctx.cw < 1) {
		ic->fontctx.cw = 1;
	}

	/* Ensure a worst-case value field fits with 4px margin before the next divider. */
	value_w = imagettftextwidth(ic, ic->fontctx.ptsize, "999.99 YiB");
	while (10 * ic->fontctx.cw < value_w + 4) {
		ic->fontctx.cw++;
	}

	/* Body height with ascenders and descenders */
	err = imagettfbbox(ic, ic->fontctx.ptsize, 0.0, "Ayjp", brect);
	if (err != NULL) {
		fprintf(stderr, "%s \"%s\": %s\n", errprefix, ic->fontctx.ttfpath, err);
		imagefontcleanup();
		return 0;
	}
	ic->fontctx.ch = brect[1] - brect[7];
	if (ic->fontctx.ch < 1) {
		ic->fontctx.ch = 1;
	}
	ic->fontctx.ascent = -brect[7];
	ic->fontctx.scale = (double)ic->fontctx.cw / 6.0;

	err = imagettfbbox(ic, ic->fontctx.ptsize * ic->fontctx.title_scale, 0.0, "Ayjp", brect);
	if (err != NULL) {
		fprintf(stderr, "%s \"%s\": %s\n", errprefix, ic->fontctx.ttfpath, err);
		imagefontcleanup();
		return 0;
	}
	ic->fontctx.header_ch = brect[1] - brect[7];
	if (ic->fontctx.header_ch < 1) {
		ic->fontctx.header_ch = 1;
	}
	ic->fontctx.header_ascent = -brect[7];
	ic->fontctx.header_h = ic->fontctx.header_ch + imageuipx(ic, 16);
	if (ic->fontctx.header_h < 24) {
		ic->fontctx.header_h = 24;
	}

	err = imagettfbbox(ic, ic->fontctx.ptsize * ic->fontctx.axis_scale, 0.0, "Ayjp", brect);
	if (err != NULL) {
		fprintf(stderr, "%s \"%s\": %s\n", errprefix, ic->fontctx.ttfpath, err);
		imagefontcleanup();
		return 0;
	}
	ic->fontctx.axis_ch = brect[1] - brect[7];
	if (ic->fontctx.axis_ch < 1) {
		ic->fontctx.axis_ch = 1;
	}
	ic->fontctx.axis_ascent = -brect[7];

	ic->fontctx.axis_num5_w = imagettftextwidth(ic, ic->fontctx.ptsize * ic->fontctx.axis_scale, "99999");
	if (ic->fontctx.axis_num5_w < 1) {
		ic->fontctx.axis_num5_w = 1;
	}

	ic->lineheight = ic->fontctx.ch + 2;

	return 1;
}
#endif

int imagefontinit(IMAGECONTENT *ic, const int largefonts)
{
	ic->fontctx.header = gdFontGetGiant();
	ic->fontctx.footer = gdFontGetTiny();
	ic->fontctx.title_scale = 1.5;
	ic->fontctx.axis_scale = 0.83;
	ic->fontctx.ascent = 0;
	ic->fontctx.header_ascent = 0;
	ic->fontctx.axis_ascent = 0;
	ic->fontctx.axis_num5_w = 0;
	ic->fontctx.ttfpath[0] = '\0';
	ic->fontctx.ptsize = 0.0;

	if (largefonts) {
		ic->fontctx.body = gdFontGetLarge();
		ic->fontctx.axis = gdFontGetSmall();
		ic->fontctx.title = gdFontGetGiant();
	} else {
		ic->fontctx.body = gdFontGetSmall();
		ic->fontctx.axis = gdFontGetTiny();
		ic->fontctx.title = gdFontGetLarge();
	}

	if (cfg.fontfile[0] == '\0') {
		ic->fontctx.mode = FONT_BUILTIN;
		ic->fontctx.cw = ic->fontctx.body->w;
		ic->fontctx.ch = ic->fontctx.body->h;
		ic->fontctx.header_ch = ic->fontctx.header->h;
		ic->fontctx.axis_ch = ic->fontctx.axis->h;
		ic->fontctx.axis_num5_w = 5 * ic->fontctx.axis->w;
		ic->fontctx.header_h = 24;
		ic->fontctx.scale = (double)ic->fontctx.cw / 6.0;
		ic->lineheight = largefonts ? 16 : 12;
		return 1;
	}

#if HAVE_DECL_GDIMAGESTRINGFT
	if (access(cfg.fontfile, R_OK) != 0) {
		fprintf(stderr, "Error: Unable to read FontFile \"%s\": %s\n", cfg.fontfile, strerror(errno));
		return 0;
	}

	ic->fontctx.mode = FONT_TTF;
	strncpy_nt(ic->fontctx.ttfpath, cfg.fontfile, 512);
	ic->fontctx.ptsize = (double)cfg.fontsize;
	if (largefonts) {
		ic->fontctx.ptsize *= 1.5;
	}

	if (!imagettfinitmetrics(ic)) {
		return 0;
	}

	if (debug) {
		printf("TTF font: \"%s\" size %.1f cw %d ch %d scale %.3f lineheight %d header_h %d\n",
			   ic->fontctx.ttfpath, ic->fontctx.ptsize, ic->fontctx.cw, ic->fontctx.ch,
			   ic->fontctx.scale, ic->lineheight, ic->fontctx.header_h);
	}

	return 1;
#else
	fprintf(stderr, "Error: FontFile is set but libGD was built without FreeType/TTF support.\n");
	return 0;
#endif
}

void imagefontcleanup(void)
{
#if HAVE_DECL_GDIMAGESTRINGFT
	if (fontcache_ready) {
		gdFontCacheShutdown();
		fontcache_ready = 0;
	}
#endif
}

void imagestring(IMAGECONTENT *ic, const fontrole_t role, const int x, const int y, const char *text, const int color)
{
#if HAVE_DECL_GDIMAGESTRINGFT
	int brect[8], baseline_y, ascent;
	double ptsize;
	char *err;
#endif

	if (text == NULL || text[0] == '\0') {
		return;
	}

	if (imageroleusesbuiltin(ic, role)) {
		gdImageString(ic->im, imagerolefont(ic, role), x, y, (unsigned char *)text, color);
		return;
	}

#if HAVE_DECL_GDIMAGESTRINGFT
	switch (role) {
		case FONT_ROLE_AXIS:
			ascent = ic->fontctx.axis_ascent;
			break;
		case FONT_ROLE_TITLE:
		case FONT_ROLE_HEADER:
			ascent = ic->fontctx.header_ascent;
			break;
		case FONT_ROLE_BODY:
		case FONT_ROLE_FOOTER:
		default:
			ascent = ic->fontctx.ascent;
			break;
	}
	ptsize = imageroleptsize(ic, role);
	baseline_y = y + ascent;
	err = gdImageStringFT(ic->im, brect, color, ic->fontctx.ttfpath, ptsize, 0.0, x, baseline_y, (char *)text);
	if (err != NULL && debug) {
		printf("gdImageStringFT failed: %s\n", err);
	}
#else
	(void)x;
	(void)y;
	(void)color;
#endif
}

void imagestringup(IMAGECONTENT *ic, const fontrole_t role, const int x, const int y, const char *text, const int color)
{
#if HAVE_DECL_GDIMAGESTRINGFT
	int brect[8], ascent, pen_x;
	double ptsize;
	char *err;
#endif

	if (text == NULL || text[0] == '\0') {
		return;
	}

	if (imageroleusesbuiltin(ic, role)) {
		gdImageStringUp(ic->im, imagerolefont(ic, role), x, y, (unsigned char *)text, color);
		return;
	}

#if HAVE_DECL_GDIMAGESTRINGFT
	/* At angle π/2 glyphs extend left of the pen by roughly ascent; shift so
	 * the visual left edge matches gdImageStringUp's x anchor. */
	switch (role) {
		case FONT_ROLE_AXIS:
			ascent = ic->fontctx.axis_ascent;
			break;
		case FONT_ROLE_TITLE:
		case FONT_ROLE_HEADER:
			ascent = ic->fontctx.header_ascent;
			break;
		case FONT_ROLE_BODY:
		case FONT_ROLE_FOOTER:
		default:
			ascent = ic->fontctx.ascent;
			break;
	}
	ptsize = imageroleptsize(ic, role);
	pen_x = x + ascent;
	err = gdImageStringFT(ic->im, brect, color, ic->fontctx.ttfpath, ptsize, M_PI / 2.0, pen_x, y, (char *)text);
	if (err != NULL && debug) {
		printf("gdImageStringFT (vertical) failed: %s\n", err);
	}
#else
	(void)x;
	(void)y;
	(void)color;
#endif
}

int imageextrapx(const IMAGECONTENT *ic, const int extra)
{
	double t;

	if (extra == 0) {
		return 0;
	}

	if (ic->fontctx.mode == FONT_BUILTIN) {
		return ic->large * extra;
	}

	/* Map scale 1.0 (small cw=6) .. 8/6 (large cw=8) onto 0 .. extra. */
	t = (ic->fontctx.scale - 1.0) / (8.0 / 6.0 - 1.0);
	if (t < 0.0) {
		t = 0.0;
	}
	return (int)lrint(t * (double)extra);
}

/* Scale a design-time pixel (usually 1 or 2) with body point size.
 * Builtin fonts and default FontSize (12pt) → returns base unchanged.
 * Larger TTF → grows with ptsize/FONTSIZE (e.g. base=1 at 40pt → 3).
 * Uses ptsize rather than fontctx.scale (cw/6): TTF cell width is already
 * wider than the builtin baseline at 12pt, so scale would thicken too early.
 * Builtin large uses imageextrapx() for layout fattening, not this. */
int imageuipx(const IMAGECONTENT *ic, const int base)
{
	int n;

	if (base <= 0) {
		return 0;
	}

	if (ic->fontctx.mode == FONT_BUILTIN) {
		return base;
	}

	n = (int)lrint((double)base * ic->fontctx.ptsize / (double)FONTSIZE);
	if (n < base) {
		return base;
	}
	return n;
}

void imagedrawhline(IMAGECONTENT *ic, const int x1, const int x2, const int y, const int color)
{
	int t, y1, y2, xa, xb;

	t = imageuipx(ic, 1);
	/* Center on y; use t/2 so even thicknesses do not grow only downward. */
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

	/* Outer edge of the border sits on the given rectangle. */
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

/* Pixels from graph xpos to axis base (builtin historical GRAPH_AXIS_BASE). */
int graph_axis_left(const IMAGECONTENT *ic)
{
	if (ic->fontctx.mode == FONT_BUILTIN) {
		return GRAPH_AXIS_BASE;
	}

	/* 5-digit field + gap to y-axis; unit text sits at xpos in the left margin
	 * and uses the spare column vs 4-char scale values. */
	return ic->fontctx.axis_num5_w + GRAPH_AXIS_LABEL_GAP;
}

int graph_xpos_margin(const IMAGECONTENT *ic)
{
	if (ic->fontctx.mode == FONT_BUILTIN) {
		return 8 + imageextrapx(ic, 14);
	}
	return 8;
}

/* Non-plot width for 5-minute / percentile style graphs (left margin + chrome + right). */
int graph_extra_space(const IMAGECONTENT *ic)
{
	if (ic->fontctx.mode == FONT_BUILTIN) {
		return FIVEMINEXTRASPACE + imageextrapx(ic, 14);
	}
	/* Plot pad tracks scaled origin cross (cross + 1) so canvas grows with chrome. */
	return graph_xpos_margin(ic) + graph_axis_left(ic) + imageuipx(ic, GRAPH_AXIS_CROSS) + 1 + GRAPH_EXTRA_RIGHT;
}

/* Extra plot width matching the 23 hour-to-hour gaps (keeps bars aligned with axis). */
int hourly_plot_extrax(const IMAGECONTENT *ic)
{
	return HOURLY_HOUR_GAPS * imageextrapx(ic, 6);
}

/* Standalone hourly canvas width: left margin + axis gutter + plot + right pad. */
int hourly_graph_width(const IMAGECONTENT *ic)
{
	const int left = 12 + (ic->fontctx.mode == FONT_BUILTIN ? imageextrapx(ic, 14) : 0);
	const int extrax = hourly_plot_extrax(ic);
	const int pole_pad = imageextrapx(ic, 2);
	/* +pole_pad shifts hours right so widened poles clear the y-axis. */
	const int plot = HOURLY_PLOT_SPAN + extrax + pole_pad;
	/* 48 = HOURLY_CANVAS_BASE - 12 - GRAPH_AXIS_BASE - HOURLY_PLOT_SPAN.
	 * +pole_pad grows tip room past the widened rightmost pole. */
	const int right = (HOURLY_CANVAS_BASE - 12 - GRAPH_AXIS_BASE - HOURLY_PLOT_SPAN)
		+ imageextrapx(ic, 168) - imageextrapx(ic, 14) - extrax + pole_pad;

	return left + graph_axis_left(ic) + plot + right;
}

/* Numeric label on a horizontal scale line (TTF right-align + ascent center;
 * builtin keeps historical x/y via builtin_x / builtin_y). */
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
		imagestring(ic, FONT_ROLE_AXIS, axis_x - GRAPH_AXIS_LABEL_GAP - imagetextwidth(ic, FONT_ROLE_AXIS, val), label_y, val, ic->ctext);
	} else {
		imagestring(ic, FONT_ROLE_AXIS, builtin_x, builtin_y, val, ic->ctext);
	}
}

/* Vertical unit label; callers pass mode-specific x (anchors differ per graph). */
void graph_draw_axis_unit(IMAGECONTENT *ic, const int x_ttf, const int x_builtin, const int y, const char *text)
{
	if (ic->fontctx.mode == FONT_TTF) {
		imagestringup(ic, FONT_ROLE_AXIS, x_ttf, y, text, ic->ctext);
	} else {
		imagestringup(ic, FONT_ROLE_AXIS, x_builtin, y, text, ic->ctext);
	}
}

int imagetextwidth(IMAGECONTENT *ic, const fontrole_t role, const char *text)
{
	if (text == NULL || text[0] == '\0') {
		return 0;
	}

	if (imageroleusesbuiltin(ic, role)) {
		return ((int)strlen(text)) * imagerolefont(ic, role)->w;
	}

#if HAVE_DECL_GDIMAGESTRINGFT
	return imagettftextwidth(ic, imageroleptsize(ic, role), text);
#else
	return 0;
#endif
}

int imagefontwidth(IMAGECONTENT *ic, const fontrole_t role)
{
	if (imageroleusesbuiltin(ic, role)) {
		return imagerolefont(ic, role)->w;
	}

	switch (role) {
		case FONT_ROLE_AXIS:
			return (int)(ic->fontctx.cw * ic->fontctx.axis_scale + 0.5);
		case FONT_ROLE_TITLE:
		case FONT_ROLE_HEADER:
			return (int)(ic->fontctx.cw * ic->fontctx.title_scale + 0.5);
		case FONT_ROLE_BODY:
		case FONT_ROLE_FOOTER:
		default:
			return ic->fontctx.cw;
	}
}

int imagefontheight(IMAGECONTENT *ic, const fontrole_t role)
{
	if (imageroleusesbuiltin(ic, role)) {
		return imagerolefont(ic, role)->h;
	}

	switch (role) {
		case FONT_ROLE_AXIS:
			return ic->fontctx.axis_ch;
		case FONT_ROLE_TITLE:
		case FONT_ROLE_HEADER:
			return ic->fontctx.header_ch;
		case FONT_ROLE_BODY:
		case FONT_ROLE_FOOTER:
		default:
			return ic->fontctx.ch;
	}
}

/* Center role text vertically in [rect_top, rect_bottom] (TTF ink-band aware). */
static int imagecentery(IMAGECONTENT *ic, const fontrole_t role, const char *text, const int rect_top, const int rect_bottom)
{
	int text_h, y;

#if HAVE_DECL_GDIMAGESTRINGFT
	if (ic->fontctx.mode == FONT_TTF && !imageroleusesbuiltin(ic, role)) {
		int brect[8], ink_h, ink_top, ink_bot, ascent;
		char *err;
		double ptsize = imageroleptsize(ic, role);

		switch (role) {
			case FONT_ROLE_AXIS:
				ascent = ic->fontctx.axis_ascent;
				break;
			case FONT_ROLE_TITLE:
			case FONT_ROLE_HEADER:
				ascent = ic->fontctx.header_ascent;
				break;
			case FONT_ROLE_BODY:
			case FONT_ROLE_FOOTER:
			default:
				ascent = ic->fontctx.ascent;
				break;
		}

		/* Center the ascent→baseline band (ignore descenders) so cap-height
		 * text gets equal padding above/below in the header bar. */
		err = imagettfbbox(ic, ptsize, 0.0, text, brect);
		if (err == NULL) {
			int clamp = imageuipx(ic, 1);

			ink_h = -brect[7];
			if (ink_h < 1) {
				ink_h = ascent;
			}
			y = rect_top + (rect_bottom - rect_top - ink_h) / 2 - ascent - brect[7];
			ink_top = y + ascent + brect[7];
			ink_bot = y + ascent; /* baseline; descenders may extend below */
			if (ink_top < rect_top + clamp) {
				y += (rect_top + clamp) - ink_top;
				ink_bot += (rect_top + clamp) - ink_top;
			}
			if (ink_bot > rect_bottom - clamp) {
				y -= ink_bot - (rect_bottom - clamp);
			}
			return y;
		}
	}
#else
	(void)text;
#endif

	text_h = imagefontheight(ic, role);
	y = rect_top + (rect_bottom - rect_top - text_h) / 2;
	{
		int clamp = imageuipx(ic, 1);

		if (y < rect_top + clamp) {
			y = rect_top + clamp;
		}
		if (y + text_h > rect_bottom - clamp) {
			y = rect_bottom - text_h - clamp;
		}
	}
	return y;
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
			date_y = imagecentery(ic, FONT_ROLE_AXIS, datestring, rect_top, rect_bottom);
		}

		gdImageFilledRectangle(ic->im, inset, rect_top, width - 1 - inset, rect_bottom, ic->cheader);
		imagestring(ic, FONT_ROLE_HEADER, imageuipx(ic, 12), title_y, buffer, ic->cheadertitle);
	}

	/* date */
	if (!ic->showheader || ic->altdate) {
		int date_y_alt = height - imagefontheight(ic, FONT_ROLE_AXIS) - bottom_margin - imageextrapx(ic, 3);
		imagestring(ic, FONT_ROLE_AXIS, imageuipx(ic, 5) + ic->showedge * edge_t, date_y_alt, datestring, ic->cvnstat);
	} else {
		imagestring(ic, FONT_ROLE_AXIS, width - (imagetextwidth(ic, FONT_ROLE_AXIS, datestring) + imageuipx(ic, 12)), date_y, datestring, ic->cheaderdate);
	}

	/* generator */
	{
		const char *generator = "vnStat / Teemu Toivola";
		int generator_x = width - imagetextwidth(ic, FONT_ROLE_FOOTER, generator) - bottom_margin;
		int generator_y = height - imagefontheight(ic, FONT_ROLE_FOOTER) - bottom_margin;
		imagestring(ic, FONT_ROLE_FOOTER, generator_x, generator_y, generator, ic->cvnstat);
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
		x_cur = israte ? (x - 12) : x;

		/* [sq][gap][rx][sep][sq][gap][tx] — both labels share y */
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

void drawpercentilelegend(IMAGECONTENT *ic, const int x, const int y, const int mode, const uint64_t percentile)
{
	int color, xoffset = 0, sq, sq_y;
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
		xoffset = 18 + imageextrapx(ic, 6);
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
		snprintf(percentiletext, 64, "95th percentile: %s", gettrafficrate(percentile, 300, 0));
		imagestring(ic, FONT_ROLE_BODY, x_cur, y, percentiletext, ic->ctext);
		return;
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
	int l, pad;

	pad = imageextrapx(ic, 2);

	if (rx > 0) {
		l = (int)lrint(((double)rx / (double)max) * len);
		if (l > 0) {
			gdImageFilledRectangle(ic->im, x - pad, y + (len - l), x + 7, y + len, ic->crx);
		}
	}

	if (tx > 0) {
		l = (int)lrint(((double)tx / (double)max) * len);
		if (l > 0) {
			gdImageFilledRectangle(ic->im, x + 5, y + (len - l), x + 12 + pad, y + len, ic->ctx);
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
	int t, wing, half, tip_x, tip_y;
	gdPoint pts[3];

	t = imageuipx(ic, 1);
	wing = imageuipx(ic, 2);
	half = (t - 1) / 2 + wing;
	/* Tip on the integer center of the thick vline stroke. */
	tip_x = (x - t / 2) + (t - 1) / 2;
	/* One pixel past the axis end so the tip is the peak. */
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
	/* Tip on the integer center of the thick hline stroke. */
	tip_y = (y - t / 2) + (t - 1) / 2;
	/* One pixel past the axis end so the tip terminates the line. */
	tip_x = x + 1;

	pts[0].x = tip_x;
	pts[0].y = tip_y;
	pts[1].x = tip_x - imageuipx(ic, 3);
	pts[1].y = tip_y - half;
	pts[2].x = tip_x - imageuipx(ic, 3);
	pts[2].y = tip_y + half;
	gdImageFilledPolygon(ic->im, pts, 3, ic->ctext);
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
