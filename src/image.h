#ifndef IMAGE_H
#define IMAGE_H

#include <gd.h>		  /* libgd2-dev libgd2 */
#include <gdfontt.h>  /* gdFontGetTiny() */
#include <gdfonts.h>  /* gdFontGetSmall() */
#include <gdfontmb.h> /* gdFontGetMediumBold() */
#include <gdfontl.h>  /* gdFontGetLarge() */
#include <gdfontg.h>  /* gdFontGetGiant() */
#include "dbsql.h"

/* rectangle size */
#define YBEGINOFFSET (-1)

#define FIVEMINHEIGHTOFFSET 4
#define FIVEMINWIDTHFULLPADDING 10
#define FIVEMINWIDTHPADDING 2
#define FIVEMINEXTRASPACE 78

#define PERCENTILEMINWIDTHFULLPADDING 10

#define SCALEMINPIXELS 25

/* Graph y-axis left chrome: builtin keeps a 36px advance; TTF sizes from 5 digits. */
#define GRAPH_AXIS_BASE 36
#define GRAPH_AXIS_LABEL_GAP 4
#define GRAPH_AXIS_CROSS 4 /* design-time overhang past origin; scale with imageuipx() */
#define GRAPH_AXIS_PLOT_PAD 5 /* GRAPH_AXIS_CROSS + 1; plot starts 1px past stem */
#define GRAPH_EXTRA_RIGHT 29 /* FIVEMINEXTRASPACE - 8 - GRAPH_AXIS_BASE - GRAPH_AXIS_PLOT_PAD */

/* Hourly graph canvas: plot span from y-axis to rightmost hour column.
 * HOURLY_PLOT_SPAN = leftmost_offset(13) + HOURLY_HOUR_GAPS * HOURLY_HOUR_STEP. */
#define HOURLY_CANVAS_BASE 500
#define HOURLY_HOUR_STEP 17
#define HOURLY_HOUR_COUNT 24
#define HOURLY_HOUR_GAPS (HOURLY_HOUR_COUNT - 1) /* 23 gaps between 24 hours */
#define HOURLY_PLOT_SPAN (440 - GRAPH_AXIS_BASE) /* 404 */
#define HOURLY_DASH_PAST 20 /* dashed grid ends this far past rightmost hour center */
#define HOURLY_AXIS_PAST 26 /* x-axis line ends this far past rightmost hour center */

typedef enum {
	FONT_BUILTIN = 0,
	FONT_TTF
} fontmode_t;

typedef enum {
	FONT_ROLE_BODY = 0, /* Small / Large */
	FONT_ROLE_AXIS,     /* Tiny / Small — graph labels + header date */
	FONT_ROLE_TITLE,    /* Large / Giant — summary section titles */
	FONT_ROLE_HEADER,   /* always Giant — layoutinit title bar */
	FONT_ROLE_FOOTER    /* always Tiny — credit line */
} fontrole_t;

typedef struct {
	fontmode_t mode;
	gdFontPtr body, axis, title, header, footer;
	int cw, ch; /* body metrics for layout */
	int ascent; /* body: shared TTF baseline offset from y */
	int header_ascent; /* title/header role baseline offset */
	int axis_ascent; /* axis role baseline offset */
	int header_ch; /* measured header/title glyph height */
	int axis_ch; /* measured axis/date glyph height */
	int axis_num5_w; /* width of "99999" at axis size (TTF gutter) */
	int header_h; /* title bar height in pixels */
	char ttfpath[512];
	double ptsize; /* effective body point size after LargeFonts */
	double scale; /* cw / 6.0 relative to small builtin */
	double title_scale; /* title/header size ratio vs body */
	double axis_scale; /* axis label size ratio vs body */
} IMAGEFONT;

typedef struct {
	gdImagePtr im;
	IMAGEFONT fontctx;
	interfaceinfo interface;
	int cbackground, cedge, cheader, cheadertitle, cheaderdate, ctext, cline, clinel, cpercentileline, cvnstat;
	int crx, crxd, ctx, ctxd, ctotal, cbgoffset, cbgoffsetmore, showheader, showedge, showlegend, altdate;
	int lineheight, large, invert;
	char headertext[65], databegin[18], dataend[18];
	time_t current;
} IMAGECONTENT;

typedef struct {
	time_t date;
	uint64_t rx, tx;
} HOURDATA;

void initimagecontent(IMAGECONTENT *ic);
void drawimage(IMAGECONTENT *ic);
#if HAVE_DECL_GD_NEAREST_NEIGHBOUR
void scaleimage(IMAGECONTENT *ic);
#endif
int drawhours(IMAGECONTENT *ic, const int xpos, const int ypos, const int israte);
void drawhourly(IMAGECONTENT *ic, const int israte);
void drawlist(IMAGECONTENT *ic, const char *listname);
void drawsummary(IMAGECONTENT *ic, const int layout, const int israte);
void drawsummary_alltime(IMAGECONTENT *ic, const int x, const int y);
void drawsummary_digest(IMAGECONTENT *ic, const int x, const int y, const char *mode);
void drawfivegraph(IMAGECONTENT *ic, const int israte, const int resultcount, const int height);
int drawfiveminutes(IMAGECONTENT *ic, const int xpos, const int ypos, const int israte, const int resultcount, const int height);
void draw95thpercentilegraph(IMAGECONTENT *ic, const int mode);
int drawpercentile(IMAGECONTENT *ic, const int mode, const int xpos, const int ypos, const int height, uint64_t *percentile);

#endif
