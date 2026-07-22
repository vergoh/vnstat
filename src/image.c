#include "common.h"
#include "dbsql.h"
#include "misc.h"
#include "percentile.h"
#include "image.h"
#include "image_support.h"
#include "vnstati.h"

void initimagecontent(IMAGECONTENT *ic)
{
	ic->im = NULL;
	imagefontinit(ic, 0);
	ic->lineheight = 12;
	ic->large = 0;
	ic->invert = 0;
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
			drawfivegraph(ic, cfg.hourlyrate, cfg.fivegresultcount, cfg.fivegheight);
			break;
		case 130:
		case 131:
		case 132:
			draw95thpercentilegraph(ic, cfg.qmode - 130);
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

int drawhours(IMAGECONTENT *ic, const int xpos, const int ypos, const int israte)
{
	int i, tmax = 0, s = 0, step, prev = 0, diff = 0, chour, cross;
	int x = xpos, y = ypos, extrax = 0, extray = 0, xt = 0;
	int axis_x, axis_y, axis_top, axis_right, dash_right, tick_left, tick_right;
	int hour_gap_extra, pole_pad, hour_step, left_grow, dash_past, axis_past;
	double ratediv;
	uint64_t max = 1, scaleunit = 0;
	char buffer[32];
	const struct tm *d;
	dbdatalist *datalist = NULL, *datalist_i = NULL;
	dbdatalistinfo datainfo;
	HOURDATA hourdata[24];

	for (i = 0; i < 24; i++) {
		hourdata[i].rx = hourdata[i].tx = 0;
		hourdata[i].date = 0;
	}

	if (cfg.hourlygmode == 0) {
		buffer[0] = '\0';
	} else {
		snprintf(buffer, 32, "today");
	}

	if (!db_getdata_range(&datalist, &datainfo, ic->interface.name, "hour", 24, buffer, "") || datainfo.count == 0) {
		imagestring(ic, FONT_ROLE_BODY, x + (28 * ic->fontctx.cw), y + 54 - imageextrapx(ic, 18), "no hour data available", ic->ctext);
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
		if (israte) {
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
						hourdata[i].rx *= 8;
						hourdata[i].tx *= 8;
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

	if (ic->fontctx.mode == FONT_BUILTIN) {
		x += imageextrapx(ic, 14);
	}
	/* Plot width extra must match sum of hour-gap extras so bars stay aligned with the axis. */
	hour_gap_extra = imageextrapx(ic, 6);
	hour_step = HOURLY_HOUR_STEP + hour_gap_extra;
	extrax = hourly_plot_extrax(ic);
	pole_pad = imageextrapx(ic, 2);
	extray = imageextrapx(ic, 35);
	/* TTF: grow left inset and tip past with point size (0 delta at FontSize 12). */
	if (ic->fontctx.mode == FONT_TTF) {
		left_grow = imageuipx(ic, 13) - 13;
		dash_past = imageuipx(ic, HOURLY_DASH_PAST);
		axis_past = imageuipx(ic, HOURLY_AXIS_PAST);
	} else {
		left_grow = 0;
		dash_past = HOURLY_DASH_PAST;
		axis_past = HOURLY_AXIS_PAST;
	}

	/* scale values */
	scaleunit = getscale(max, israte);

	s = (int)lrint(((double)scaleunit / (double)max) * (124 + extray));
	if (s < SCALEMINPIXELS) {
		step = 2;
	} else {
		step = 1;
	}

	xt = x + graph_axis_left(ic);
	cross = imageuipx(ic, GRAPH_AXIS_CROSS);
	/* Hours are shifted right by pole_pad (+ left_grow for TTF) so leftmost poles clear
	 * the y-axis; tip room also grows so rightmost poles do not overrun the axis / grid. */
	dash_right = xt + HOURLY_PLOT_SPAN + extrax + pole_pad + left_grow + dash_past + pole_pad;
	axis_right = xt + HOURLY_PLOT_SPAN + extrax + pole_pad + left_grow + axis_past + pole_pad;

	for (i = step; i * s <= (124 + extray + cross); i = i + step) {
		const char *val;
		int line_y;

		line_y = y + 124 - (i * s);
		imagedrawdashedhline(ic, xt + graph_stroke_half(ic), dash_right, line_y, ic->cline);
		imagedrawdashedhline(ic, xt + graph_stroke_half(ic), dash_right, y + 124 - prev - (step * s) / 2, ic->clinel);
		val = getimagevalue(scaleunit * (unsigned int)i, 3, israte);
		graph_draw_axis_value(ic, xt, line_y, val, x + 16 - imageextrapx(ic, 3), line_y - 3 - imageextrapx(ic, 3));
		prev = i * s;
	}
	if ((prev + (step * s) / 2) <= (124 + extray + cross)) {
		imagedrawdashedhline(ic, xt + graph_stroke_half(ic), dash_right, y + 124 - prev - (step * s) / 2, ic->clinel);
	}

	/* scale text */
	graph_draw_axis_unit(ic, x, x - 2 - imageextrapx(ic, 14), y + 58 + (israte * 10) - (extray / 2), getimagescale(scaleunit * (unsigned int)step, israte));

	/* axis */
	axis_x = xt;
	axis_y = y + 124;
	axis_top = y - 10 - extray;
	imagedrawhline(ic, xt - cross, axis_right, axis_y, ic->ctext);
	imagedrawvline(ic, axis_x, axis_top, axis_y + cross, ic->ctext);

	/* Rightmost hour column: plot span + gap extras + left pole clearance */
	xt = xt + HOURLY_PLOT_SPAN + extrax + pole_pad + left_grow;

	/* keep alignment when midnight line isn't shown s*/
	if (cfg.hourlygmode || tmax - 23 == 0) {
		xt--;
	}

	/* x-axis values and poles */
	for (i = 0; i < 24; i++) {
		if (cfg.hourlygmode == 0) {
			s = tmax - i;
			if (s < 0) {
				s += 24;
			}
		} else {
			s = 23 - i;
		}
		snprintf(buffer, 32, "%02d", s);
		if (hourdata[s].date == 0) {
			chour = ic->cline;
		} else {
			chour = ic->ctext;
		}
		if (ic->fontctx.mode == FONT_TTF) {
			/* Center "HH" on pole-pair midpoint so digits sit under rx / tx. */
			imagestring(ic, FONT_ROLE_AXIS, xt + imageuipx(ic, 4) - imagetextwidth(ic, FONT_ROLE_AXIS, buffer) / 2, y + 124 + imageuipx(ic, 8), buffer, chour);
		} else {
			imagestring(ic, FONT_ROLE_AXIS, xt, y + 128, buffer, chour);
		}
		drawpoles(ic, xt - 2, y - extray, 124 + extray, hourdata[s].rx, hourdata[s].tx, max);
		if (s == 0 && i != 23) {
			/* midnight line — stop above the thick x-axis band */
			imagedrawvline(ic, xt - 5 - imageextrapx(ic, 3), y - 5 - extray, axis_y - graph_stroke_half(ic) - 1, ic->clinel);
			xt--;
		}
		xt = xt - hour_step;
	}

	/* Axes + arrows last so poles/grid cannot nick the strokes. */
	imagedrawhline(ic, axis_x - cross, axis_right, axis_y, ic->ctext);
	imagedrawvline(ic, axis_x, axis_top, axis_y + cross, ic->ctext);
	drawarrowup(ic, axis_x, axis_top);
	drawarrowright(ic, axis_right, axis_y);

	/* Hour ticks on the axis after the final redraw so missing-data hours dim
	 * that segment (ctext ticks are a no-op). Drawing below the thick stroke
	 * made a second incomplete rule at large TTF sizes. */
	xt = axis_x + HOURLY_PLOT_SPAN + extrax + pole_pad + left_grow;
	if (cfg.hourlygmode || tmax - 23 == 0) {
		xt--;
	}
	for (i = 0; i < 24; i++) {
		if (cfg.hourlygmode == 0) {
			s = tmax - i;
			if (s < 0) {
				s += 24;
			}
		} else {
			s = 23 - i;
		}
		if (hourdata[s].date == 0) {
			chour = ic->cline;
		} else {
			chour = ic->ctext;
		}
		/* Keep hour ticks from extending past the axis end (and over the arrow). */
		tick_left = xt - cross - imageextrapx(ic, 3);
		tick_right = xt + imageuipx(ic, 12) + imageextrapx(ic, 3);
		if (tick_right > axis_right) {
			tick_right = axis_right;
		}
		imagedrawhline(ic, tick_left, tick_right, axis_y, chour);
		if (s == 0 && i != 23) {
			xt--;
		}
		xt = xt - hour_step;
	}

	return 1;
}

void drawhourly(IMAGECONTENT *ic, const int israte)
{
	int width, height, headermod = 0, header_extra = 0;
	int ypos, axis_top_base, min_axis_top, graph_left;

	graph_left = hourly_graph_left(ic);
	width = hourly_graph_width(ic);
	height = 200 + imageextrapx(ic, 48);

	if (!ic->showheader) {
		headermod = ic->fontctx.header_h + imageuipx(ic, 2);
		height -= ic->fontctx.header_h - 2;
	} else {
		/* axis_top = ypos - 10 - extray; keep the up-arrow below the header.
		 * imageextrapx(40)-imageextrapx(35) alone does not track header_h. */
		ypos = 46 + imageextrapx(ic, 40);
		axis_top_base = ypos - 10 - imageextrapx(ic, 35);
		min_axis_top = ic->fontctx.header_h + imageuipx(ic, 8);
		if (ic->fontctx.mode == FONT_TTF) {
			min_axis_top += ic->fontctx.axis_ch / 2;
		}
		header_extra = min_axis_top - axis_top_base;
		if (header_extra < 0) {
			header_extra = 0;
		}
		height += header_extra;
	}

	imageinit(ic, width, height);
	layoutinit(ic, " / hourly", width, height);

	if (drawhours(ic, graph_left, 46 + header_extra - headermod + imageextrapx(ic, 40), israte)) {
		if (ic->fontctx.mode == FONT_TTF) {
			drawlegend(ic, width / 2 - imageextrapx(ic, 10), height - ic->showedge - ic->fontctx.ch * 3 / 2, 0);
		} else {
			drawlegend(ic, width / 2 - imageextrapx(ic, 10), 183 - headermod + imageextrapx(ic, 46), 0);
		}
	}
}

typedef struct {
	int textx, offsetx;
	int d24, d37, d50; /* vertical divider x */
	int hline_right_rate; /* 65*cw+offsetx+2 */
	int hline_right_norate; /* 50*cw+offsetx-imageuipx(4) */
	/* TTF measured edges / header decimal anchors */
	int rx_edge, tx_edge, total_edge, rate_edge;
	int rx_dec, tx_dec, total_dec;
	int date_field_right, header_field_right;
} ListColumns;

static void listcolumns_init(IMAGECONTENT *ic, const int textx, const int offsetx, ListColumns *cols)
{
	const int cw = ic->fontctx.cw;

	cols->textx = textx;
	cols->offsetx = offsetx;
	cols->d24 = textx + (24 * cw) + offsetx;
	cols->d37 = textx + (37 * cw) + offsetx;
	cols->d50 = textx + (50 * cw) + offsetx;
	cols->hline_right_rate = textx + (65 * cw) + offsetx + 2;
	cols->hline_right_norate = textx + (50 * cw) + offsetx - imageuipx(ic, 4);

	cols->rx_edge = cols->tx_edge = cols->total_edge = cols->rate_edge = 0;
	cols->rx_dec = cols->tx_dec = cols->total_dec = 0;
	cols->date_field_right = cols->header_field_right = 0;

	if (ic->fontctx.mode == FONT_TTF) {
		const int colpad = imageuipx(ic, 8);
		const char *sample = "00.00 GiB";
		int sample_w, prefix_w;

		cols->rx_edge = cols->d24 - colpad;
		cols->tx_edge = cols->d37 - colpad;
		cols->total_edge = cols->d50 - colpad;
		cols->rate_edge = textx + (65 * cw) + offsetx - colpad;
		cols->date_field_right = textx + 10 * cw;
		cols->header_field_right = textx + 9 * cw;

		sample_w = imagetextwidth(ic, FONT_ROLE_BODY, sample);
		prefix_w = imagetextwidth(ic, FONT_ROLE_BODY, "00");
		cols->rx_dec = cols->rx_edge - sample_w + prefix_w;
		cols->tx_dec = cols->tx_edge - sample_w + prefix_w;
		cols->total_dec = cols->total_edge - sample_w + prefix_w;
	}
}

static int list_bar_y(const IMAGECONTENT *ic, const int texty)
{
	if (ic->fontctx.mode == FONT_TTF) {
		return texty;
	}
	return texty + 4;
}

static int list_header_rule_y(const IMAGECONTENT *ic, const int texty)
{
	if (ic->fontctx.mode == FONT_TTF) {
		return texty + ic->fontctx.ch + (ic->lineheight + imageuipx(ic, 8) - ic->fontctx.ch) / 2;
	}
	return texty + ic->lineheight + 4;
}

static int list_mid_rule_y(const IMAGECONTENT *ic, const int texty)
{
	if (ic->fontctx.mode == FONT_TTF) {
		return texty - ic->lineheight + ic->fontctx.ch + (ic->lineheight + imageuipx(ic, 8) - ic->fontctx.ch) / 2;
	}
	return texty + 5 - imageextrapx(ic, 2);
}

static void list_draw_hline(IMAGECONTENT *ic, const ListColumns *cols, const int y, const int withrate)
{
	int x2 = withrate ? cols->hline_right_rate : cols->hline_right_norate;
	imagedrawhline(ic, cols->textx + imageuipx(ic, 2), x2, y, ic->cline);
}

static void list_draw_vdividers(IMAGECONTENT *ic, const ListColumns *cols, const int y1, const int y2, const int withrate)
{
	imagedrawvline(ic, cols->d24, y1, y2, ic->cline);
	imagedrawvline(ic, cols->d37, y1, y2, ic->cline);
	if (withrate) {
		imagedrawvline(ic, cols->d50, y1, y2, ic->cline);
	}
}

void drawlist(IMAGECONTENT *ic, const char *listname)
{
	ListType listtype = LT_None;
	ListColumns cols;
	int textx, texty, offsetx = 0;
	int width, height, headermod, i = 1, liney, mid_y, v_top, rowcount = 0;
	int estimateavailable = 0, estimatevisible = 0, monthrotatenotevisible = 0;
	int32_t limit;
	uint64_t e_rx = 0, e_tx = 0, e_secs;
	char buffer[512], datebuff[16], daybuff[16], monthrotatenote[96];
	char stampformat[64], titlename[16], colname[8];
	char rxbuf[64], txbuf[64], totalbuf[64], ratebuf[64];
	const struct tm *d;
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
		offsetx = 5 * ic->fontctx.cw;
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
		getestimates(&e_rx, &e_tx, listtype, ic->interface.updated, ic->interface.created, &datalist);
		if ((cfg.estimatestyle > 0 || cfg.barshowsrate > 0) && e_rx + e_tx > datainfo.max) {
			datainfo.max = e_rx + e_tx;
		}
		estimateavailable = 1;
		if (cfg.estimatevisible && (listtype == LT_Day || listtype == LT_Month || listtype == LT_Year)) {
			estimatevisible = 1;
		}
	}

	if (listtype == LT_Month && ismonthrotatenoteneeded()) {
		monthrotatenotevisible = 1;
		getmonthrotatenote(monthrotatenote, sizeof(monthrotatenote));
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

	width = 83 * ic->fontctx.cw + imageuipx(ic, 2) + imageextrapx(ic, 2);
	height = 62 + (ic->fontctx.header_h - 24) + 3 * ic->lineheight;

	// less space needed when no estimate or sum is shown
	if (!estimatevisible && !(strlen(ic->dataend) > 0 && datainfo.count > 1 && listtype != LT_Top)) {
		height = 62 + (ic->fontctx.header_h - 24) + 2 * ic->lineheight;
	}

	// exception for 5min and Hour when having sum shown
	if ((listtype == LT_5min || listtype == LT_Hour) && datainfo.count > 1 && strlen(ic->dataend) > 0) {
		height = 62 + (ic->fontctx.header_h - 24) + 3 * ic->lineheight;
	}

	if (ismonthrotatenoteneeded()) {
		height += ic->lineheight * 2;
	}

	height += (ic->lineheight + cfg.linespaceadjust) * rowcount - cfg.linespaceadjust;

	// "no data available"
	if (!datainfo.count) {
		height = 98 + imageextrapx(ic, 12) + (ic->fontctx.header_h - 24);
		monthrotatenotevisible = 0;
	}

	if (!ic->showheader) {
		headermod = ic->fontctx.header_h + imageuipx(ic, 2);
		height -= ic->fontctx.header_h - 2;
	} else {
		headermod = 0;
	}

	snprintf(buffer, 512, " / %s", titlename);

	imageinit(ic, width, height);
	layoutinit(ic, buffer, width, height);

	if (datainfo.count) {
		if (listtype == LT_Top) {
			if (cfg.ostyle <= 2) {
				drawlegend(ic, 66 * ic->fontctx.cw + 2, ic->fontctx.header_h + 16 - headermod, 0);
			}
			current = time(NULL);
			d = localtime(&current);
			strftime(daybuff, 16, stampformat, d);
		} else { // everything else
			if (cfg.ostyle > 2) {
				if (estimateavailable && cfg.barshowsrate) {
					drawlegend(ic, 72 * ic->fontctx.cw, ic->fontctx.header_h + 16 - headermod, 1);
				} else {
					drawlegend(ic, 72 * ic->fontctx.cw, ic->fontctx.header_h + 16 - headermod, 0);
				}
			} else {
				drawlegend(ic, 64 * ic->fontctx.cw + 1, ic->fontctx.header_h + 16 - headermod, 0);
			}
		}
	}

	textx = 10;
	texty = ic->fontctx.header_h + 16 - headermod;
	listcolumns_init(ic, textx, offsetx, &cols);

	/* column headers */
	if (ic->fontctx.mode == FONT_TTF) {
		if (listtype == LT_Top) {
			imagestring(ic, FONT_ROLE_BODY, textx, texty, "   #      day", ic->ctext);
		} else {
			imagestring(ic, FONT_ROLE_BODY, cols.header_field_right - imagetextwidth(ic, FONT_ROLE_BODY, colname), texty, colname, ic->ctext);
		}
		imagestring(ic, FONT_ROLE_BODY, cols.rx_dec, texty, "rx", ic->ctext);
		imagestring(ic, FONT_ROLE_BODY, cols.tx_dec, texty, "tx", ic->ctext);
		imagestring(ic, FONT_ROLE_BODY, cols.total_dec - imagetextwidth(ic, FONT_ROLE_BODY, "t"), texty, "total", ic->ctext);
		if (cfg.ostyle > 2) {
			imagestring(ic, FONT_ROLE_BODY, cols.rate_edge - imagetextwidth(ic, FONT_ROLE_BODY, "avg. rate"), texty, "avg. rate", ic->ctext);
		}
	} else {
		if (listtype == LT_Top) {
			snprintf(buffer, 512, "   #      day        rx           tx          total");
		} else {
			snprintf(buffer, 512, " %8s       rx           tx          total", colname);
		}
		if (cfg.ostyle > 2) {
			strcat(buffer, "       avg. rate");
		}
		imagestring(ic, FONT_ROLE_BODY, textx, texty, buffer, ic->ctext);
	}

	liney = list_header_rule_y(ic, texty);
	list_draw_hline(ic, &cols, liney, cfg.ostyle > 2);
	texty += ic->lineheight + imageuipx(ic, 8);
	/* Top of vdividers: into the column-header row (day/rx/tx/total), above the rule. */
	v_top = texty - imageuipx(ic, 6) - ic->lineheight;

	/* End vdividers on the mid rule so they meet that hline. Avoid imageextrapx()
	 * for the end Y: for TTF it grows with cw and leaves a gap. */
	if (datainfo.count) {
		mid_y = list_mid_rule_y(ic, texty + ((ic->lineheight + cfg.linespaceadjust) * rowcount) - cfg.linespaceadjust);
		list_draw_vdividers(ic, &cols, v_top, mid_y, cfg.ostyle > 2);
	}

	while (datalist_i != NULL) {
		int bar_y;

		d = localtime(&datalist_i->timestamp);

		if (listtype == LT_5min || listtype == LT_Hour) {
			strftime(datebuff, 16, cfg.dformat, d);
			if (strcmp(daybuff, datebuff) != 0) {
				snprintf(buffer, 32, " %s", datebuff);
				imagestring(ic, FONT_ROLE_BODY, textx, texty, buffer, ic->ctext);
				texty += ic->lineheight + cfg.linespaceadjust;
				strcpy(daybuff, datebuff);
			}
		}

		if (ic->fontctx.mode == FONT_TTF) {
			if (listtype == LT_Top) {
				int short_stamp = (strftime(datebuff, 16, stampformat, d) <= 8);

				if (strcmp(datebuff, daybuff) == 0) {
					int pad2 = imageuipx(ic, 2);

					if (cfg.ostyle > 2) {
						gdImageFilledRectangle(ic->im, textx + pad2, texty + pad2, textx + (65 * ic->fontctx.cw) + offsetx + pad2, texty + ic->fontctx.ch - pad2, ic->cbgoffset);
					} else {
						gdImageFilledRectangle(ic->im, textx + pad2, texty + pad2, textx + (50 * ic->fontctx.cw) + offsetx - imageuipx(ic, 4), texty + ic->fontctx.ch - pad2, ic->cbgoffset);
					}
				}
				if (short_stamp) {
					snprintf(buffer, 32, "  %2d", i);
					imagestring(ic, FONT_ROLE_BODY, textx, texty, buffer, ic->ctext);
					imagestring(ic, FONT_ROLE_BODY, textx + 15 * ic->fontctx.cw - imagetextwidth(ic, FONT_ROLE_BODY, datebuff), texty, datebuff, ic->ctext);
				} else {
					snprintf(buffer, 32, "  %2d  %-*s", i, getpadding(11, datebuff), datebuff);
					imagestring(ic, FONT_ROLE_BODY, textx, texty, buffer, ic->ctext);
				}
			} else {
				if (strftime(datebuff, 16, stampformat, d) <= 8) {
					imagestring(ic, FONT_ROLE_BODY, cols.date_field_right - imagetextwidth(ic, FONT_ROLE_BODY, datebuff), texty, datebuff, ic->ctext);
				} else {
					snprintf(buffer, 32, " %-*s", getpadding(11, datebuff), datebuff);
					imagestring(ic, FONT_ROLE_BODY, textx, texty, buffer, ic->ctext);
				}
			}

			strncpy_nt(rxbuf, getvalue(datalist_i->rx, 10, RT_Normal), 64);
			imagestring(ic, FONT_ROLE_BODY, cols.rx_edge - imagetextwidth(ic, FONT_ROLE_BODY, rxbuf), texty, rxbuf, ic->ctext);
			strncpy_nt(txbuf, getvalue(datalist_i->tx, 10, RT_Normal), 64);
			imagestring(ic, FONT_ROLE_BODY, cols.tx_edge - imagetextwidth(ic, FONT_ROLE_BODY, txbuf), texty, txbuf, ic->ctext);
			strncpy_nt(totalbuf, getvalue(datalist_i->rx + datalist_i->tx, 10, RT_Normal), 64);
			imagestring(ic, FONT_ROLE_BODY, cols.total_edge - imagetextwidth(ic, FONT_ROLE_BODY, totalbuf), texty, totalbuf, ic->ctext);
			if (cfg.ostyle > 2) {
				if (datalist_i->next == NULL && issametimeslot(listtype, datalist_i->timestamp, ic->interface.updated)) {
					e_secs = getperiodseconds(listtype, datalist_i->timestamp, ic->interface.updated, ic->interface.created, 1);
				} else {
					e_secs = getperiodseconds(listtype, datalist_i->timestamp, ic->interface.updated, ic->interface.created, 0);
				}
				strncpy_nt(ratebuf, gettrafficrate(datalist_i->rx + datalist_i->tx, (time_t)e_secs, 14), 64);
				imagestring(ic, FONT_ROLE_BODY, cols.rate_edge - imagetextwidth(ic, FONT_ROLE_BODY, ratebuf), texty, ratebuf, ic->ctext);
			}
		} else {
			if (listtype == LT_Top) {
				if (strftime(datebuff, 16, stampformat, d) <= 8) {
					snprintf(buffer, 32, "  %2d   %*s", i, getpadding(8, datebuff), datebuff);
					strcat(buffer, "   ");
				} else {
					snprintf(buffer, 32, "  %2d  %-*s ", i, getpadding(11, datebuff), datebuff);
				}
				if (strcmp(datebuff, daybuff) == 0) {
					int pad2 = imageuipx(ic, 2);

					if (cfg.ostyle > 2) {
						gdImageFilledRectangle(ic->im, textx + pad2, texty + pad2, textx + (65 * ic->fontctx.cw) + offsetx + pad2, texty + ic->fontctx.ch - pad2, ic->cbgoffset);
					} else {
						gdImageFilledRectangle(ic->im, textx + pad2, texty + pad2, textx + (50 * ic->fontctx.cw) + offsetx - imageuipx(ic, 4), texty + ic->fontctx.ch - pad2, ic->cbgoffset);
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
					e_secs = getperiodseconds(listtype, datalist_i->timestamp, ic->interface.updated, ic->interface.created, 1);
				} else {
					e_secs = getperiodseconds(listtype, datalist_i->timestamp, ic->interface.updated, ic->interface.created, 0);
				}
				strncat(buffer, gettrafficrate(datalist_i->rx + datalist_i->tx, (time_t)e_secs, 14), 32);
			}
			imagestring(ic, FONT_ROLE_BODY, textx, texty, buffer, ic->ctext);
		}

		bar_y = list_bar_y(ic, texty);
		if (listtype == LT_Top) {
			if (cfg.ostyle > 2) {
				drawbar(ic, textx + (71 * ic->fontctx.cw) + 2, bar_y, 9 * ic->fontctx.cw - 1, datalist_i->rx, datalist_i->tx, datainfo.max, 0);
			} else {
				drawbar(ic, textx + (56 * ic->fontctx.cw), bar_y, 23 * ic->fontctx.cw + 3, datalist_i->rx, datalist_i->tx, datainfo.max, 0);
			}
		} else {
			if (cfg.ostyle > 2) {
				if (datalist_i->next == NULL && estimateavailable && cfg.barshowsrate) {
					drawbar(ic, textx + (67 * ic->fontctx.cw) - 2, bar_y, 13 * ic->fontctx.cw + 1, e_rx, e_tx, datainfo.max, 0);
				} else {
					drawbar(ic, textx + (67 * ic->fontctx.cw) - 2, bar_y, 13 * ic->fontctx.cw + 1, datalist_i->rx, datalist_i->tx, datainfo.max, 0);
				}
			} else {
				drawbar(ic, textx + (51 * ic->fontctx.cw) - 2, bar_y, 28 * ic->fontctx.cw + 3, datalist_i->rx, datalist_i->tx, datainfo.max, 0);
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
		i = 17 * ic->fontctx.cw;
		if (cfg.ostyle > 2) {
			i += 8 * ic->fontctx.cw;
		}
		imagestring(ic, FONT_ROLE_BODY, textx + i, texty, "no data available", ic->ctext);
		texty += ic->lineheight;
	}

	mid_y = list_mid_rule_y(ic, texty);
	list_draw_hline(ic, &cols, mid_y, cfg.ostyle > 2);
	if (!datainfo.count) {
		list_draw_vdividers(ic, &cols, v_top, mid_y, cfg.ostyle > 2);
	}

	buffer[0] = '\0';

	if (estimatevisible) {
		int bar_y = list_bar_y(ic, texty - ic->lineheight);
		int footer_end;

		if (cfg.estimatestyle) {
			if (cfg.ostyle > 2) {
				drawbar(ic, textx + (67 * ic->fontctx.cw) - 2, bar_y, 13 * ic->fontctx.cw + 1, e_rx, e_tx, datainfo.max, 1);
				drawbar(ic, textx + (67 * ic->fontctx.cw) - 2, bar_y, 13 * ic->fontctx.cw + 1, datalist_i->rx, datalist_i->tx, datainfo.max, 0);
			} else {
				drawbar(ic, textx + (51 * ic->fontctx.cw) - 2, bar_y, 28 * ic->fontctx.cw + 3, e_rx, e_tx, datainfo.max, 1);
				drawbar(ic, textx + (51 * ic->fontctx.cw) - 2, bar_y, 28 * ic->fontctx.cw + 3, datalist_i->rx, datalist_i->tx, datainfo.max, 0);
			}
		}

		texty += imageuipx(ic, 8);
		if (ic->fontctx.mode == FONT_TTF) {
			int date_right;

			strncpy_nt(rxbuf, getvalue(e_rx, 10, RT_Estimate), 64);
			strncpy_nt(txbuf, getvalue(e_tx, 10, RT_Estimate), 64);
			strncpy_nt(totalbuf, getvalue(e_rx + e_tx, 10, RT_Estimate), 64);

			if (strlen(datebuff) <= 8) {
				date_right = cols.date_field_right;
			} else {
				snprintf(buffer, 32, " %s", datebuff);
				date_right = textx + imagetextwidth(ic, FONT_ROLE_BODY, buffer);
			}
			imagestring(ic, FONT_ROLE_BODY, date_right - imagetextwidth(ic, FONT_ROLE_BODY, cfg.estimatetext), texty, cfg.estimatetext, ic->ctext);
			imagestring(ic, FONT_ROLE_BODY, cols.rx_edge - imagetextwidth(ic, FONT_ROLE_BODY, rxbuf), texty, rxbuf, ic->ctext);
			imagestring(ic, FONT_ROLE_BODY, cols.tx_edge - imagetextwidth(ic, FONT_ROLE_BODY, txbuf), texty, txbuf, ic->ctext);
			imagestring(ic, FONT_ROLE_BODY, cols.total_edge - imagetextwidth(ic, FONT_ROLE_BODY, totalbuf), texty, totalbuf, ic->ctext);
		} else {
			if (strlen(datebuff) <= 9) {
				snprintf(buffer, 32, " %9s   ", cfg.estimatetext);
			} else {
				snprintf(buffer, 32, "  %9s  ", cfg.estimatetext);
			}
			strncat(buffer, getvalue(e_rx, 10, RT_Estimate), 32);
			strcat(buffer, "   ");
			strncat(buffer, getvalue(e_tx, 10, RT_Estimate), 32);
			strcat(buffer, "   ");
			strncat(buffer, getvalue(e_rx + e_tx, 10, RT_Estimate), 32);
			imagestring(ic, FONT_ROLE_BODY, textx, texty, buffer, ic->ctext);
		}

		if (ic->fontctx.mode == FONT_TTF) {
			footer_end = texty + ic->fontctx.ch;
		} else {
			footer_end = texty + ic->lineheight - imageextrapx(ic, 2);
		}
		list_draw_vdividers(ic, &cols, mid_y, footer_end, cfg.ostyle > 2);
	} else if (strlen(ic->dataend) > 0 && datainfo.count > 1 && listtype != LT_Top) {
		int footer_end;

		texty += imageuipx(ic, 8);
		if (ic->fontctx.mode == FONT_TTF) {
			char sumlabel[16];
			int date_right;

			if (strlen(datebuff) <= 8) {
				date_right = cols.date_field_right;
			} else {
				snprintf(buffer, 32, " %s", datebuff);
				date_right = textx + imagetextwidth(ic, FONT_ROLE_BODY, buffer);
			}

			if (datainfo.count < 100) {
				snprintf(sumlabel, 16, "sum of %" PRIu32 "", datainfo.count);
			} else {
				snprintf(sumlabel, 16, "sum");
			}
			strncpy_nt(rxbuf, getvalue(datainfo.sumrx, 10, RT_Normal), 64);
			strncpy_nt(txbuf, getvalue(datainfo.sumtx, 10, RT_Normal), 64);
			strncpy_nt(totalbuf, getvalue(datainfo.sumrx + datainfo.sumtx, 10, RT_Normal), 64);

			imagestring(ic, FONT_ROLE_BODY, date_right - imagetextwidth(ic, FONT_ROLE_BODY, sumlabel), texty, sumlabel, ic->ctext);
			imagestring(ic, FONT_ROLE_BODY, cols.rx_edge - imagetextwidth(ic, FONT_ROLE_BODY, rxbuf), texty, rxbuf, ic->ctext);
			imagestring(ic, FONT_ROLE_BODY, cols.tx_edge - imagetextwidth(ic, FONT_ROLE_BODY, txbuf), texty, txbuf, ic->ctext);
			imagestring(ic, FONT_ROLE_BODY, cols.total_edge - imagetextwidth(ic, FONT_ROLE_BODY, totalbuf), texty, totalbuf, ic->ctext);
		} else {
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
			imagestring(ic, FONT_ROLE_BODY, textx, texty, buffer, ic->ctext);
		}

		if (ic->fontctx.mode == FONT_TTF) {
			footer_end = texty + ic->fontctx.ch;
		} else {
			footer_end = texty + ic->lineheight - imageextrapx(ic, 2);
		}
		list_draw_vdividers(ic, &cols, mid_y, footer_end, cfg.ostyle > 2);
	}

	if (monthrotatenotevisible) {
		texty += ic->lineheight * 2;
		imagestring(ic, FONT_ROLE_BODY, textx + ic->fontctx.cw, texty, monthrotatenote, ic->ctext);
	}

	dbdatalistfree(&datalist);
}

/* Pixels per 5-minute sample. Builtin -L doubles; TTF widens when 2-hour labels need a gap. */
static int fiveg_barwidth(IMAGECONTENT *ic)
{
	int label_w, gap, need, slot = 24; /* hour labels every 2 hours = 24 samples */

	if (ic->fontctx.mode == FONT_BUILTIN) {
		return 1 + imageextrapx(ic, 1);
	}

	/* Gap grows with label size (min 4px) so large fonts stay readable between hours */
	label_w = imagetextwidth(ic, FONT_ROLE_AXIS, "00");
	gap = label_w / 4;
	if (gap < 4) {
		gap = 4;
	}
	need = label_w + gap;
	return (need + slot - 1) / slot;
}

/* Matches old left-aligned "rx " + getvalue(..., 12) width ("  999.99 YiB" pad). */
static const char summary_stack_sample[] = "rx   999.99 YiB";

/* Left inset of digest/all-time stacks (digest_x - bodyoff). */
static int summary_ttf_content_left(const IMAGECONTENT *ic)
{
	const int digest_x = (14 * ic->fontctx.cw + 2) + 26;
	const int bodyoff = 12 * ic->fontctx.cw + 2;

	return digest_x - bodyoff;
}

/* Right edge of a value stack including the +2*cw pad used for values/since/rate. */
static int summary_ttf_stack_right(IMAGECONTENT *ic, const int body_left)
{
	return body_left + imagetextwidth(ic, FONT_ROLE_BODY, summary_stack_sample) + 2 * ic->fontctx.cw;
}

/* Right edge of TTF drawlegend at legend_x (non-rate). */
static int summary_ttf_legend_right(IMAGECONTENT *ic, const int legend_x)
{
	const int sq = ic->fontctx.cw;
	const int gap = imageuipx(ic, 4);
	const int sep = sq + 2 * gap;
	int x_cur = legend_x;

	x_cur += sq + gap;
	x_cur += imagetextwidth(ic, FONT_ROLE_BODY, "rx") + sep;
	x_cur += sq + gap;
	x_cur += imagetextwidth(ic, FONT_ROLE_BODY, "tx");
	return x_cur;
}

/* Right ink of the second digest column donut (normal two-entry case). */
static int summary_ttf_digest_right(IMAGECONTENT *ic, const int digest_x)
{
	const int bodyoff = 12 * ic->fontctx.cw + 2;
	const int donut_size = 49 + imageextrapx(ic, 10);
	const int textx = digest_x + 30 * ic->fontctx.cw;
	const int body_left = textx - bodyoff;
	const int col_right = body_left + imagetextwidth(ic, FONT_ROLE_BODY, summary_stack_sample);

	/* Center at col_right + donut_size; radius donut_size/2. */
	return col_right + donut_size + donut_size / 2;
}

static void summary_ttf_set_positions(IMAGECONTENT *ic, const int headermod,
	int *digest_x, int *alltime_x, int *legend_x, int *graph_x, int *fivegraph_x,
	int *digest_day_y, int *digest_month_y, int *alltime_y, int *legend_y)
{
	*alltime_x = 66 * ic->fontctx.cw;
	*legend_x = 69 * ic->fontctx.cw;
	*graph_x = 84 * ic->fontctx.cw;
	*fivegraph_x = *graph_x;
	/* body at textx - (12*cw+2) stays near the builtin left margin (~26). */
	*digest_x = (14 * ic->fontctx.cw + 2) + 26;
	/* Clear tall header title; builtin keeps y=30. */
	*digest_day_y = ic->fontctx.header_h + 15 - headermod;
	*digest_month_y = *digest_day_y - 1 + 8 * ic->lineheight;
	*alltime_y = *digest_day_y + 27 + imageextrapx(ic, 10);
	/* Under all-time "since" line, matching builtin legend vs since gap */
	*legend_y = *alltime_y + 9 * ic->lineheight;
}

static int summary_ttf_compute_width(IMAGECONTENT *ic, const int layout,
	const int digest_x, const int alltime_x, const int legend_x, const int graph_x, const int fivegraph_x,
	const int fiveg_barwidth_val)
{
	int content_left, content_right, r, width;

	content_left = summary_ttf_content_left(ic);
	content_right = summary_ttf_stack_right(ic, alltime_x);
	r = summary_ttf_legend_right(ic, legend_x);
	if (r > content_right) {
		content_right = r;
	}
	r = summary_ttf_digest_right(ic, digest_x);
	if (r > content_right) {
		content_right = r;
	}

	if (layout == 1) {
		if (cfg.summarygraph == 1) {
			int fiveg_samples = 422 + imageextrapx(ic, 154);

			r = fivegraph_x + fiveg_samples * fiveg_barwidth_val + graph_extra_space(ic) - graph_xpos_margin(ic);
		} else {
			r = graph_x + hourly_graph_width(ic) - hourly_graph_left(ic);
		}
		if (r > content_right) {
			content_right = r;
		}
	}

	width = content_right + content_left;
	if (width < 1) {
		width = 1;
	}

	/* Vertical graph may be wider than the text block (esp. multi-pixel 5-min bars). */
	if (layout == 2) {
		int graph_w;

		if (cfg.summarygraph == 1) {
			graph_w = (422 + imageextrapx(ic, 154)) * fiveg_barwidth_val + graph_extra_space(ic);
		} else {
			graph_w = hourly_graph_width(ic);
		}
		if (width < graph_w) {
			width = graph_w;
		}
	}

	return width;
}

static void summary_ttf_adjust_height(IMAGECONTENT *ic, const int layout,
	const int header_extra, const int headermod, const int monthrotatenotevisible,
	int *height, int *vs_fiveg_bottom)
{
	if (layout == 2) {
		if (cfg.summarygraph == 1) {
			int bottom_margin;

			/* Axis labels sit below the 5-min plot; grow margin + canvas together */
			bottom_margin = ic->fontctx.axis_ch + imageuipx(ic, 4) + imageuipx(ic, 12) + ic->showedge;
			if (bottom_margin > *vs_fiveg_bottom) {
				*height += bottom_margin - *vs_fiveg_bottom;
				*vs_fiveg_bottom = bottom_margin;
			}
		} else {
			int graph_y, needed;

			graph_y = 215 + header_extra + imageextrapx(ic, 84) - headermod
				+ (monthrotatenotevisible * (ic->lineheight * 2));
			/* labels at graph_y+124+imageuipx(8); Tiny footer at height-12-showedge */
			needed = graph_y + 124 + imageuipx(ic, 8) + ic->fontctx.axis_ch + imageuipx(ic, 4) + imageuipx(ic, 12) + ic->showedge;
			if (*height < needed) {
				*height = needed;
			}
		}
	} else {
		/* Extra bottom pad so rate/legend clear the footer */
		*height += 2 * ic->lineheight;
	}
}

void drawsummary(IMAGECONTENT *ic, const int layout, const int israte)
{
	int width, height, headermod, header_extra, digest_x, alltime_x, legend_x, legend_y, graph_x, fivegraph_x;
	int digest_day_y, digest_month_y, alltime_y;
	int monthrotatenotevisible = 0;
	int vs_fiveg_bottom, fiveg_barwidth_val = 1;
	char monthrotatenote[96];

	monthrotatenotevisible = ismonthrotatenoteneeded();
	if (monthrotatenotevisible) {
		getmonthrotatenote(monthrotatenote, sizeof(monthrotatenote));
	}

	vs_fiveg_bottom = 31 + imageextrapx(ic, 6);

	if (cfg.summarygraph == 1 && (layout == 1 || layout == 2)) {
		fiveg_barwidth_val = fiveg_barwidth(ic);
	}

	switch (layout) {
		// horizontal
		case 1:
			height = 56 + 12 * ic->lineheight;
			if (ic->fontctx.mode == FONT_TTF) {
				height += ic->lineheight;
			}
			break;
		// vertical
		case 2:
			height = 370 + imageextrapx(ic, 90);
			break;
		// no hours
		default:
			height = 56 + 12 * ic->lineheight;
			if (ic->fontctx.mode == FONT_TTF) {
				height += ic->lineheight;
			}
			break;
	}

	if (monthrotatenotevisible) {
		height += ic->lineheight * 2;
	}

	if (!ic->showheader) {
		headermod = ic->fontctx.header_h + imageuipx(ic, 2);
		header_extra = 0;
		height -= ic->fontctx.header_h - 2;
	} else {
		headermod = 0;
		header_extra = ic->fontctx.header_h - 24;
		height += header_extra;
	}

	if (ic->fontctx.mode == FONT_TTF) {
		summary_ttf_adjust_height(ic, layout, header_extra, headermod, monthrotatenotevisible, &height, &vs_fiveg_bottom);
	}

	/* Scale fiveg plot height with barwidth to keep aspect ratio (stable base, not chrome) */
	if (cfg.summarygraph == 1 && (layout == 1 || layout == 2) && fiveg_barwidth_val > 1) {
		int fiveg_h;

		if (layout == 2) {
			fiveg_h = 132;
		} else {
			fiveg_h = height - 68 + headermod - imageextrapx(ic, 8) - (monthrotatenotevisible * (ic->lineheight + 2));
		}
		if (fiveg_h > 0) {
			height += (fiveg_barwidth_val - 1) * fiveg_h;
		}
	}

	if (ic->fontctx.mode == FONT_TTF) {
		summary_ttf_set_positions(ic, headermod, &digest_x, &alltime_x, &legend_x, &graph_x, &fivegraph_x,
			&digest_day_y, &digest_month_y, &alltime_y, &legend_y);
		width = summary_ttf_compute_width(ic, layout, digest_x, alltime_x, legend_x, graph_x, fivegraph_x, fiveg_barwidth_val);
	} else {
		switch (layout) {
			case 1:
				width = 163 * ic->fontctx.cw + imageuipx(ic, 2) + imageextrapx(ic, 2);
				break;
			case 2:
				width = 83 * ic->fontctx.cw + imageuipx(ic, 2) + imageextrapx(ic, 2);
				break;
			default:
				width = 83 * ic->fontctx.cw + imageuipx(ic, 2) + imageextrapx(ic, 2);
				break;
		}

		/* Multi-pixel bars widen the 5-min plot; grow canvas so it is not clipped */
		if (cfg.summarygraph == 1 && (layout == 1 || layout == 2)) {
			int fiveg_samples = 422 + imageextrapx(ic, 154);

			width += (fiveg_barwidth_val - 1) * fiveg_samples;
		}

		alltime_x = 385 + imageextrapx(ic, 125);
		legend_x = 410 + imageextrapx(ic, 132);
		graph_x = 500 + imageextrapx(ic, 160);
		fivegraph_x = 496 + imageextrapx(ic, 174);
		digest_x = 100;
		digest_day_y = 30 + header_extra - headermod;
		digest_month_y = 29 + 7 * ic->lineheight + header_extra - headermod;
		alltime_y = 57 + header_extra - headermod + imageextrapx(ic, 10);
		legend_y = 155 - headermod + imageextrapx(ic, 40);
	}

	imageinit(ic, width, height);
	layoutinit(ic, "", width, height);

	drawsummary_alltime(ic, alltime_x, alltime_y);
	drawlegend(ic, legend_x, legend_y, 0);

	drawsummary_digest(ic, digest_x, digest_day_y, "day");
	drawsummary_digest(ic, digest_x, digest_month_y, "month");

	switch (layout) {
		// horizontal
		case 1:
			if (cfg.summarygraph == 1) {
				drawfiveminutes(ic, fivegraph_x, height - 30 - imageextrapx(ic, 8) - (monthrotatenotevisible * ic->lineheight), israte, 422 + imageextrapx(ic, 154), height - 68 + headermod - imageextrapx(ic, 8) - (monthrotatenotevisible * (ic->lineheight + 2)));
			} else {
				drawhours(ic, graph_x, 46 + header_extra + imageextrapx(ic, 40) - headermod, israte);
			}
			if (monthrotatenotevisible) {
				imagestring(ic, FONT_ROLE_BODY, 13 - imageextrapx(ic, 4) + (ic->fontctx.cw * 2) + ic->showedge, height - imageuipx(ic, 12) - ic->showedge - ic->lineheight, monthrotatenote, ic->ctext);
			}
			break;
		// vertical
		case 2:
			if (cfg.summarygraph == 1) {
				int fiveg_samples = 422 + imageextrapx(ic, 154);
				int fiveg_w = fiveg_samples * fiveg_barwidth_val + graph_extra_space(ic);
				int fiveg_x = graph_xpos_margin(ic);

				if (ic->fontctx.mode == FONT_TTF) {
					/* Same block centering as standalone: xpos = block_start + graph_xpos_margin. */
					fiveg_x = (width - fiveg_w) / 2 + graph_xpos_margin(ic);
					if (fiveg_x < 0) {
						fiveg_x = 0;
					}
				}
				drawfiveminutes(ic, fiveg_x, height - vs_fiveg_bottom, israte, fiveg_samples, 132 * fiveg_barwidth_val + imageextrapx(ic, 35));
			} else {
				int hours_x = hourly_graph_left(ic);

				if (ic->fontctx.mode == FONT_TTF) {
					int hours_w = hourly_graph_width(ic);

					/* Same block centering as standalone: xpos = block_start + hourly_graph_left. */
					hours_x = (width - hours_w) / 2 + hourly_graph_left(ic);
					if (hours_x < 0) {
						hours_x = 0;
					}
				}
				drawhours(ic, hours_x, 215 + header_extra + imageextrapx(ic, 84) - headermod + (monthrotatenotevisible * (ic->lineheight * 2)), israte);
			}
			if (monthrotatenotevisible) {
				imagestring(ic, FONT_ROLE_BODY, 13 - imageextrapx(ic, 4) + (ic->fontctx.cw * 2) + ic->showedge, 215 + header_extra + imageextrapx(ic, 84) - headermod - (ic->lineheight * (1 + imageextrapx(ic, 2))), monthrotatenote, ic->ctext);
			}
			break;
		default:
			if (monthrotatenotevisible) {
				imagestring(ic, FONT_ROLE_BODY, 13 - imageextrapx(ic, 4) + (ic->fontctx.cw * 2) + ic->showedge, height - imageuipx(ic, 12) - ic->showedge - ic->lineheight, monthrotatenote, ic->ctext);
			}
			break;
	}
}

static void drawsummary_stack_ttf(IMAGECONTENT *ic, const int body_left, const int value_edge,
	const int y_rx, const int y_tx, const int y_eq, const uint64_t rx, const uint64_t tx)
{
	const int label_edge = body_left + imagetextwidth(ic, FONT_ROLE_BODY, "rx");
	/* Shift values right by 2*cw; callers keep donut/title anchors on value_edge. */
	const int padded_edge = value_edge + 2 * ic->fontctx.cw;
	char valbuf[64];

	imagestring(ic, FONT_ROLE_BODY, label_edge - imagetextwidth(ic, FONT_ROLE_BODY, "rx"), y_rx, "rx", ic->ctext);
	strncpy_nt(valbuf, getvalue(rx, 12, RT_Normal), 64);
	imagestring(ic, FONT_ROLE_BODY, padded_edge - imagetextwidth(ic, FONT_ROLE_BODY, valbuf), y_rx, valbuf, ic->ctext);

	imagestring(ic, FONT_ROLE_BODY, label_edge - imagetextwidth(ic, FONT_ROLE_BODY, "tx"), y_tx, "tx", ic->ctext);
	strncpy_nt(valbuf, getvalue(tx, 12, RT_Normal), 64);
	imagestring(ic, FONT_ROLE_BODY, padded_edge - imagetextwidth(ic, FONT_ROLE_BODY, valbuf), y_tx, valbuf, ic->ctext);

	imagestring(ic, FONT_ROLE_BODY, label_edge - imagetextwidth(ic, FONT_ROLE_BODY, "="), y_eq, "=", ic->ctext);
	strncpy_nt(valbuf, getvalue(rx + tx, 12, RT_Normal), 64);
	imagestring(ic, FONT_ROLE_BODY, padded_edge - imagetextwidth(ic, FONT_ROLE_BODY, valbuf), y_eq, valbuf, ic->ctext);
}

void drawsummary_alltime(IMAGECONTENT *ic, const int x, const int y)
{
	int title_x, col_right, since_x;
	const struct tm *d;
	char buffer[512], datebuff[16], daytemp[32];

	if (ic->fontctx.mode == FONT_TTF) {
		col_right = x + imagetextwidth(ic, FONT_ROLE_BODY, summary_stack_sample);
		title_x = col_right - imagetextwidth(ic, FONT_ROLE_TITLE, "all time");
		imagestring(ic, FONT_ROLE_TITLE, title_x, y, "all time", ic->ctext);

		drawsummary_stack_ttf(ic, x, col_right,
			y + (2 * ic->lineheight),
			y + (3 * ic->lineheight),
			y + (ic->lineheight * 9 / 2),
			ic->interface.rxtotal, ic->interface.txtotal);

		d = localtime(&ic->interface.created);
		strftime(datebuff, 16, cfg.tformat, d);
		snprintf(daytemp, 24, "since %s", datebuff);
		since_x = col_right + 2 * ic->fontctx.cw - imagetextwidth(ic, FONT_ROLE_BODY, daytemp);
		imagestring(ic, FONT_ROLE_BODY, since_x, y + (6 * ic->lineheight), daytemp, ic->ctext);
		return;
	}

	title_x = x + 12 + imageextrapx(ic, 10);
	imagestring(ic, FONT_ROLE_TITLE, title_x, y, "all time", ic->ctext);

	snprintf(buffer, 4, "rx ");
	strncat(buffer, getvalue(ic->interface.rxtotal, 12, RT_Normal), 32);
	imagestring(ic, FONT_ROLE_BODY, x, y + (2 * ic->lineheight), buffer, ic->ctext);
	snprintf(buffer, 4, "tx ");
	strncat(buffer, getvalue(ic->interface.txtotal, 12, RT_Normal), 32);
	imagestring(ic, FONT_ROLE_BODY, x, y + (3 * ic->lineheight), buffer, ic->ctext);
	snprintf(buffer, 4, " = ");
	strncat(buffer, getvalue(ic->interface.rxtotal + ic->interface.txtotal, 12, RT_Normal), 32);
	imagestring(ic, FONT_ROLE_BODY, x, y + (ic->lineheight * 9 / 2), buffer, ic->ctext);
	d = localtime(&ic->interface.created);
	strftime(datebuff, 16, cfg.tformat, d);
	snprintf(daytemp, 24, "since %s", datebuff);
	snprintf(buffer, 32, "%23s", daytemp);
	imagestring(ic, FONT_ROLE_BODY, x - 8 * ic->fontctx.cw, y + (5 * ic->lineheight) + 10 + imageextrapx(ic, 4), buffer, ic->ctext);
}

void drawsummary_digest(IMAGECONTENT *ic, const int x, const int y, const char *mode)
{
	int textx, texty, offset = 0, bodyoff, body_left, col_right, title_x, title_y;
	int donut_x, donut_y, donut_size, donut_hole, y_tx, y_eq, rate_x;
	double rxp, txp, mod;
	char buffer[512], datebuff[16], daytemp[32], ratebuf[64];
	const char *rateptr;
	time_t yesterday;
	const struct tm *d = NULL;
	dbdatalist *datalist = NULL;
	const dbdatalist *data_current = NULL, *data_previous = NULL;
	dbdatalistinfo datainfo;

	yesterday = ic->current - 86400;
	/* Builtin keeps historical 74px; TTF scales with cell width (12*6+2 == 74). */
	bodyoff = (ic->fontctx.mode == FONT_TTF) ? (12 * ic->fontctx.cw + 2) : 74;

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
		snprintf(buffer, 512, "no %s data available", mode);
		imagestring(ic, FONT_ROLE_BODY, 20 * ic->fontctx.cw, y + 30, buffer, ic->ctext);
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
		offset = 85 + imageextrapx(ic, 25);
	}

	textx = x + offset;
	texty = y;
	donut_size = 49 + imageextrapx(ic, 10);
	donut_hole = 15 + imageextrapx(ic, 3);

	if (mode[0] == 'd') {
		d = localtime(&ic->current);
		strftime(datebuff, 16, cfg.dformat, d);
		d = localtime(&data_current->timestamp);
		strftime(daytemp, 16, cfg.dformat, d);
		if (strcmp(datebuff, daytemp) == 0) {
			strncpy_nt(daytemp, "today", 32);
		}
	} else if (mode[0] == 'm') {
		d = localtime(&data_current->timestamp);
		strftime(daytemp, 16, cfg.mformat, d);
	}

	snprintf(buffer, 4, "rx ");
	strncat(buffer, getvalue(data_current->rx, 12, RT_Normal), 32);

	if (ic->fontctx.mode == FONT_TTF) {
		body_left = textx - bodyoff;
		col_right = body_left + imagetextwidth(ic, FONT_ROLE_BODY, summary_stack_sample);
		title_x = col_right - imagetextwidth(ic, FONT_ROLE_TITLE, daytemp) / 2;
		title_y = texty;
		y_tx = texty + 3 * ic->lineheight;
		y_eq = texty + 4 * ic->lineheight + 2;
		donut_y = (y_tx + y_eq) / 2 + imagefontheight(ic, FONT_ROLE_BODY) / 2;
		donut_x = col_right + donut_size;

		drawdonut(ic, donut_x, donut_y, (float)rxp, (float)txp, donut_size, donut_hole);
		imagestring(ic, FONT_ROLE_TITLE, title_x, title_y, daytemp, ic->ctext);

		drawsummary_stack_ttf(ic, body_left, col_right,
			texty + 2 * ic->lineheight,
			texty + 3 * ic->lineheight,
			texty + 4 * ic->lineheight + 2,
			data_current->rx, data_current->tx);

		if (cfg.summaryrate) {
			d = localtime(&ic->interface.updated);
			if (mode[0] == 'd') {
				rateptr = gettrafficrate(data_current->rx + data_current->tx, (time_t)getperiodseconds(LT_Day, data_current->timestamp, ic->interface.updated, ic->interface.created, 1), 1);
			} else {
				rateptr = gettrafficrate(data_current->rx + data_current->tx, (time_t)getperiodseconds(LT_Month, data_current->timestamp, ic->interface.updated, ic->interface.created, 1), 1);
			}
			while (*rateptr == ' ') {
				rateptr++;
			}
			strncpy_nt(ratebuf, rateptr, 64);
			rate_x = col_right + 2 * ic->fontctx.cw - imagetextwidth(ic, FONT_ROLE_BODY, ratebuf);
			imagestring(ic, FONT_ROLE_BODY, rate_x, texty + 5 * ic->lineheight + 10, ratebuf, ic->ctext);
		}
	} else {
		char titlebuf[32];

		donut_x = textx + 50 + imageextrapx(ic, 40);
		donut_y = texty + 45 + imageextrapx(ic, 10);
		title_x = textx - 54 + imageextrapx(ic, ic->fontctx.cw * 3 - 4);
		title_y = texty - 1;
		drawdonut(ic, donut_x, donut_y, (float)rxp, (float)txp, donut_size, donut_hole);
		snprintf(titlebuf, 32, "%*s", getpadding(12, daytemp), daytemp);
		imagestring(ic, FONT_ROLE_TITLE, title_x, title_y, titlebuf, ic->ctext);

		if (cfg.summaryrate) {
			d = localtime(&ic->interface.updated);
			if (mode[0] == 'd') {
				snprintf(datebuff, 16, "%15s", gettrafficrate(data_current->rx + data_current->tx, (time_t)getperiodseconds(LT_Day, data_current->timestamp, ic->interface.updated, ic->interface.created, 1), 15));
			} else if (mode[0] == 'm') {
				snprintf(datebuff, 16, "%15s", gettrafficrate(data_current->rx + data_current->tx, (time_t)getperiodseconds(LT_Month, data_current->timestamp, ic->interface.updated, ic->interface.created, 1), 15));
			}
			imagestring(ic, FONT_ROLE_BODY, textx - bodyoff, texty + 4 * ic->lineheight + 10, datebuff, ic->ctext);
		} else {
			texty += 7;
		}

		imagestring(ic, FONT_ROLE_BODY, textx - bodyoff, texty + ic->lineheight + 6, buffer, ic->ctext);
		snprintf(buffer, 4, "tx ");
		strncat(buffer, getvalue(data_current->tx, 12, RT_Normal), 32);
		imagestring(ic, FONT_ROLE_BODY, textx - bodyoff, texty + 2 * ic->lineheight + 6, buffer, ic->ctext);
		snprintf(buffer, 4, " = ");
		strncat(buffer, getvalue(data_current->rx + data_current->tx, 12, RT_Normal), 32);
		imagestring(ic, FONT_ROLE_BODY, textx - bodyoff, texty + 3 * ic->lineheight + 8, buffer, ic->ctext);
	}

	/* previous entry */
	if (data_previous != NULL) {
		if (data_previous->rx + data_previous->tx == 0) {
			rxp = txp = 0;
		} else {
			rxp = (double)data_previous->rx / (double)(data_previous->rx + data_previous->tx) * 100;
			txp = (double)100 - rxp;
		}

		if ((data_previous->rx + data_previous->tx) < (data_current->rx + data_current->tx)) {
			mod = (double)(data_previous->rx + data_previous->tx) / (double)(data_current->rx + data_current->tx);
			rxp = rxp * mod;
			txp = txp * mod;
		}

		if (ic->fontctx.mode == FONT_TTF) {
			textx += 30 * ic->fontctx.cw;
		} else {
			textx += 180 + imageextrapx(ic, 60);
		}

		if (mode[0] == 'd') {
			d = localtime(&yesterday);
			strftime(datebuff, 16, cfg.dformat, d);
			d = localtime(&data_previous->timestamp);
			strftime(daytemp, 16, cfg.dformat, d);
			if (strcmp(datebuff, daytemp) == 0) {
				strncpy_nt(daytemp, "yesterday", 32);
			}
		} else if (mode[0] == 'm') {
			d = localtime(&data_previous->timestamp);
			strftime(daytemp, 16, cfg.mformat, d);
		}

		snprintf(buffer, 4, "rx ");
		strncat(buffer, getvalue(data_previous->rx, 12, RT_Normal), 32);

		if (ic->fontctx.mode == FONT_TTF) {
			body_left = textx - bodyoff;
			col_right = body_left + imagetextwidth(ic, FONT_ROLE_BODY, summary_stack_sample);
			title_x = col_right - imagetextwidth(ic, FONT_ROLE_TITLE, daytemp) / 2;
			title_y = y;
			y_tx = y + 3 * ic->lineheight;
			y_eq = y + 4 * ic->lineheight + 2;
			donut_y = (y_tx + y_eq) / 2 + imagefontheight(ic, FONT_ROLE_BODY) / 2;
			donut_x = col_right + donut_size;

			drawdonut(ic, donut_x, donut_y, (float)rxp, (float)txp, donut_size, donut_hole);
			imagestring(ic, FONT_ROLE_TITLE, title_x, title_y, daytemp, ic->ctext);

			drawsummary_stack_ttf(ic, body_left, col_right,
				y + 2 * ic->lineheight,
				y + 3 * ic->lineheight,
				y + 4 * ic->lineheight + 2,
				data_previous->rx, data_previous->tx);

			if (cfg.summaryrate) {
				if (mode[0] == 'd') {
					rateptr = gettrafficrate(data_previous->rx + data_previous->tx, 86400, 1);
				} else {
					rateptr = gettrafficrate(data_previous->rx + data_previous->tx, dmonth(d->tm_mon) * 86400, 1);
				}
				while (*rateptr == ' ') {
					rateptr++;
				}
				strncpy_nt(ratebuf, rateptr, 64);
				rate_x = col_right + 2 * ic->fontctx.cw - imagetextwidth(ic, FONT_ROLE_BODY, ratebuf);
				imagestring(ic, FONT_ROLE_BODY, rate_x, y + 5 * ic->lineheight + 10, ratebuf, ic->ctext);
			}
		} else {
			char titlebuf[32];

			donut_x = textx + 50 + imageextrapx(ic, 40);
			donut_y = texty + 45 + imageextrapx(ic, 10);
			title_x = textx - 54 + imageextrapx(ic, ic->fontctx.cw * 3 - 4);
			title_y = texty - 1;
			drawdonut(ic, donut_x, donut_y, (float)rxp, (float)txp, donut_size, donut_hole);
			snprintf(titlebuf, 32, "%*s", getpadding(12, daytemp), daytemp);
			imagestring(ic, FONT_ROLE_TITLE, title_x, title_y, titlebuf, ic->ctext);

			if (cfg.summaryrate) {
				if (mode[0] == 'd') {
					snprintf(datebuff, 16, "%15s", gettrafficrate(data_previous->rx + data_previous->tx, 86400, 15));
				} else if (mode[0] == 'm') {
					snprintf(datebuff, 16, "%15s", gettrafficrate(data_previous->rx + data_previous->tx, dmonth(d->tm_mon) * 86400, 15));
				}
				imagestring(ic, FONT_ROLE_BODY, textx - bodyoff, texty + 4 * ic->lineheight + 10, datebuff, ic->ctext);
			} else {
				texty += 7;
			}

			imagestring(ic, FONT_ROLE_BODY, textx - bodyoff, texty + ic->lineheight + 6, buffer, ic->ctext);
			snprintf(buffer, 4, "tx ");
			strncat(buffer, getvalue(data_previous->tx, 12, RT_Normal), 32);
			imagestring(ic, FONT_ROLE_BODY, textx - bodyoff, texty + 2 * ic->lineheight + 6, buffer, ic->ctext);
			snprintf(buffer, 4, " = ");
			strncat(buffer, getvalue(data_previous->rx + data_previous->tx, 12, RT_Normal), 32);
			imagestring(ic, FONT_ROLE_BODY, textx - bodyoff, texty + 3 * ic->lineheight + 8, buffer, ic->ctext);
		}
	}

	data_current = NULL;
	data_previous = NULL;
	dbdatalistfree(&datalist);
}

void drawfivegraph(IMAGECONTENT *ic, const int israte, const int resultcount, const int height)
{
	int imagewidth, imageheight, headermod = 0, header_extra = 0;
	int bottom, legend_y, graph_height, barwidth, base_graph, top;

	barwidth = fiveg_barwidth(ic);
	imagewidth = resultcount * barwidth + graph_extra_space(ic);

	/* Plot height from configured size (default top 38 + bottom 30), then * barwidth.
	 * Do not derive from imageheight - chrome, or large fonts shrink the plot first. */
	base_graph = height - 68;
	if (base_graph < 1) {
		base_graph = 1;
	}
	graph_height = base_graph * barwidth;

	if (!ic->showheader) {
		headermod = ic->fontctx.header_h - 2;
		top = 38 - headermod;
		if (top < 2) {
			top = 2;
		}
		if (ic->fontctx.mode == FONT_TTF) {
			int min_top = imageuipx(ic, 8) + ic->fontctx.axis_ch / 2;
			if (top < min_top) {
				top = min_top;
			}
		}
	} else {
		header_extra = ic->fontctx.header_h - 24;
		if (header_extra < 0) {
			header_extra = 0;
		}
		top = 38 + header_extra;
		if (ic->fontctx.mode == FONT_TTF) {
			/* Clear header for up-arrow and topmost scale label ascent. */
			int min_top = ic->fontctx.header_h + imageuipx(ic, 8) + ic->fontctx.axis_ch / 2;
			if (top < min_top) {
				top = min_top;
			}
		}
	}

	bottom = 30 + imageextrapx(ic, 8);

	if (ic->fontctx.mode == FONT_TTF) {
		int needed_bottom;

		/* Labels at ypos+imageuipx(8); leave room for legend + footer */
		needed_bottom = imageuipx(ic, 8) + ic->fontctx.axis_ch + imageuipx(ic, 4) + ic->fontctx.ch + imageuipx(ic, 4) + imageuipx(ic, 12) + ic->showedge;
		if (needed_bottom > bottom) {
			bottom = needed_bottom;
		}
	}

	imageheight = top + graph_height + bottom;

	if (ic->fontctx.mode == FONT_TTF) {
		legend_y = imageheight - ic->showedge - (int)(ic->fontctx.ch * 1.5);
	} else {
		legend_y = imageheight - 17 - imageextrapx(ic, 2);
	}

	imageinit(ic, imagewidth, imageheight);
	layoutinit(ic, " / 5 minute", imagewidth, imageheight);

	if (drawfiveminutes(ic, graph_xpos_margin(ic), imageheight - bottom, israte, resultcount, graph_height)) {
		drawlegend(ic, imagewidth / 2 - imageextrapx(ic, 10), legend_y, 0);
	}
}

int drawfiveminutes(IMAGECONTENT *ic, const int xpos, const int ypos, const int israte, const int resultcount, const int height)
{
	int x = xpos, y = ypos, i = 0, t = 0, rxh = 0, txh = 0, step = 0, s = 0, prev = 0, cross;
	int barwidth, plot_w, b, px, axis_left, unit_x, pad_full, pad_inner;
	int axis_base_x, axis_stem_x, plot_x0, hline_x0, center_y, stroke_half;
	uint64_t scaleunit, max;
	time_t timestamp;
	double ratediv, e;
	char buffer[32];
	const struct tm *d;
	dbdatalist *datalist = NULL, *datalist_i = NULL;
	dbdatalistinfo datainfo;

	barwidth = fiveg_barwidth(ic);
	plot_w = resultcount * barwidth;
	axis_left = graph_axis_left(ic);
	unit_x = xpos;
	pad_full = imageuipx(ic, FIVEMINWIDTHFULLPADDING);
	pad_inner = imageuipx(ic, FIVEMINWIDTHPADDING);
	stroke_half = graph_stroke_half(ic);

	if (!db_getdata(&datalist, &datainfo, ic->interface.name, "fiveminute", (uint32_t)resultcount) || datainfo.count == 0) {
		x = (plot_w + graph_extra_space(ic)) / 2 - (13 * ic->fontctx.cw);
		imagestring(ic, FONT_ROLE_BODY, x, y - (height / 2) - ic->fontctx.ch, "no 5 minute data available", ic->ctext);
		return 0;
	}

	datalist_i = datalist;

	if (cfg.rateunit) {
		ratediv = 37.5; /* x * 8 / 300 */
	} else {
		ratediv = 300;
	}

	/* axis */
	cross = imageuipx(ic, GRAPH_AXIS_CROSS);
	x += axis_left;
	axis_base_x = x;
	axis_stem_x = x + cross;
	imagedrawhline(ic, x, x + (plot_w + pad_full), y, ic->ctext);
	imagedrawvline(ic, x + cross, y + cross, y - height, ic->ctext);

	/* arrows — tip at axis endpoint so the head is not inset past the stroke */
	drawarrowup(ic, x + cross, y - height);
	drawarrowright(ic, x + (plot_w + pad_full), y);

	max = datainfo.maxrx + datainfo.maxtx;

	if (datainfo.maxrx == datainfo.maxtx) {
		txh = (int)((height - FIVEMINHEIGHTOFFSET * 2) / 2);
		rxh = height - FIVEMINHEIGHTOFFSET * 2 - txh;
		max = (uint64_t)((double)datainfo.maxrx / ratediv);
		t = rxh;
	} else if (datainfo.maxrx > datainfo.maxtx) {
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

	/* center line; y-axis is at x-1 after this advance (drawn at previous x+cross) */
	x += cross + 1;
	plot_x0 = x;
	hline_x0 = plot_x0 + stroke_half;
	y -= txh + FIVEMINHEIGHTOFFSET;
	center_y = y;
	imagedrawhline(ic, hline_x0, plot_x0 + (plot_w + pad_inner), y, ic->ctext);
	graph_draw_axis_value(ic, plot_x0 - 1, y, "  0", plot_x0 - 21 - imageextrapx(ic, 3), y - 4 - imageextrapx(ic, 3));

	/* scale values */
	scaleunit = getscale(max, israte);

	s = (int)lrint(((double)scaleunit / (double)max) * t);
	if (s == 0) {
		s = 1; // force to show something when there's not much or any traffic, scale is likely to be wrong in this case
	}
	{
		int min_step_px = SCALEMINPIXELS;

		/* Tall TTF axis digits need more vertical space between scale labels */
		if (ic->fontctx.mode == FONT_TTF && ic->fontctx.axis_ch + 4 > min_step_px) {
			min_step_px = ic->fontctx.axis_ch + 4;
		}
		while (s * step < min_step_px) {
			step++;
		}
	}

	if (debug) {
		printf("maxrx: %" PRIu64 "\n", datainfo.maxrx);
		printf("maxtx: %" PRIu64 "\n", datainfo.maxtx);
		printf("rxh: %d     txh: %d\n", rxh, txh);
		printf("max divided: %" PRIu64 "\n", max);
		printf("scaleunit:   %" PRIu64 "\nstep: %d\n", scaleunit, step);
		printf("pixels per step: %d\n", s);
		printf("barwidth: %d\n", barwidth);
		printf("mintime: %" PRIu64 "\nmaxtime: %" PRIu64 "\n", (uint64_t)datainfo.mintime, (uint64_t)datainfo.maxtime);
		printf("count: %u\n", datainfo.count);
	}

	/* upper part scale values */
	y--; // adjust to start above center line
	for (i = step; i * s <= rxh; i = i + step) {
		const char *val;
		int line_y;

		line_y = y - (i * s);
		imagedrawdashedhline(ic, hline_x0, plot_x0 + (plot_w + pad_inner), line_y, ic->cline);
		imagedrawdashedhline(ic, hline_x0, plot_x0 + (plot_w + pad_inner), y - prev - (step * s) / 2, ic->clinel);
		val = getimagevalue(scaleunit * (unsigned int)i, 3, israte);
		graph_draw_axis_value(ic, plot_x0 - 1, line_y, val, plot_x0 - 21 - imageextrapx(ic, 3), line_y - 3 - imageextrapx(ic, 3));
		prev = i * s;
	}
	if ((prev + (step * s) / 2) <= rxh) {
		imagedrawdashedhline(ic, hline_x0, plot_x0 + (plot_w + pad_inner), y - prev - (step * s) / 2, ic->clinel);
	}

	y += 2; // adjust to start below center line
	prev = 0;

	/* lower part scale values */
	for (i = step; i * s <= txh; i = i + step) {
		const char *val;
		int line_y;

		line_y = y + (i * s);
		imagedrawdashedhline(ic, hline_x0, plot_x0 + (plot_w + pad_inner), line_y, ic->cline);
		imagedrawdashedhline(ic, hline_x0, plot_x0 + (plot_w + pad_inner), y + prev + (step * s) / 2, ic->clinel);
		val = getimagevalue(scaleunit * (unsigned int)i, 3, israte);
		graph_draw_axis_value(ic, plot_x0 - 1, line_y, val, plot_x0 - 21 - imageextrapx(ic, 3), line_y - 3 - imageextrapx(ic, 3));
		prev = i * s;
	}
	if ((prev + (step * s) / 2) <= txh) {
		imagedrawdashedhline(ic, hline_x0, plot_x0 + (plot_w + pad_inner), y + prev + (step * s) / 2, ic->clinel);
	}

	y--; // y is now back on center line

	/* scale text */
	graph_draw_axis_unit(ic, unit_x, plot_x0 - 39 - imageextrapx(ic, 14), ypos - height / 2 + (israte * 10), getimagescale(scaleunit * (unsigned int)step, israte));

	timestamp = datainfo.maxtime - (resultcount * 300);

	while (datalist_i != NULL && datalist_i->timestamp < timestamp + 300) {
		if (debug) {
			printf("Skip data, %" PRIu64 " < %" PRIu64 "\n", (uint64_t)datalist_i->timestamp, (uint64_t)timestamp + 300);
		}
		datalist_i = datalist_i->next;
	}

	for (i = 0; i < resultcount; i++) {

		if (datalist_i == NULL) {
			break;
		}

		timestamp += 300;
		d = localtime(&timestamp);
		px = plot_x0 + i * barwidth;

		if (d->tm_min == 0 && i > 2) {
			/* Split around thick zero-line and bottom x-axis so hour marks do not punch holes. */
			int line_t = imageuipx(ic, 1);
			int center_top = center_y - line_t / 2;
			int center_bot = center_top + line_t - 1;
			int y_bot = ypos - stroke_half - 1;
			int y_top = center_y - rxh - 1;
			int hour_color;

			if (d->tm_hour % 2 == 0) {
				hour_color = (d->tm_hour == 0) ? ic->cline : ic->cbgoffset;
				if (y_bot > center_bot + 1) {
					imagedrawvline(ic, px, y_bot, center_bot + 1, hour_color);
				}
				if (y_top < center_top - 1) {
					imagedrawvline(ic, px, center_top - 1, y_top, hour_color);
				}

				if (i * barwidth > imagefontwidth(ic, FONT_ROLE_AXIS)) {
					int label_x, label_y, label_color;

					snprintf(buffer, 32, "%02d", d->tm_hour);
					if (ic->fontctx.mode == FONT_TTF) {
						label_x = px - imagetextwidth(ic, FONT_ROLE_AXIS, buffer) / 2;
						/* Hourly uses axis+imageuipx(8); keep labels clear of the x-axis line */
						label_y = center_y + txh + FIVEMINHEIGHTOFFSET + imageuipx(ic, 8);
					} else {
						label_x = px - imagefontwidth(ic, FONT_ROLE_AXIS) + 1;
						label_y = center_y + txh + imagefontheight(ic, FONT_ROLE_AXIS) - imageextrapx(ic, 5);
					}
					if (datalist_i->timestamp > timestamp) {
						label_color = ic->cline;
					} else {
						label_color = ic->ctext;
					}
					imagestring(ic, FONT_ROLE_AXIS, label_x, label_y, buffer, label_color);
				}
			} else {
				if (y_bot > center_bot + 1) {
					imagedrawvline(ic, px, y_bot, center_bot + 1, ic->cbgoffset);
				}
				if (y_top < center_top - 1) {
					imagedrawvline(ic, px, center_top - 1, y_top, ic->cbgoffset);
				}
			}
		}

		if (datalist_i->timestamp > timestamp) {
			gdImageSetPixel(ic->im, px, center_y, ic->cline);
			gdImageSetPixel(ic->im, px, center_y + txh + FIVEMINHEIGHTOFFSET, ic->cline);
			continue;
		}

		/* only the last entry can be the currently ongoing period that may need scaling */
		if (datalist_i->next == NULL && issametimeslot(LT_5min, datalist_i->timestamp, ic->interface.updated)) {
			e = (double)(ic->interface.updated - datalist_i->timestamp) / (double)300;
			if (e < 0.01) {
				e = 1;
			}
		} else {
			e = 1;
		}

		t = (int)lrint(((double)datalist_i->rx / e / (double)datainfo.maxrx) * rxh);
		if (t > rxh) {
			t = rxh;
		}
		for (b = 0; b < barwidth; b++) {
			drawpole(ic, px + b, center_y - 1, t, 1, ic->crx);
		}

		t = (int)lrint(((double)datalist_i->tx / e / (double)datainfo.maxtx) * txh);
		if (t > txh) {
			t = txh;
		}
		for (b = 0; b < barwidth; b++) {
			drawpole(ic, px + b, center_y + 1, t, 2, ic->ctx);
		}

		datalist_i = datalist_i->next;
	}

	/* Redraw axes last so thick grid marks cannot nick the strokes (mirrors hourly). */
	imagedrawhline(ic, axis_base_x, axis_base_x + plot_w + pad_full, ypos, ic->ctext);
	imagedrawvline(ic, axis_stem_x, ypos + cross, ypos - height, ic->ctext);
	imagedrawhline(ic, hline_x0, plot_x0 + plot_w + pad_inner, center_y, ic->ctext);
	drawarrowup(ic, axis_stem_x, ypos - height);
	drawarrowright(ic, axis_base_x + plot_w + pad_full, ypos);

	dbdatalistfree(&datalist);

	return 1;
}

/* Pixels per hour. Builtin -L doubles; TTF widens when day labels need a gap. */
static int percentile_barwidth(IMAGECONTENT *ic)
{
	int label_w, gap, need, slot = 24; /* day labels every 24 hours */

	if (ic->fontctx.mode == FONT_BUILTIN) {
		return 1 + imageextrapx(ic, 1);
	}

	/* Gap grows with label size (min 4px) so large fonts stay readable between days */
	label_w = imagetextwidth(ic, FONT_ROLE_AXIS, "00");
	gap = label_w / 4;
	if (gap < 4) {
		gap = 4;
	}
	need = label_w + gap;
	return (need + slot - 1) / slot;
}

void draw95thpercentilegraph(IMAGECONTENT *ic, const int mode)
{
	int imagewidth, imageheight, headermod = 0, header_extra = 0;
	int bottom, legend_y, graph_height, barwidth, base_graph, top;
	uint64_t percentile = 0;

	barwidth = percentile_barwidth(ic);
	imagewidth = PERCENTILEENTRYCOUNT * barwidth + graph_extra_space(ic);

	/* Plot height from base size (default top 38 + bottom 30), then * barwidth.
	 * Do not derive from imageheight - chrome, or large fonts shrink the plot first. */
	base_graph = 300 - 68;
	if (base_graph < 1) {
		base_graph = 1;
	}
	graph_height = base_graph * barwidth;

	if (!ic->showheader) {
		headermod = ic->fontctx.header_h - 2;
		top = 38 - headermod;
		if (top < 2) {
			top = 2;
		}
		if (ic->fontctx.mode == FONT_TTF) {
			int min_top = imageuipx(ic, 8) + ic->fontctx.axis_ch / 2;
			if (top < min_top) {
				top = min_top;
			}
		}
	} else {
		header_extra = ic->fontctx.header_h - 24;
		if (header_extra < 0) {
			header_extra = 0;
		}
		top = 38 + header_extra;
		if (ic->fontctx.mode == FONT_TTF) {
			int min_top = ic->fontctx.header_h + imageuipx(ic, 8) + ic->fontctx.axis_ch / 2;
			if (top < min_top) {
				top = min_top;
			}
		}
	}

	bottom = 30 + imageextrapx(ic, 8);

	if (ic->fontctx.mode == FONT_TTF) {
		int needed_bottom;

		/* Labels at ypos+imageuipx(8); leave room for legend + footer */
		needed_bottom = imageuipx(ic, 8) + ic->fontctx.axis_ch + imageuipx(ic, 4) + ic->fontctx.ch + imageuipx(ic, 4) + imageuipx(ic, 12) + ic->showedge;
		if (needed_bottom > bottom) {
			bottom = needed_bottom;
		}
	}

	imageheight = top + graph_height + bottom;

	if (ic->fontctx.mode == FONT_TTF) {
		legend_y = imageheight - ic->showedge - (int)(ic->fontctx.ch * 1.5);
	} else {
		legend_y = imageheight - 17 - imageextrapx(ic, 2);
	}

	imageinit(ic, imagewidth, imageheight);
	layoutinit(ic, " / 95th percentile", imagewidth, imageheight);

	if (drawpercentile(ic, mode, graph_xpos_margin(ic), imageheight - bottom, graph_height, &percentile)) {
		drawpercentilelegend(ic, imagewidth / 2 - imageextrapx(ic, 50), legend_y, mode, percentile);
	}
}

int drawpercentile(IMAGECONTENT *ic, const int mode, const int xpos, const int ypos, const int height, uint64_t *percentile)
{
	int i, l, b, x = xpos, y = ypos, s = 0, step = 1, prev = 0, last = 0, color, cross;
	int barwidth, plot_w, px, label_x, label_y, line_y, pad_full;
	int axis_base_x, axis_stem_x, hline_x0, axis_y, stroke_half;
	uint64_t scaleunit, max, percentile_val;
	double ratediv, percentileratediv;
	const struct tm *d;
	const char *val;
	time_t current;
	char datebuff[DATEBUFFLEN];
	dbdatalist *datalist = NULL, *datalist_i = NULL;
	dbdatalistinfo datainfo;
	percentiledata pdata;

	barwidth = percentile_barwidth(ic);
	plot_w = PERCENTILEENTRYCOUNT * barwidth;
	pad_full = imageuipx(ic, PERCENTILEMINWIDTHFULLPADDING);

	if (cfg.fiveminutehours < PERCENTILEENTRYCOUNT) {
		fprintf(stderr, "\nWarning: Configuration \"5MinuteHours\" needs to be at least %d for 100%% coverage.\n", PERCENTILEENTRYCOUNT);
		fprintf(stderr, "         \"5MinuteHours\" is currently set at %d.\n\n", cfg.fiveminutehours);
	}

	if (!getpercentiledata(&pdata, ic->interface.name, 0)) {
		imagestring(ic, FONT_ROLE_BODY, x + plot_w / 2 - 30 * ic->fontctx.cw / 2, y - height / 2 - ic->fontctx.ch, "failed to get percentile data", ic->ctext);
		return 0;
	}

	/* hourly/percentile bytes to rate */
	if (cfg.rateunit) {
		ratediv = 450;
		percentileratediv = 37.5;
	} else {
		ratediv = 3600;
		percentileratediv = 300;
	}

	d = localtime(&pdata.monthbegin);
	strftime(datebuff, DATEBUFFLEN, "%Y-%m-%d", d);

	if (!db_getdata_range(&datalist, &datainfo, ic->interface.name, "percentile", PERCENTILEENTRYCOUNT, datebuff, "") || datainfo.count == 0) {
		imagestring(ic, FONT_ROLE_BODY, x + plot_w / 2 - 30 * ic->fontctx.cw / 2, y - height / 2 - ic->fontctx.ch, "no percentile data available", ic->ctext);
		return 0;
	}

	if (debug) {
		printf("mode:  %d - %d\n", mode, cfg.qmode);
		printf("count: %" PRIu32 "\n", datainfo.count);
		printf("barwidth: %d\n", barwidth);
	}

	if (mode == 0) {
		color = ic->crx;
		percentile_val = pdata.rxpercentile;
		max = (uint64_t)((double)datainfo.maxrx / ratediv);
	} else if (mode == 1) {
		color = ic->ctx;
		percentile_val = pdata.txpercentile;
		max = (uint64_t)((double)datainfo.maxtx / ratediv);
	} else {
		color = ic->ctotal;
		percentile_val = pdata.sumpercentile;
		max = (uint64_t)((double)datainfo.max / ratediv);
	}

	if ((uint64_t)((double)(percentile_val) / percentileratediv) > max) {
		max = (uint64_t)((double)(percentile_val) / percentileratediv);
	}

	/* scale values */
	scaleunit = getscale(max, 1);

	s = (int)lrint(((double)scaleunit / (double)max) * height);
	if (s == 0) {
		s = 1;
	}
	{
		int min_step_px = SCALEMINPIXELS;

		/* Tall TTF axis digits need more vertical space between scale labels */
		if (ic->fontctx.mode == FONT_TTF && ic->fontctx.axis_ch + 4 > min_step_px) {
			min_step_px = ic->fontctx.axis_ch + 4;
		}
		while (s * step < min_step_px) {
			step++;
		}
	}

	/* scale text */
	graph_draw_axis_unit(ic, xpos, x - 2 - imageextrapx(ic, 14), y - (height / 2), getimagescale(scaleunit * (unsigned int)step, 1));

	/* axis */
	cross = imageuipx(ic, GRAPH_AXIS_CROSS);
	stroke_half = graph_stroke_half(ic);
	x += graph_axis_left(ic);
	axis_base_x = x;
	axis_stem_x = x + cross;
	axis_y = y;
	imagedrawhline(ic, x, x + (plot_w + pad_full), y, ic->ctext);
	imagedrawvline(ic, x + cross, y + cross, y - height, ic->ctext);

	/* arrows — tip at axis endpoint so the head is not inset past the stroke */
	drawarrowup(ic, x + cross, y - height);
	drawarrowright(ic, x + (plot_w + pad_full), y);

	/* adjust cursor to first point on graph (1px past stem) */
	x += cross + 1;
	hline_x0 = x + stroke_half;
	y -= 1;

	for (i = step; i * s <= height; i = i + step) {
		line_y = y - (i * s);
		imagedrawdashedhline(ic, hline_x0, x + (plot_w + pad_full) - 5, line_y, ic->cline);
		imagedrawdashedhline(ic, hline_x0, x + (plot_w + pad_full) - 5, y - prev - (step * s) / 2, ic->clinel);
		val = getimagevalue(scaleunit * (unsigned int)i, 3, 1);
		graph_draw_axis_value(ic, x - 1, line_y, val, x - 22 - imageextrapx(ic, 3), line_y - 4 - imageextrapx(ic, 3));
		prev = i * s;
	}
	if ((prev + (step * s) / 2) <= height) {
		imagedrawdashedhline(ic, hline_x0, x + (plot_w + pad_full) - 5, y - prev - (step * s) / 2, ic->clinel);
	}

	datalist_i = datalist;
	current = pdata.monthbegin;
	prev = -24;

	/* draw data */
	for (i = 0; i < PERCENTILEENTRYCOUNT; i++, current += 3600) {
		px = x + i * barwidth;

		if (datalist_i == NULL || current < datalist_i->timestamp) {
			for (b = 0; b < barwidth; b++) {
				gdImageSetPixel(ic->im, px + b, y + 1, ic->cbgoffset);
			}
			if (i >= prev + 24 && i % 24 == 0 && current < pdata.dataend) {
				d = localtime(&current);
				strftime(datebuff, DATEBUFFLEN, "%d", d);
				if (i > 0) {
					drawpole(ic, px, y + 4, height, 1, ic->cbgoffset);
				}
				if (ic->fontctx.mode == FONT_TTF) {
					label_x = px + 12 * barwidth - imagetextwidth(ic, FONT_ROLE_AXIS, datebuff) / 2;
					label_y = y + imageuipx(ic, 8);
				} else {
					label_x = px + 12 - 4 - imageextrapx(ic, 1);
					label_y = y + 5;
				}
				imagestring(ic, FONT_ROLE_AXIS, label_x, label_y, datebuff, ic->cline);
				prev = i;
			}
			continue;
		}

		if (i >= prev + 24 && i % 24 == 0) {
			d = localtime(&current);
			strftime(datebuff, DATEBUFFLEN, "%d", d);
			drawpole(ic, px, y, height, 1, ic->cbgoffset);
			if (i > 0) {
				imagedrawvline(ic, px, y + 1, y + 4, ic->ctext);
			}
			if (ic->fontctx.mode == FONT_TTF) {
				label_x = px + 12 * barwidth - imagetextwidth(ic, FONT_ROLE_AXIS, datebuff) / 2;
				label_y = y + imageuipx(ic, 8);
			} else {
				label_x = px + 12 - 4 - imageextrapx(ic, 1);
				label_y = y + 5;
			}
			imagestring(ic, FONT_ROLE_AXIS, label_x, label_y, datebuff, ic->ctext);
			prev = i;
		}

		if (mode == 0) {
			l = (int)lrint(((double)(datalist_i->rx) / (double)ratediv / (double)max) * height);
		} else if (mode == 1) {
			l = (int)lrint(((double)(datalist_i->tx) / (double)ratediv / (double)max) * height);
		} else {
			l = (int)lrint(((double)(datalist_i->rx + datalist_i->tx) / (double)ratediv / (double)max) * height);
		}
		if (l > height) {
			l = height;
		}
		for (b = 0; b < barwidth; b++) {
			drawpole(ic, px + b, y, l, 1, color);
		}

		last = i;
		datalist_i = datalist_i->next;
	}

	/* Redraw axes so day markers / thick dashes do not nick the strokes. */
	imagedrawhline(ic, axis_base_x, axis_base_x + plot_w + pad_full, axis_y, ic->ctext);
	imagedrawvline(ic, axis_stem_x, axis_y + cross, axis_y - height, ic->ctext);

	dbdatalistfree(&datalist);

	/* 95th percentile line */
	l = (int)lrint(((double)(percentile_val) / percentileratediv / (double)max) * height);
	if (l > height) {
		l = height;
	} else if (l == 0) {
		l = 1;
	}
	imagedrawhline(ic, x, x + (last + 1) * barwidth - 1, y - l, ic->cpercentileline);

	drawarrowup(ic, axis_stem_x, axis_y - height);
	drawarrowright(ic, axis_base_x + plot_w + pad_full, axis_y);

	if (debug) {
		printf("s:   %d\n", s);
		printf("l:   %d\n", l);
		printf("h:   %d\n", height);
		printf("p:   %" PRIu64 "\n", (uint64_t)((double)percentile_val / percentileratediv));
		printf("max: %" PRIu64 "\n", max);
		printf("max rate: %s\n", gettrafficrate((uint64_t)((double)max * ratediv), 3600, 0));
		printf("per rate: %s\n", gettrafficrate(percentile_val, 300, 0));
	}

	if (percentile != NULL) {
		*percentile = percentile_val;
	}

	return 1;
}
