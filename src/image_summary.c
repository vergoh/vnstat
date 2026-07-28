#include "common.h"
#include "dbsql.h"
#include "misc.h"
#include "image_summary.h"
#include "image_graph.h"
#include "image_font.h"
#include "image_widget.h"
#include "image_support.h"

/* embedded summary sample count, built-in small/large maps to 422/576;
 * TTF keeps a fixed 576 so FontSize does not inflate sample history */
static int summary_fiveg_samples(const IMAGECONTENT *ic)
{
	if (ic->fontctx.mode == FONT_BUILTIN) {
		return 422 + imageextrapx(ic, 154);
	}
	return 576;
}

/* matches old left-aligned "rx " + getvalue(..., 12) width ("  999.99 YiB" pad) */
static const char summary_stack_sample[] = "rx   999.99 YiB";

/* left inset of digest/all time stacks (digest_x - bodyoff) */
static int summary_ttf_content_left(const IMAGECONTENT *ic)
{
	const int digest_x = (14 * ic->fontctx.cw + 2) + 26;
	const int bodyoff = 12 * ic->fontctx.cw + 2;

	return digest_x - bodyoff;
}

/* right edge of a value stack including the +2*cw pad used for values/since/rate */
static int summary_ttf_stack_right(IMAGECONTENT *ic, const int body_left)
{
	return body_left + imagetextwidth(ic, FONT_ROLE_BODY, summary_stack_sample) + 2 * ic->fontctx.cw;
}

/* right edge of TTF drawlegend at legend_x (non-rate) */
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

/* right ink of the second digest column donut (normal two-entry case) */
static int summary_ttf_digest_right(IMAGECONTENT *ic, const int digest_x)
{
	const int bodyoff = 12 * ic->fontctx.cw + 2;
	const int donut_size = 49 + imageextrapx(ic, 10);
	const int textx = digest_x + 30 * ic->fontctx.cw;
	const int body_left = textx - bodyoff;
	const int col_right = body_left + imagetextwidth(ic, FONT_ROLE_BODY, summary_stack_sample);

	/* center at col_right + donut_size; radius donut_size/2 */
	return col_right + donut_size + donut_size / 2;
}

/* design time 15px gap below the header (or from the top with --noheader) */
static int summary_ttf_top_gap(const IMAGECONTENT *ic)
{
	return imageuipx(ic, 15);
}

/* extra Y so summary / hourly plot track the scaled top gap (0 at FontSize 12) */
static int summary_ttf_top_gap_extra(const IMAGECONTENT *ic)
{
	return summary_ttf_top_gap(ic) - 15;
}

/* vertical extents of drawhours() relative to ypos: tip is ypos-above, labels end at ypos+below */
static void hourly_graph_y_extents(const IMAGECONTENT *ic, int *above, int *below)
{
	*above = 10 + imageextrapx(ic, 35);
	*below = 124 + imageuipx(ic, 8) + ic->fontctx.axis_ch;
}

/* vertical extents of embedded 5-min plot relative to ypos (x-axis):
 * tip at ypos-plot_h; leave imageuipx(8)+axis_ch/2 above tip for arrow and
 * top scale labels (same clearance as standalone -5g) */
static void summary_hs_fiveg_y_extents(const IMAGECONTENT *ic, const int plot_h, int *above, int *below)
{
	*above = plot_h + imageuipx(ic, 8) + ic->fontctx.axis_ch / 2;
	*below = imageuipx(ic, 8) + ic->fontctx.axis_ch;
}

/* tiny footer clearance matching layoutinit() generator placement */
static int summary_hs_footer_clearance(const IMAGECONTENT *ic)
{
	return ic->fontctx.footer->h + 4 + ic->showedge * imageuipx(ic, 1);
}

/* content band for embedding the hourly plot in -hs (below header, above bottom edge/note) */
static void summary_hs_hourly_content_band(const IMAGECONTENT *ic, const int height,
	const int monthrotatenotevisible, int *content_top, int *content_bottom)
{
	if (ic->showheader) {
		*content_top = ic->fontctx.header_h;
	} else {
		*content_top = ic->showedge * imageuipx(ic, 1);
	}
	/* center to the image edge so header→poles and labels→edge gaps match */
	*content_bottom = height - ic->showedge;
	if (monthrotatenotevisible) {
		*content_bottom -= ic->lineheight;
	}
}

/* embedded hourly ypos for -hs (layout 1), TTF: center pole-tops→labels in the content band */
static int summary_hours_y_hs(const IMAGECONTENT *ic, const int header_extra, const int graph_headermod,
	const int height, const int monthrotatenotevisible)
{
	int hours_y = 46 + header_extra + imageextrapx(ic, 40) - graph_headermod;

	if (ic->fontctx.mode == FONT_TTF) {
		int tip_above, pole_above, below, span, content_top, content_bottom, available;
		int min_hours_y, max_hours_y, bottom_clear;

		hourly_graph_y_extents(ic, &tip_above, &below);
		/* center the visible plot (pole tops → hour labels), the axis tip sits
		 * a fixed 10px above the poles and is only clamped into the band */
		pole_above = tip_above - 10;
		if (pole_above < 0) {
			pole_above = 0;
		}
		span = pole_above + below;
		summary_hs_hourly_content_band(ic, height, monthrotatenotevisible, &content_top, &content_bottom);
		available = content_bottom - content_top;
		if (available > span) {
			hours_y = content_top + (available - span) / 2 + pole_above;
		} else {
			hours_y = content_top + tip_above;
		}

		min_hours_y = content_top + tip_above;
		bottom_clear = summary_hs_footer_clearance(ic);
		if (monthrotatenotevisible) {
			bottom_clear += ic->lineheight;
		}
		max_hours_y = height - bottom_clear - below;
		if (hours_y < min_hours_y) {
			hours_y = min_hours_y;
		}
		if (hours_y > max_hours_y) {
			hours_y = max_hours_y;
		}
	}
	return hours_y;
}

/* embedded hourly ypos for -vs (layout 2), axis tip is at ypos-10-extray; keep it
 * below the month digest (rate line) after the scaled top gap moves that block */
static int summary_hours_y_vs(const IMAGECONTENT *ic, const int header_extra, const int graph_headermod,
	const int monthrotatenotevisible, const int digest_month_y)
{
	int hours_y = 215 + header_extra + imageextrapx(ic, 84) - graph_headermod
		+ (monthrotatenotevisible * (ic->lineheight * 2));

	if (ic->fontctx.mode == FONT_TTF) {
		const int extray = imageextrapx(ic, 35);
		/* rate line + body ink; axis tip is ypos-10-extray and scale labels sit near it */
		const int clear = digest_month_y + 5 * ic->lineheight + 10 + ic->fontctx.ch
			+ ic->fontctx.axis_ch + imageuipx(ic, 12);
		int min_hours_y;

		hours_y += summary_ttf_top_gap_extra(ic);
		min_hours_y = clear + 10 + extray;
		if (hours_y < min_hours_y) {
			hours_y = min_hours_y;
		}
	}
	return hours_y;
}

static void summary_ttf_set_positions(IMAGECONTENT *ic, const int headermod,
	int *digest_x, int *alltime_x, int *legend_x, int *graph_x, int *fivegraph_x,
	int *digest_day_y, int *digest_month_y, int *alltime_y, int *legend_y)
{
	*alltime_x = 66 * ic->fontctx.cw;
	*legend_x = 69 * ic->fontctx.cw;
	*graph_x = 84 * ic->fontctx.cw;
	*fivegraph_x = *graph_x;
	/* body at textx - (12*cw+2) stays near the built-in left margin (~26). */
	*digest_x = (14 * ic->fontctx.cw + 2) + 26;
	/* scale with FontSize so TITLE role text clears the edge (fixed 15px clips) */
	*digest_day_y = ic->fontctx.header_h + summary_ttf_top_gap(ic) - headermod;
	*digest_month_y = *digest_day_y - 1 + 8 * ic->lineheight;
	*alltime_y = *digest_day_y + 27 + imageextrapx(ic, 10);
	/* under all time "since" line, matching built-in legend vs since gap */
	*legend_y = *alltime_y + 9 * ic->lineheight;
}

/* text-only summary content width (stacks/legend/digest), excluding embedded graphs */
static int summary_ttf_text_width(IMAGECONTENT *ic, const int digest_x, const int alltime_x, const int legend_x)
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

	width = content_right + content_left;
	if (width < 1) {
		width = 1;
	}
	return width;
}

/* embedded -vs/-hs bar width, TTF grows bars so the plot tracks text-only -vs width;
 * built-in uses fiveg_barwidth() (usually 1), same value for -vs and -hs */
static int summary_fiveg_barwidth(IMAGECONTENT *ic)
{
	int digest_x, alltime_x, legend_x, graph_x, fivegraph_x;
	int digest_day_y, digest_month_y, alltime_y, legend_y;
	int text_w, chrome, samples, usable, bw;

	if (ic->fontctx.mode != FONT_TTF) {
		return fiveg_barwidth(ic);
	}

	summary_ttf_set_positions(ic, 0, &digest_x, &alltime_x, &legend_x, &graph_x, &fivegraph_x,
		&digest_day_y, &digest_month_y, &alltime_y, &legend_y);
	text_w = summary_ttf_text_width(ic, digest_x, alltime_x, legend_x);
	chrome = graph_extra_space(ic);
	samples = summary_fiveg_samples(ic);
	usable = text_w - chrome;
	if (samples < 1 || usable < samples) {
		return 1;
	}
	bw = usable / samples;
	if (bw < 1) {
		bw = 1;
	}
	return bw;
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
			int fiveg_samples = summary_fiveg_samples(ic);

			/* graph_extra_space() balances standalone L/R chrome; -hs only needs a
			 * modest gap past the axis tip, so trim a little of that right pad. */
			r = fivegraph_x + fiveg_samples * fiveg_barwidth_val + graph_extra_space(ic)
				- graph_xpos_margin(ic) - imageuipx(ic, 20);
		} else {
			/* standalone width includes a left-matching right pad; when embedded at
			 * graph_x that pad must not extend past the axis tip (subtract left twice) */
			r = graph_x + hourly_graph_width(ic) - 2 * hourly_graph_left(ic);
		}
		if (r > content_right) {
			content_right = r;
		}
	}

	width = content_right + content_left;
	if (width < 1) {
		width = 1;
	}

	/* vertical graph may be wider than the text block (especially multi-pixel 5-min bars) */
	if (layout == 2) {
		int graph_w;

		if (cfg.summarygraph == 1) {
			graph_w = summary_fiveg_samples(ic) * fiveg_barwidth_val + graph_extra_space(ic);
		} else {
			graph_w = hourly_graph_width(ic);
		}
		if (width < graph_w) {
			width = graph_w;
		}
	}

	return width;
}

int image_summary_width(IMAGECONTENT *ic, const int layout)
{
	int digest_x, alltime_x, legend_x, graph_x, fivegraph_x;
	int digest_day_y, digest_month_y, alltime_y, legend_y;
	int fiveg_barwidth_val = 1;

	if (ic->fontctx.mode != FONT_TTF) {
		switch (layout) {
			case 1:
				return 163 * ic->fontctx.cw + imageuipx(ic, 2) + imageextrapx(ic, 2);
			case 2:
			default:
				return image_list_width(ic);
		}
	}

	if (cfg.summarygraph == 1 && (layout == 1 || layout == 2)) {
		fiveg_barwidth_val = summary_fiveg_barwidth(ic);
	}

	summary_ttf_set_positions(ic, 0, &digest_x, &alltime_x, &legend_x, &graph_x, &fivegraph_x,
		&digest_day_y, &digest_month_y, &alltime_y, &legend_y);
	return summary_ttf_compute_width(ic, layout, digest_x, alltime_x, legend_x, graph_x, fivegraph_x, fiveg_barwidth_val);
}

int image_common_target_width(IMAGECONTENT *ic)
{
	int list_w, summary_w, hourly_w, width;

	list_w = image_list_width(ic);
	summary_w = image_summary_width(ic, 0);
	hourly_w = hourly_graph_width(ic);

	width = list_w;
	if (summary_w > width) {
		width = summary_w;
	}
	if (hourly_w > width) {
		width = hourly_w;
	}
	if (width < 1) {
		width = 1;
	}
	return width;
}

static void summary_ttf_adjust_height(IMAGECONTENT *ic, const int layout,
	const int header_extra, const int graph_headermod, const int monthrotatenotevisible,
	const int digest_month_y, int *height, int *vs_fiveg_bottom)
{
	/* match summary_ttf_set_positions / hourly graph Y shifts below */
	*height += summary_ttf_top_gap_extra(ic);

	if (layout == 2) {
		if (cfg.summarygraph == 1) {
			int bottom_margin, plot_h, clear, needed;

			/* axis labels sit below the 5-min plot; grow margin + canvas together */
			bottom_margin = ic->fontctx.axis_ch + imageuipx(ic, 4) + imageuipx(ic, 12) + ic->showedge;
			if (bottom_margin > *vs_fiveg_bottom) {
				*height += bottom_margin - *vs_fiveg_bottom;
				*vs_fiveg_bottom = bottom_margin;
			}

			/* plot tip must clear the month digest (same clear band as hourly) */
			plot_h = 132 + imageextrapx(ic, 35);
			clear = digest_month_y + 5 * ic->lineheight + 10 + ic->fontctx.ch
				+ ic->fontctx.axis_ch + imageuipx(ic, 12);
			needed = clear + plot_h + *vs_fiveg_bottom;
			if (*height < needed) {
				*height = needed;
			}
		} else {
			int graph_y, needed;

			graph_y = summary_hours_y_vs(ic, header_extra, graph_headermod, monthrotatenotevisible, digest_month_y);
			/* labels at graph_y+124+imageuipx(8); Tiny footer at height-12-showedge */
			needed = graph_y + 124 + imageuipx(ic, 8) + ic->fontctx.axis_ch + imageuipx(ic, 4) + imageuipx(ic, 12) + ic->showedge;
			if (*height < needed) {
				*height = needed;
			}
		}
	} else {
		/* extra bottom pad so rate/legend clear the footer */
		*height += 2 * ic->lineheight;

		/* grow canvas if the embedded plot span cannot fit the content band */
		if (cfg.summarygraph == 0) {
			int above, below, content_top, needed;
			int bottom_clear = summary_hs_footer_clearance(ic);

			if (monthrotatenotevisible) {
				bottom_clear += ic->lineheight;
			}
			hourly_graph_y_extents(ic, &above, &below);
			if (ic->showheader) {
				content_top = ic->fontctx.header_h;
			} else {
				content_top = ic->showedge * imageuipx(ic, 1);
			}
			needed = content_top + above + below + bottom_clear;
			if (*height < needed) {
				*height = needed;
			}
		} else {
			/* 5-min -hs: same content-band fit as hourly (tip→labels + footer) */
			int content_top, above, below, needed;
			int bottom_clear = summary_hs_footer_clearance(ic);
			/* design time plot height used when growing the canvas */
			const int min_plot = 132;

			if (monthrotatenotevisible) {
				bottom_clear += ic->lineheight;
			}
			if (ic->showheader) {
				content_top = ic->fontctx.header_h;
			} else {
				content_top = ic->showedge * imageuipx(ic, 1);
			}
			summary_hs_fiveg_y_extents(ic, min_plot, &above, &below);
			needed = content_top + above + below + bottom_clear;
			if (*height < needed) {
				*height = needed;
			}
		}
	}
}

void drawsummary(IMAGECONTENT *ic, const int layout, const int israte)
{
	int width, height, headermod, graph_headermod, header_extra, digest_x, alltime_x, legend_x, legend_y, graph_x, fivegraph_x;
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
		fiveg_barwidth_val = summary_fiveg_barwidth(ic);
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

	/* tall TTF headers grow the canvas; apply that growth before the noheader
	 * subtraction so --noheader only removes the baseline 24px chrome (same
	 * net as list outputs), not the TTF growth that was never drawn */
	header_extra = ic->fontctx.header_h - 24;
	if (header_extra < 0) {
		header_extra = 0;
	}
	height += header_extra;

	if (!ic->showheader) {
		/* text at header_h + N needs full header_h cancelled; hourly graph
		 * Y (46 / 215) is a 24px-header layout and must not use TTF header_h */
		headermod = ic->fontctx.header_h + imageuipx(ic, 2);
		graph_headermod = 24 + imageuipx(ic, 2);
		/* positioning must not keep header_extra when the bar is absent */
		header_extra = 0;
		height -= ic->fontctx.header_h - 2;
	} else {
		headermod = 0;
		graph_headermod = 0;
	}

	if (ic->fontctx.mode == FONT_TTF) {
		summary_ttf_set_positions(ic, headermod, &digest_x, &alltime_x, &legend_x, &graph_x, &fivegraph_x,
			&digest_day_y, &digest_month_y, &alltime_y, &legend_y);
		summary_ttf_adjust_height(ic, layout, header_extra, graph_headermod, monthrotatenotevisible,
			digest_month_y, &height, &vs_fiveg_bottom);
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

		/* multi-pixel bars widen the 5-min plot; grow canvas so it is not clipped */
		if (cfg.summarygraph == 1 && (layout == 1 || layout == 2)) {
			width += (fiveg_barwidth_val - 1) * summary_fiveg_samples(ic);
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

	/* scale fiveg plot height with barwidth to keep aspect ratio (stable base, not chrome),
	 * built-in multi-pixel bars only; TTF embeds widen bars for width fill without growing height */
	if (cfg.summarygraph == 1 && (layout == 1 || layout == 2) && fiveg_barwidth_val > 1
		&& ic->fontctx.mode == FONT_BUILTIN) {
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

	if (layout == 0 && ic->commonwidth > 0) {
		width = ic->commonwidth;
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
				int fiveg_samples = summary_fiveg_samples(ic);
				int fiveg_h, fiveg_y;

				if (ic->fontctx.mode == FONT_TTF) {
					int content_top, content_bottom, available, top_clear, below;
					int bottom_clear;

					if (ic->showheader) {
						content_top = ic->fontctx.header_h;
					} else {
						content_top = ic->showedge * imageuipx(ic, 1);
					}
					bottom_clear = summary_hs_footer_clearance(ic);
					if (monthrotatenotevisible) {
						bottom_clear += ic->lineheight;
					}
					/* end the label band above the footer (not at the image edge) */
					content_bottom = height - bottom_clear;
					available = content_bottom - content_top;

					/* same top clearance as standalone -5g: pad + half axis glyph */
					top_clear = imageuipx(ic, 8) + ic->fontctx.axis_ch / 2;
					below = imageuipx(ic, 8) + ic->fontctx.axis_ch;
					fiveg_h = available - top_clear - below;
					if (fiveg_h < 1) {
						fiveg_h = 1;
					}
					/* tip below header; labels end at content_bottom */
					fiveg_y = content_top + top_clear + fiveg_h;
				} else {
					int fiveg_bottom = 30 + imageextrapx(ic, 8);

					/* built-in: preserve tip at 38 - headermod */
					fiveg_h = height - 68 + headermod - imageextrapx(ic, 8)
						- (monthrotatenotevisible * (ic->lineheight + 2));
					if (fiveg_h < 1) {
						fiveg_h = 1;
					}
					fiveg_y = height - fiveg_bottom - (monthrotatenotevisible * ic->lineheight);
				}
				drawfiveminutes(ic, fivegraph_x, fiveg_y, israte, fiveg_samples, fiveg_h, fiveg_barwidth_val);
			} else {
				drawhours(ic, graph_x, summary_hours_y_hs(ic, header_extra, graph_headermod, height, monthrotatenotevisible), israte);
			}
			if (monthrotatenotevisible) {
				imagestring(ic, FONT_ROLE_BODY, 13 - imageextrapx(ic, 4) + (ic->fontctx.cw * 2) + ic->showedge, height - imageuipx(ic, 12) - ic->showedge - ic->lineheight, monthrotatenote, ic->ctext);
			}
			break;
		// vertical
		case 2:
			if (cfg.summarygraph == 1) {
				int fiveg_samples = summary_fiveg_samples(ic);
				int fiveg_w = fiveg_samples * fiveg_barwidth_val + graph_extra_space(ic);
				int fiveg_x = graph_xpos_margin(ic);
				int fiveg_plot_h;

				if (ic->fontctx.mode == FONT_TTF) {
					/* same block centering as standalone: xpos = block_start + graph_xpos_margin */
					fiveg_x = (width - fiveg_w) / 2 + graph_xpos_margin(ic);
					if (fiveg_x < 0) {
						fiveg_x = 0;
					}
					/* width fill uses barwidth; keep the previous 1px-bar plot height */
					fiveg_plot_h = 132 + imageextrapx(ic, 35);
				} else {
					fiveg_plot_h = 132 * fiveg_barwidth_val + imageextrapx(ic, 35);
				}
				drawfiveminutes(ic, fiveg_x, height - vs_fiveg_bottom, israte, fiveg_samples,
					fiveg_plot_h, fiveg_barwidth_val);
			} else {
				int hours_x = hourly_graph_left(ic);
				int hours_y = summary_hours_y_vs(ic, header_extra, graph_headermod, monthrotatenotevisible, digest_month_y);

				if (ic->fontctx.mode == FONT_TTF) {
					int hours_w = hourly_graph_width(ic);

					/* same block centering as standalone: xpos = block_start + hourly_graph_left */
					hours_x = (width - hours_w) / 2 + hourly_graph_left(ic);
					if (hours_x < 0) {
						hours_x = 0;
					}
				}
				drawhours(ic, hours_x, hours_y, israte);
			}
			if (monthrotatenotevisible) {
				/* hours_y already includes +2*lineheight for this note; sit above the plot */
				int hours_y = summary_hours_y_vs(ic, header_extra, graph_headermod, monthrotatenotevisible, digest_month_y);
				int note_y = hours_y - (ic->lineheight * (3 + imageextrapx(ic, 2)));

				imagestring(ic, FONT_ROLE_BODY, 13 - imageextrapx(ic, 4) + (ic->fontctx.cw * 2) + ic->showedge, note_y, monthrotatenote, ic->ctext);
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
	/* shift values right by 2*cw; callers keep donut/title anchors on value_edge */
	const int padded_edge = value_edge + 2 * ic->fontctx.cw;
	char num[64], unit[16];

	imagestring(ic, FONT_ROLE_BODY, label_edge - imagetextwidth(ic, FONT_ROLE_BODY, "rx"), y_rx, "rx", ic->ctext);
	getvalueparts(rx, RT_Normal, num, sizeof(num), unit, sizeof(unit), NULL);
	imagestring_value_right(ic, FONT_ROLE_BODY, padded_edge, y_rx, num, unit, ic->ctext);

	imagestring(ic, FONT_ROLE_BODY, label_edge - imagetextwidth(ic, FONT_ROLE_BODY, "tx"), y_tx, "tx", ic->ctext);
	getvalueparts(tx, RT_Normal, num, sizeof(num), unit, sizeof(unit), NULL);
	imagestring_value_right(ic, FONT_ROLE_BODY, padded_edge, y_tx, num, unit, ic->ctext);

	imagestring(ic, FONT_ROLE_BODY, label_edge - imagetextwidth(ic, FONT_ROLE_BODY, "="), y_eq, "=", ic->ctext);
	getvalueparts(rx + tx, RT_Normal, num, sizeof(num), unit, sizeof(unit), NULL);
	imagestring_value_right(ic, FONT_ROLE_BODY, padded_edge, y_eq, num, unit, ic->ctext);
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
		imagestring(ic, FONT_ROLE_BODY, since_x, y + (7 * ic->lineheight), daytemp, ic->ctext);
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
	/* built-in keeps historical 74px; TTF scales with cell width (12*6+2 == 74) */
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

