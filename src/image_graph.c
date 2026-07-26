#include "common.h"
#include "dbsql.h"
#include "misc.h"
#include "percentile.h"
#include "image_graph.h"
#include "image_font.h"
#include "image_widget.h"
#include "image_support.h"

int drawhours(IMAGECONTENT *ic, const int xpos, const int ypos, const int israte)
{
	int i, tmax = 0, s = 0, step, prev = 0, diff = 0, chour, cross;
	int x = xpos, y = ypos, extrax = 0, extray = 0, xt = 0;
	int axis_x, axis_y, axis_top, axis_right, dash_right, tick_left, tick_right;
	int pole_pad, hour_step, left_grow, dash_past, axis_past;
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
	/* plot width extra must match sum of hour-gap extras so bars stay aligned with the axis */
	hour_step = hourly_hour_step(ic);
	extrax = hourly_plot_extrax(ic);
	pole_pad = imageextrapx(ic, 2);
	extray = imageextrapx(ic, 35);
	/* TTF: grow left inset and tip past with point size (0 delta at FontSize 12) */
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
	/* hours are shifted right by pole_pad (+ left_grow for TTF) so leftmost poles clear
	 * the y-axis; tip room also grows so rightmost poles do not overrun the axis / grid */
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

	/* rightmost hour column: plot span + gap extras + left pole clearance */
	xt = xt + HOURLY_PLOT_SPAN + extrax + pole_pad + left_grow;

	/* keep alignment when midnight line isn't shown */
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
			/* center "HH" on pole-pair midpoint so digits sit under rx / tx */
			imagestring(ic, FONT_ROLE_AXIS, xt + hourly_map_px(ic, 4) - imagetextwidth(ic, FONT_ROLE_AXIS, buffer) / 2, y + 124 + imageuipx(ic, 8), buffer, chour);
		} else {
			imagestring(ic, FONT_ROLE_AXIS, xt, y + 128, buffer, chour);
		}
		drawpoles(ic, xt - hourly_map_px(ic, 2), y - extray, 124 + extray, hourdata[s].rx, hourdata[s].tx, max);
		if (s == 0 && i != 23) {
			/* midnight line: stop above the thick x-axis band */
			imagedrawvline(ic, xt - hourly_map_px(ic, 5) - imageextrapx(ic, 3), y - 5 - extray, axis_y - graph_stroke_half(ic) - 1, ic->clinel);
			xt--;
		}
		xt = xt - hour_step;
	}

	/* axes + arrows last so poles/grid cannot nick the strokes */
	imagedrawhline(ic, axis_x - cross, axis_right, axis_y, ic->ctext);
	imagedrawvline(ic, axis_x, axis_top, axis_y + cross, ic->ctext);
	drawarrowup(ic, axis_x, axis_top);
	drawarrowright(ic, axis_right, axis_y);

	/* hour ticks on the axis after the final redraw so missing-data hours dim
	 * that segment (ctext ticks are a no-op) */
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
		/* keep hour ticks from extending past the axis end (and over the arrow) */
		tick_left = xt - cross - imageextrapx(ic, 3);
		tick_right = xt + hourly_map_px(ic, 12) - hourly_map_px(ic, 2) + imageextrapx(ic, 3);
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
	int ypos, axis_top_base, min_axis_top, graph_left, graph_y;

	graph_left = hourly_graph_left(ic);
	width = hourly_graph_width(ic);
	if (ic->commonwidth > 0) {
		width = ic->commonwidth;
	}
	height = 200 + imageextrapx(ic, 48);

	if (!ic->showheader) {
		/* graph_y is from a 24px-header layout; shift by that chrome only,
		 * full TTF header_h would over-shift the plot into the top edge */
		headermod = 24 + imageuipx(ic, 2);
		/* base height assumes the 24px built-in header; TTF header growth is
		 * only applied when the header is shown (else branch) */
		height -= 24 - 2;
	} else {
		/* axis_top = ypos - 10 - extray; keep the up-arrow below the header,
		 * imageextrapx(40)-imageextrapx(35) alone does not track header_h */
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

	graph_y = 46 + header_extra - headermod + imageextrapx(ic, 40);

	/* TTF: room below the x-axis for hour labels, legend, and footer clearance */
	if (ic->fontctx.mode == FONT_TTF) {
		int label_bottom, min_height;

		label_bottom = graph_y + 124 + imageuipx(ic, 8) + ic->fontctx.axis_ch;
		/* legend_y = height - showedge - ch*3/2; keep a gap above the legend */
		min_height = label_bottom + imageuipx(ic, 8) + ic->showedge + ic->fontctx.ch * 3 / 2;
		if (height < min_height) {
			height = min_height;
		}
	}

	imageinit(ic, width, height);
	layoutinit(ic, " / hourly", width, height);

	if (drawhours(ic, graph_left, graph_y, israte)) {
		if (ic->fontctx.mode == FONT_TTF) {
			drawlegend(ic, width / 2 - imageextrapx(ic, 10), height - ic->showedge - ic->fontctx.ch * 3 / 2, 0);
		} else {
			drawlegend(ic, width / 2 - imageextrapx(ic, 10), 183 - headermod + imageextrapx(ic, 46), 0);
		}
	}
}


/* space needed for one axis hour label ("00") plus a readable gap */
static int fiveg_label_need(IMAGECONTENT *ic)
{
	int label_w, gap;

	label_w = imagetextwidth(ic, FONT_ROLE_AXIS, "00");
	gap = label_w / 4;
	if (gap < 4) {
		gap = 4;
	}
	return label_w + gap;
}

/* pixels per 5-minute sample, built-in stays 1px (master sizing); standalone TTF
 * widens for 2-hour labels, embedded -vs/-hs use summary_fiveg_barwidth() */
int fiveg_barwidth(IMAGECONTENT *ic)
{
	int need, slot = 24; /* hour labels every 2 hours = 24 samples */

	if (ic->fontctx.mode == FONT_BUILTIN) {
		return 1;
	}

	need = fiveg_label_need(ic);
	return (need + slot - 1) / slot;
}

/* hours between x-axis labels so "00"+gap fits in hours*12*barwidth pixels */
static int fiveg_label_hours(IMAGECONTENT *ic, const int barwidth)
{
	static const int intervals[] = {2, 4, 6, 12};
	int need, i, bw, slot;

	if (ic->fontctx.mode == FONT_BUILTIN) {
		return 2;
	}

	bw = barwidth;
	if (bw < 1) {
		bw = 1;
	}
	need = fiveg_label_need(ic);
	for (i = 0; i < 4; i++) {
		slot = intervals[i] * 12 * bw;
		if (slot >= need) {
			return intervals[i];
		}
	}
	return 12;
}


void drawfivegraph(IMAGECONTENT *ic, const int israte, const int resultcount, const int height)
{
	int imagewidth, imageheight, headermod = 0, header_extra = 0;
	int bottom, legend_y, graph_height, barwidth, base_graph, top;
	int count = resultcount;

	barwidth = fiveg_barwidth(ic);
	if (ic->commonwidth > 0 && barwidth > 0) {
		int extra = graph_extra_space(ic);
		int usable = ic->commonwidth - extra;

		if (usable < barwidth) {
			usable = barwidth;
		}
		count = usable / barwidth;
		if (count < FIVEGMINRESULTCOUNT) {
			count = FIVEGMINRESULTCOUNT;
		}
	}
	imagewidth = count * barwidth + graph_extra_space(ic);
	if (ic->commonwidth > 0 && imagewidth < ic->commonwidth) {
		imagewidth = ic->commonwidth;
	}

	/* plot height from configured size (default top 38 + bottom 30), then * barwidth,
	 * do not derive from imageheight - chrome, or large fonts shrink the plot first */
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
			/* clear header for up-arrow and topmost scale label ascent */
			int min_top = ic->fontctx.header_h + imageuipx(ic, 8) + ic->fontctx.axis_ch / 2;
			if (top < min_top) {
				top = min_top;
			}
		}
	}

	bottom = 30 + imageextrapx(ic, 8);

	if (ic->fontctx.mode == FONT_TTF) {
		int needed_bottom;

		/* labels at ypos+imageuipx(8); leave room for legend + footer */
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

	if (drawfiveminutes(ic, graph_xpos_margin(ic), imageheight - bottom, israte, count, graph_height, barwidth)) {
		drawlegend(ic, imagewidth / 2 - imageextrapx(ic, 10), legend_y, 0);
	}
}

int drawfiveminutes(IMAGECONTENT *ic, const int xpos, const int ypos, const int israte, const int resultcount, const int height, const int barwidth)
{
	int x = xpos, y = ypos, i = 0, t = 0, rxh = 0, txh = 0, step = 0, s = 0, prev = 0, cross;
	int plot_w, b, px, axis_left, unit_x, pad_full, pad_inner, label_hours, bw;
	int axis_base_x, axis_stem_x, plot_x0, hline_x0, center_y, stroke_half;
	uint64_t scaleunit, max;
	time_t timestamp;
	double ratediv, e;
	char buffer[32];
	const struct tm *d;
	dbdatalist *datalist = NULL, *datalist_i = NULL;
	dbdatalistinfo datainfo;

	bw = barwidth;
	if (bw < 1) {
		bw = 1;
	}
	label_hours = fiveg_label_hours(ic, bw);
	plot_w = resultcount * bw;
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

	/* arrows: tip at axis endpoint so the head is not inset past the stroke */
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

		/* tall TTF axis digits need more vertical space between scale labels */
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
		printf("barwidth: %d\n", bw);
		printf("label_hours: %d\n", label_hours);
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
		px = plot_x0 + i * bw;

		if (d->tm_min == 0 && i > 2) {
			/* split around thick zero-line and bottom x-axis so hour marks do not punch holes */
			int line_t = imageuipx(ic, 1);
			int center_top = center_y - line_t / 2;
			int center_bot = center_top + line_t - 1;
			int y_bot = ypos - stroke_half - 1;
			int y_top = center_y - rxh - 1;
			/* imagedrawvline centers a thick stroke; shift so the left edge sits on
			 * the bar boundary and cannot nick the previous sample's poles */
			int line_x = px + line_t / 2;
			int hour_color;
			int is_label_hour = (label_hours > 0 && (d->tm_hour % label_hours) == 0);

			if (d->tm_hour % 2 == 0) {
				hour_color = (d->tm_hour == 0) ? ic->cline : ic->cbgoffset;
				if (y_bot > center_bot + 1) {
					imagedrawvline(ic, line_x, y_bot, center_bot + 1, hour_color);
				}
				if (y_top < center_top - 1) {
					imagedrawvline(ic, line_x, center_top - 1, y_top, hour_color);
				}

				if (is_label_hour && i * bw > imagefontwidth(ic, FONT_ROLE_AXIS)) {
					int label_x, label_y, label_color;

					snprintf(buffer, 32, "%02d", d->tm_hour);
					if (ic->fontctx.mode == FONT_TTF) {
						label_x = px - imagetextwidth(ic, FONT_ROLE_AXIS, buffer) / 2;
						/* hourly uses axis+imageuipx(8); keep labels clear of the x-axis line */
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
					imagedrawvline(ic, line_x, y_bot, center_bot + 1, ic->cbgoffset);
				}
				if (y_top < center_top - 1) {
					imagedrawvline(ic, line_x, center_top - 1, y_top, ic->cbgoffset);
				}
			}
		}

		if (datalist_i->timestamp > timestamp) {
			/* scale future / no-data marks with UI stroke thickness */
			int t = imageuipx(ic, 1);
			int x0 = px - t / 2;
			int y0 = center_y - t / 2;
			int y1 = center_y + txh + FIVEMINHEIGHTOFFSET - t / 2;

			gdImageFilledRectangle(ic->im, x0, y0, x0 + t - 1, y0 + t - 1, ic->cline);
			gdImageFilledRectangle(ic->im, x0, y1, x0 + t - 1, y1 + t - 1, ic->cline);
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
		for (b = 0; b < bw; b++) {
			drawpole(ic, px + b, center_y - 1, t, 1, ic->crx);
		}

		t = (int)lrint(((double)datalist_i->tx / e / (double)datainfo.maxtx) * txh);
		if (t > txh) {
			t = txh;
		}
		for (b = 0; b < bw; b++) {
			drawpole(ic, px + b, center_y + 1, t, 2, ic->ctx);
		}

		datalist_i = datalist_i->next;
	}

	/* redraw axes last so thick grid marks cannot nick the strokes (mirrors hourly) */
	imagedrawhline(ic, axis_base_x, axis_base_x + plot_w + pad_full, ypos, ic->ctext);
	imagedrawvline(ic, axis_stem_x, ypos + cross, ypos - height, ic->ctext);
	imagedrawhline(ic, hline_x0, plot_x0 + plot_w + pad_inner, center_y, ic->ctext);
	drawarrowup(ic, axis_stem_x, ypos - height);
	drawarrowright(ic, axis_base_x + plot_w + pad_full, ypos);

	dbdatalistfree(&datalist);

	return 1;
}

/* pixels per hour, built-in stays 1px (master sizing); TTF widens for day labels */
static int percentile_barwidth(IMAGECONTENT *ic)
{
	int label_w, gap, need, slot = 24; /* day labels every 24 hours */

	if (ic->fontctx.mode == FONT_BUILTIN) {
		return 1;
	}

	/* gap grows with label size (min 4px) so large fonts stay readable between days */
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

	/* plot height from base size (default top 38 + bottom 30), then * barwidth,
	 * do not derive from imageheight - chrome, or large fonts shrink the plot first */
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

		/* labels at ypos+imageuipx(8); leave room for legend + footer */
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
		int legend_x;

		/* built-in: historical half-width guess, TTF: measure real legend width */
		if (ic->fontctx.mode == FONT_TTF) {
			legend_x = imagewidth / 2 - percentilelegendwidth(ic, mode, percentile) / 2;
		} else {
			legend_x = imagewidth / 2 - imageextrapx(ic, 50);
		}
		drawpercentilelegend(ic, legend_x, legend_y, mode, percentile);
	}
}

int drawpercentile(IMAGECONTENT *ic, const int mode, const int xpos, const int ypos, const int height, uint64_t *percentile)
{
	int i, l, b, x = xpos, y = ypos, s = 0, step = 1, prev = 0, last = 0, color, cross;
	int barwidth, plot_w, px, label_x, label_y, line_y, pad_full;
	int axis_base_x, axis_stem_x, hline_x0, dash_x1, axis_y, stroke_half;
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

		/* tall TTF axis digits need more vertical space between scale labels */
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

	/* arrows: tip at axis endpoint so the head is not inset past the stroke */
	drawarrowup(ic, x + cross, y - height);
	drawarrowright(ic, x + (plot_w + pad_full), y);

	/* adjust cursor to first point on graph (1 px past stem) */
	x += cross + 1;
	hline_x0 = x + stroke_half;
	/* axis tip; do not use x after the cross+1 advance with a fixed inset */
	dash_x1 = axis_base_x + plot_w + pad_full;
	y -= 1;

	for (i = step; i * s <= height; i = i + step) {
		line_y = y - (i * s);
		imagedrawdashedhline(ic, hline_x0, dash_x1, line_y, ic->cline);
		imagedrawdashedhline(ic, hline_x0, dash_x1, y - prev - (step * s) / 2, ic->clinel);
		val = getimagevalue(scaleunit * (unsigned int)i, 3, 1);
		graph_draw_axis_value(ic, x - 1, line_y, val, x - 22 - imageextrapx(ic, 3), line_y - 4 - imageextrapx(ic, 3));
		prev = i * s;
	}
	if ((prev + (step * s) / 2) <= height) {
		imagedrawdashedhline(ic, hline_x0, dash_x1, y - prev - (step * s) / 2, ic->clinel);
	}

	datalist_i = datalist;
	current = pdata.monthbegin;
	prev = -24;

	/* draw data */
	for (i = 0; i < PERCENTILEENTRYCOUNT; i++, current += 3600) {
		int day_boundary = 0;

		px = x + i * barwidth;

		if (datalist_i == NULL || current < datalist_i->timestamp) {
			for (b = 0; b < barwidth; b++) {
				gdImageSetPixel(ic->im, px + b, y + 1, ic->cbgoffset);
			}
			if (i >= prev + 24 && i % 24 == 0 && current < pdata.dataend) {
				d = localtime(&current);
				strftime(datebuff, DATEBUFFLEN, "%d", d);
				if (i > 0) {
					/* left-align thick stroke on bar edge (same as 5min hour marks) */
					imagedrawvline(ic, px + stroke_half, y - height + 1, y, ic->cbgoffset);
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
			day_boundary = 1;
			d = localtime(&current);
			strftime(datebuff, DATEBUFFLEN, "%d", d);
			if (i > 0) {
				imagedrawvline(ic, px + stroke_half, y + 1, y + imageuipx(ic, 4), ic->ctext);
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

		/* day separator above the pole only (like 5min hour marks: never paint over traffic) */
		if (day_boundary && i > 0) {
			int sep_top = y - height + 1;
			int sep_bottom = y - l;

			if (sep_bottom >= sep_top) {
				/* left-align thick stroke on bar edge so it cannot nick the previous pole */
				imagedrawvline(ic, px + stroke_half, sep_top, sep_bottom, ic->cbgoffset);
			}
		}

		for (b = 0; b < barwidth; b++) {
			drawpole(ic, px + b, y, l, 1, color);
		}

		last = i;
		datalist_i = datalist_i->next;
	}

	/* redraw axes so day markers / thick dashes do not nick the strokes */
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
