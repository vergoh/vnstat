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

/* graph y-axis left chrome: built-in keeps a 36px advance; TTF sizes from 5 digits */
#define GRAPH_AXIS_BASE 36
#define GRAPH_AXIS_LABEL_GAP 4
#define GRAPH_AXIS_CROSS 4 /* design time overhang past origin; scale with imageuipx() */
#define GRAPH_AXIS_PLOT_PAD 5 /* GRAPH_AXIS_CROSS + 1; plot starts 1px past stem */
#define GRAPH_EXTRA_RIGHT 29 /* FIVEMINEXTRASPACE - 8 - GRAPH_AXIS_BASE - GRAPH_AXIS_PLOT_PAD */

/* hourly graph canvas: plot span from y-axis to rightmost hour column
 * HOURLY_PLOT_SPAN = leftmost_offset(13) + HOURLY_HOUR_GAPS * HOURLY_HOUR_STEP */
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
	FONT_ROLE_AXIS,     /* graph labels: Tiny / Small */
	FONT_ROLE_TIMESTAMP, /* header / alt date: Tiny / Small */
	FONT_ROLE_TITLE,    /* summary section titles: Large / Giant */
	FONT_ROLE_HEADER,   /* layoutinit title bar: always Giant */
	FONT_ROLE_FOOTER    /* credit line: always Tiny */
} fontrole_t;

typedef struct {
	int textx, offsetx;
	int d24, d37, d50; /* vertical divider x */
	int hline_right_rate; /* 65*cw+offsetx+2 */
	int hline_right_norate; /* 50*cw+offsetx-imageuipx(4) */
	/* TTF measured edges / header decimal anchors */
	int rx_edge, tx_edge, total_edge, rate_edge;
	int rx_dec, tx_dec, total_dec;
	int date_field_right, header_field_right;
	int rank_center; /* top list # column center; 0 if unused */
} ListColumns;

typedef struct {
	fontmode_t mode;
	gdFontPtr body, axis, title, header, footer;
	int cw, ch; /* body metrics for layout */
	int ascent; /* body: shared TTF baseline offset from y */
	int header_ascent; /* header role baseline offset */
	int title_ascent; /* title role baseline offset */
	int axis_ascent; /* axis role baseline offset */
	int timestamp_ascent; /* timestamp role baseline offset */
	int header_ch; /* measured header glyph height */
	int title_ch; /* measured title glyph height */
	int axis_ch; /* measured axis glyph height */
	int timestamp_ch; /* measured timestamp/date glyph height */
	int axis_num5_w; /* width of "99999" at axis size (TTF gutter) */
	int header_h; /* title bar height in pixels */
	char ttfpath[512];
	double ptsize; /* effective body point size after LargeFonts */
	double scale; /* cw / 6.0 relative to small built-in */
	double header_scale; /* header size ratio vs body */
	double title_scale; /* title size ratio vs body */
	double axis_scale; /* axis label size ratio vs body */
	double timestamp_scale; /* header/alt date size ratio vs body */
} IMAGEFONT;

typedef struct {
	gdImagePtr im;
	IMAGEFONT fontctx;
	interfaceinfo interface;
	int cbackground, cedge, cheader, cheadertitle, cheaderdate, ctext, cline, clinel, cpercentileline, cvnstat;
	int crx, crxd, ctx, ctxd, ctotal, cbgoffset, cbgoffsetmore, showheader, showedge, showlegend, altdate;
	int lineheight, large, invert, commonwidth;
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

#endif
