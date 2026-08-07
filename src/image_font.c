#include "common.h"
#include "misc.h"
#include "image_font.h"

static gdFontPtr imagerolefont(const IMAGECONTENT *ic, const fontrole_t role)
{
	switch (role) {
		case FONT_ROLE_AXIS:
		case FONT_ROLE_TIMESTAMP:
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
		case FONT_ROLE_TIMESTAMP:
			return ic->fontctx.ptsize * ic->fontctx.timestamp_scale;
		case FONT_ROLE_HEADER:
			return ic->fontctx.ptsize * ic->fontctx.header_scale;
		case FONT_ROLE_TITLE:
			return ic->fontctx.ptsize * ic->fontctx.title_scale;
		case FONT_ROLE_BODY:
		case FONT_ROLE_FOOTER:
		default:
			return ic->fontctx.ptsize;
	}
}

static int imageroleascent(const IMAGECONTENT *ic, const fontrole_t role)
{
	switch (role) {
		case FONT_ROLE_AXIS:
			return ic->fontctx.axis_ascent;
		case FONT_ROLE_TIMESTAMP:
			return ic->fontctx.timestamp_ascent;
		case FONT_ROLE_HEADER:
			return ic->fontctx.header_ascent;
		case FONT_ROLE_TITLE:
			return ic->fontctx.title_ascent;
		case FONT_ROLE_BODY:
		case FONT_ROLE_FOOTER:
		default:
			return ic->fontctx.ascent;
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
	const char *err;

	if (text == NULL || text[0] == '\0') {
		return 0;
	}

	err = imagettfbbox(ic, ptsize, 0.0, text, brect);
	if (err != NULL) {
		return 0;
	}

	/* pen-origin to right edge. Do not use brect[2]-brect[0]: a positive left
	 * bearing (common on digit-leading strings) would shrink the width and
	 * shift right-aligned values like "155.27 GiB" off the unit column */
	return brect[2];
}

static int imagettfinitmetrics(IMAGECONTENT *ic)
{
	int brect[8], template_cw, digit_cw, value_w;
	const char *err;
	const char *errprefix = "Error: Unable to use FontFile";

	if (gdFontCacheSetup() != 0) {
		fprintf(stderr, "Error: gdFontCacheSetup failed.\n");
		return 0;
	}
	fontcache_ready = 1;

	/* template-average cell width for space-padded layouts */
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

	/* ensure a worst-case value field fits with 4px margin before the next divider */
	value_w = imagettftextwidth(ic, ic->fontctx.ptsize, "999.99 YiB");
	while (10 * ic->fontctx.cw < value_w + 4) {
		ic->fontctx.cw++;
	}

	/* body height with ascenders and descenders */
	err = imagettfbbox(ic, ic->fontctx.ptsize, 0.0, TTF_HEIGHT_SAMPLE, brect);
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

	err = imagettfbbox(ic, ic->fontctx.ptsize * ic->fontctx.header_scale, 0.0, TTF_HEIGHT_SAMPLE, brect);
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

	err = imagettfbbox(ic, ic->fontctx.ptsize * ic->fontctx.title_scale, 0.0, TTF_HEIGHT_SAMPLE, brect);
	if (err != NULL) {
		fprintf(stderr, "%s \"%s\": %s\n", errprefix, ic->fontctx.ttfpath, err);
		imagefontcleanup();
		return 0;
	}
	ic->fontctx.title_ch = brect[1] - brect[7];
	if (ic->fontctx.title_ch < 1) {
		ic->fontctx.title_ch = 1;
	}
	ic->fontctx.title_ascent = -brect[7];

	err = imagettfbbox(ic, ic->fontctx.ptsize * ic->fontctx.axis_scale, 0.0, TTF_HEIGHT_SAMPLE, brect);
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

	err = imagettfbbox(ic, ic->fontctx.ptsize * ic->fontctx.timestamp_scale, 0.0, TTF_HEIGHT_SAMPLE, brect);
	if (err != NULL) {
		fprintf(stderr, "%s \"%s\": %s\n", errprefix, ic->fontctx.ttfpath, err);
		imagefontcleanup();
		return 0;
	}
	ic->fontctx.timestamp_ch = brect[1] - brect[7];
	if (ic->fontctx.timestamp_ch < 1) {
		ic->fontctx.timestamp_ch = 1;
	}
	ic->fontctx.timestamp_ascent = -brect[7];

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
	ic->fontctx.header_scale = cfg.fontscaleheader / 100.0;
	ic->fontctx.title_scale = cfg.fontscaletitle / 100.0;
	ic->fontctx.axis_scale = cfg.fontscaleaxis / 100.0;
	ic->fontctx.timestamp_scale = cfg.fontscaletimestamp / 100.0;
	ic->fontctx.ascent = 0;
	ic->fontctx.header_ascent = 0;
	ic->fontctx.title_ascent = 0;
	ic->fontctx.axis_ascent = 0;
	ic->fontctx.timestamp_ascent = 0;
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
		ic->fontctx.title_ch = ic->fontctx.title->h;
		ic->fontctx.axis_ch = ic->fontctx.axis->h;
		ic->fontctx.timestamp_ch = ic->fontctx.axis_ch;
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
	fprintf(stderr, "Error: FontFile is set but libGD lacks FreeType/TTF support.\n");
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
	int brect[8], baseline_y;
	double ptsize;
	const char *err;
#endif

	if (text == NULL || text[0] == '\0') {
		return;
	}

	if (imageroleusesbuiltin(ic, role)) {
		gdImageString(ic->im, imagerolefont(ic, role), x, y, (unsigned char *)text, color);
		return;
	}

#if HAVE_DECL_GDIMAGESTRINGFT
	ptsize = imageroleptsize(ic, role);
	baseline_y = y + imageroleascent(ic, role);
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

void imagestring_value_right(IMAGECONTENT *ic, const fontrole_t role, const int edge, const int y, const char *num, const char *unit, const int color)
{
	int suffix_x, body_x, num_x;
	size_t ulen, suffix_len, prefix_len;
	const char *suffix;
	char prefix[32];

	if (num == NULL || num[0] == '\0') {
		return;
	}

	if (unit == NULL || unit[0] == '\0') {
		/* no unit (estimate "--", rate "n/a"): sit in the number column, matching
		 * getvalue()'s blank unit field sized to getunitprefix(2) */
		const char *ref = getunitprefix(2);
		size_t reflen = strlen(ref);
		char refprefix[16];
		size_t prelen = (reflen > 0 && ref[reflen - 1] == 'B') ? reflen - 1 : reflen;

		if (prelen >= sizeof(refprefix)) {
			prelen = sizeof(refprefix) - 1;
		}
		memcpy(refprefix, ref, prelen);
		refprefix[prelen] = '\0';

		suffix_x = edge - imagetextwidth(ic, role, "B");
		body_x = (prelen > 0) ? suffix_x - imagetextwidth(ic, role, refprefix) : suffix_x;
		num_x = body_x - imagetextwidth(ic, role, " ") - imagetextwidth(ic, role, num);
		imagestring(ic, role, num_x, y, num, color);
		return;
	}

	/* Pin a shared trailing suffix so mixed units (MiB vs GiB, MiB/s vs GiB/s)
	 * share one right edge. Measuring whole unit strings independently still
	 * allows ±1px FreeType/libgd bbox drift between different prefixes. */
	ulen = strlen(unit);
	if (ulen >= 2 && strcmp(unit + ulen - 2, "/s") == 0) {
		suffix = "/s";
	} else if (unit[ulen - 1] == 'B') {
		suffix = "B";
	} else {
		suffix = NULL;
	}

	if (suffix == NULL) {
		suffix_x = edge - imagetextwidth(ic, role, unit);
		num_x = suffix_x - imagetextwidth(ic, role, " ") - imagetextwidth(ic, role, num);
		imagestring(ic, role, num_x, y, num, color);
		imagestring(ic, role, suffix_x, y, unit, color);
		return;
	}

	suffix_len = strlen(suffix);
	prefix_len = ulen - suffix_len;
	if (prefix_len >= sizeof(prefix)) {
		prefix_len = sizeof(prefix) - 1;
	}
	if (prefix_len > 0) {
		memcpy(prefix, unit, prefix_len);
		prefix[prefix_len] = '\0';
	} else {
		prefix[0] = '\0';
	}

	suffix_x = edge - imagetextwidth(ic, role, suffix);
	if (prefix_len > 0) {
		body_x = suffix_x - imagetextwidth(ic, role, prefix);
		imagestring(ic, role, body_x, y, prefix, color);
	} else {
		body_x = suffix_x;
	}
	imagestring(ic, role, suffix_x, y, suffix, color);

	num_x = body_x - imagetextwidth(ic, role, " ") - imagetextwidth(ic, role, num);
	imagestring(ic, role, num_x, y, num, color);
}

void imagestringup(IMAGECONTENT *ic, const fontrole_t role, const int x, const int y, const char *text, const int color)
{
#if HAVE_DECL_GDIMAGESTRINGFT
	int brect[8], pen_x;
	double ptsize;
	const char *err;
#endif

	if (text == NULL || text[0] == '\0') {
		return;
	}

	if (imageroleusesbuiltin(ic, role)) {
		gdImageStringUp(ic->im, imagerolefont(ic, role), x, y, (unsigned char *)text, color);
		return;
	}

#if HAVE_DECL_GDIMAGESTRINGFT
	/* at angle pi/2 (90 degrees) glyphs extend left of the pen by roughly ascent; shift so
	 * the visual left edge matches gdImageStringUp's x anchor */
	ptsize = imageroleptsize(ic, role);
	pen_x = x + imageroleascent(ic, role);
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

/* Scale a design time pixel (usually 1 or 2) with body point size.
 * Built-in fonts and default FontSize (12pt) returns base unchanged.
 * Larger TTF grows with ptsize/FONTSIZE (e.g. base=1 at 40pt -> 3).
 * Uses ptsize rather than fontctx.scale (cw/6): TTF cell width is already
 * wider than the built-in baseline at 12pt, so scale would thicken too early.
 * Built-in large uses imageextrapx() for layout fattening, not this. */
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

int imagetextwidth(const IMAGECONTENT *ic, const fontrole_t role, const char *text)
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
		case FONT_ROLE_TIMESTAMP:
			return (int)(ic->fontctx.cw * ic->fontctx.timestamp_scale + 0.5);
		case FONT_ROLE_HEADER:
			return (int)(ic->fontctx.cw * ic->fontctx.header_scale + 0.5);
		case FONT_ROLE_TITLE:
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
		case FONT_ROLE_TIMESTAMP:
			return ic->fontctx.timestamp_ch;
		case FONT_ROLE_HEADER:
			return ic->fontctx.header_ch;
		case FONT_ROLE_TITLE:
			return ic->fontctx.title_ch;
		case FONT_ROLE_BODY:
		case FONT_ROLE_FOOTER:
		default:
			return ic->fontctx.ch;
	}
}

/* center role text vertically in [rect_top, rect_bottom] (TTF ink-band aware) */
int imagecentery(IMAGECONTENT *ic, const fontrole_t role, const char *text, const int rect_top, const int rect_bottom)
{
	int text_h, y;

#if HAVE_DECL_GDIMAGESTRINGFT
	if (ic->fontctx.mode == FONT_TTF && !imageroleusesbuiltin(ic, role)) {
		int brect[8], ink_h, ink_top, ink_bot, ascent;
		const char *err;
		double ptsize = imageroleptsize(ic, role);

		ascent = imageroleascent(ic, role);

		/* center the ascent→baseline band (ignore descenders) so cap-height
		 * text gets equal padding above/below in the header bar */
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

