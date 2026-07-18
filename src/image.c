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
	int i, tmax = 0, s = 0, step, prev = 0, diff = 0, chour;
	int x = xpos, y = ypos, extrax = 0, extray = 0, xt = 0;
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

	x += imageextrapx(ic, 14);
	extrax = imageextrapx(ic, 145);
	extray = imageextrapx(ic, 35);

	/* scale values */
	scaleunit = getscale(max, israte);

	s = (int)lrint(((double)scaleunit / (double)max) * (124 + extray));
	if (s < SCALEMINPIXELS) {
		step = 2;
	} else {
		step = 1;
	}

	xt = x + 36;

	for (i = step; i * s <= (124 + extray + 4); i = i + step) {
		const char *val;
		int label_y;

		gdImageDashedLine(ic->im, xt, y + 124 - (i * s), xt + 424 + extrax, y + 124 - (i * s), ic->cline);
		gdImageDashedLine(ic->im, xt, y + 124 - prev - (step * s) / 2, xt + 424 + extrax, y + 124 - prev - (step * s) / 2, ic->clinel);
		val = getimagevalue(scaleunit * (unsigned int)i, 3, israte);
		label_y = y + 121 - (i * s) - imageextrapx(ic, 3);
		if (ic->fontctx.mode == FONT_TTF) {
			int label_gap = 4;

			while (*val == ' ') {
				val++;
			}
			/* Right-align to the fixed Y-axis; do not move xt with font size */
			imagestring(ic, FONT_ROLE_AXIS, xt - label_gap - imagetextwidth(ic, FONT_ROLE_AXIS, val), label_y, val, ic->ctext);
		} else {
			imagestring(ic, FONT_ROLE_AXIS, x + 16 - imageextrapx(ic, 3), label_y, val, ic->ctext);
		}
		prev = i * s;
	}
	if ((prev + (step * s) / 2) <= (124 + extray + 4)) {
		gdImageDashedLine(ic->im, xt, y + 124 - prev - (step * s) / 2, xt + 424 + extrax, y + 124 - prev - (step * s) / 2, ic->clinel);
	}

	/* scale text */
	imagestringup(ic, FONT_ROLE_AXIS, x - 2 - imageextrapx(ic, 14), y + 58 + (israte * 10) - (extray / 2), getimagescale(scaleunit * (unsigned int)step, israte), ic->ctext);

	/* axis */
	gdImageLine(ic->im, xt - 4, y + 124, xt + 430 + extrax, y + 124, ic->ctext);
	gdImageLine(ic->im, xt, y - 10 - extray, xt, y + 124 + 4, ic->ctext);

	/* arrows */
	drawarrowup(ic, xt, y - 9 - extray);
	drawarrowright(ic, xt + 429 + extrax, y + 124);

	xt = x + 440 + extrax;

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
		snprintf(buffer, 32, "%02d ", s);
		if (hourdata[s].date == 0) {
			chour = ic->cline;
		} else {
			chour = ic->ctext;
		}
		if (ic->fontctx.mode == FONT_TTF) {
			imagestring(ic, FONT_ROLE_AXIS, xt - imagefontwidth(ic, FONT_ROLE_AXIS) / 2, y + 132, buffer, chour);
		} else {
			imagestring(ic, FONT_ROLE_AXIS, xt, y + 128, buffer, chour);
		}
		drawpoles(ic, xt - 2, y - extray, 124 + extray, hourdata[s].rx, hourdata[s].tx, max);
		gdImageLine(ic->im, xt - 4 - imageextrapx(ic, 3), y + 124, xt + 12 + imageextrapx(ic, 3), y + 124, chour);
		if (s == 0 && i != 23) {
			/* midnight line */
			gdImageLine(ic->im, xt - 5 - imageextrapx(ic, 3), y - 5 - extray, xt - 5 - imageextrapx(ic, 3), y + 124 - 1, ic->clinel);
			xt--;
		}
		xt = xt - (17 + imageextrapx(ic, 6));
	}

	return 1;
}

void drawhourly(IMAGECONTENT *ic, const int israte)
{
	int width, height, headermod = 0;

	width = 500 + imageextrapx(ic, 168);
	height = 200 + imageextrapx(ic, 48);

	if (!ic->showheader) {
		headermod = ic->fontctx.header_h + 2;
		height -= ic->fontctx.header_h - 2;
	}

	imageinit(ic, width, height);
	layoutinit(ic, " / hourly", width, height);

	if (drawhours(ic, 12, 46 - headermod + imageextrapx(ic, 40), israte)) {
		drawlegend(ic, width / 2 - imageextrapx(ic, 10), 183 - headermod + imageextrapx(ic, 46), 0);
	}
}

typedef struct {
	int textx, offsetx;
	int d24, d37, d50; /* vertical divider x */
	int hline_right_rate; /* 65*cw+offsetx+2 */
	int hline_right_norate; /* 50*cw+offsetx-4 */
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
	cols->hline_right_norate = textx + (50 * cw) + offsetx - 4;

	cols->rx_edge = cols->tx_edge = cols->total_edge = cols->rate_edge = 0;
	cols->rx_dec = cols->tx_dec = cols->total_dec = 0;
	cols->date_field_right = cols->header_field_right = 0;

	if (ic->fontctx.mode == FONT_TTF) {
		const int colpad = 8;
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
		return texty + ic->fontctx.ch + (ic->lineheight + 8 - ic->fontctx.ch) / 2;
	}
	return texty + ic->lineheight + 4;
}

static int list_mid_rule_y(const IMAGECONTENT *ic, const int texty)
{
	if (ic->fontctx.mode == FONT_TTF) {
		return texty - ic->lineheight + ic->fontctx.ch + (ic->lineheight + 8 - ic->fontctx.ch) / 2;
	}
	return texty + 5 - imageextrapx(ic, 2);
}

static void list_draw_hline(IMAGECONTENT *ic, const ListColumns *cols, const int y, const int withrate)
{
	int x2 = withrate ? cols->hline_right_rate : cols->hline_right_norate;
	gdImageLine(ic->im, cols->textx + 2, y, x2, y, ic->cline);
}

static void list_draw_vdividers(IMAGECONTENT *ic, const ListColumns *cols, const int y1, const int y2, const int withrate)
{
	gdImageLine(ic->im, cols->d24, y1, cols->d24, y2, ic->cline);
	gdImageLine(ic->im, cols->d37, y1, cols->d37, y2, ic->cline);
	if (withrate) {
		gdImageLine(ic->im, cols->d50, y1, cols->d50, y2, ic->cline);
	}
}

void drawlist(IMAGECONTENT *ic, const char *listname)
{
	ListType listtype = LT_None;
	ListColumns cols;
	int textx, texty, offsetx = 0;
	int width, height, headermod, i = 1, liney, rowcount = 0;
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

	width = 83 * ic->fontctx.cw + 2 + imageextrapx(ic, 2);
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
		headermod = ic->fontctx.header_h + 2;
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
	texty += ic->lineheight + 8;

	if (datainfo.count) {
		list_draw_vdividers(ic, &cols, texty - 6 - ic->lineheight,
			texty + ((ic->lineheight + cfg.linespaceadjust) * rowcount) - cfg.linespaceadjust + 5 - imageextrapx(ic, 2),
			cfg.ostyle > 2);
	} else {
		list_draw_vdividers(ic, &cols, texty - 6 - ic->lineheight, texty - 4, cfg.ostyle > 2);
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
					if (cfg.ostyle > 2) {
						gdImageFilledRectangle(ic->im, textx + 2, texty + 2, textx + (65 * ic->fontctx.cw) + offsetx + 2, texty + ic->fontctx.ch - 2, ic->cbgoffset);
					} else {
						gdImageFilledRectangle(ic->im, textx + 2, texty + 2, textx + (50 * ic->fontctx.cw) + offsetx - 4, texty + ic->fontctx.ch - 2, ic->cbgoffset);
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
					if (cfg.ostyle > 2) {
						gdImageFilledRectangle(ic->im, textx + 2, texty + 2, textx + (65 * ic->fontctx.cw) + offsetx + 2, texty + ic->fontctx.ch - 2, ic->cbgoffset);
					} else {
						gdImageFilledRectangle(ic->im, textx + 2, texty + 2, textx + (50 * ic->fontctx.cw) + offsetx - 4, texty + ic->fontctx.ch - 2, ic->cbgoffset);
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

	list_draw_hline(ic, &cols, list_mid_rule_y(ic, texty), cfg.ostyle > 2);

	buffer[0] = '\0';

	if (estimatevisible) {
		int bar_y = list_bar_y(ic, texty - ic->lineheight);

		if (cfg.estimatestyle) {
			if (cfg.ostyle > 2) {
				drawbar(ic, textx + (67 * ic->fontctx.cw) - 2, bar_y, 13 * ic->fontctx.cw + 1, e_rx, e_tx, datainfo.max, 1);
				drawbar(ic, textx + (67 * ic->fontctx.cw) - 2, bar_y, 13 * ic->fontctx.cw + 1, datalist_i->rx, datalist_i->tx, datainfo.max, 0);
			} else {
				drawbar(ic, textx + (51 * ic->fontctx.cw) - 2, bar_y, 28 * ic->fontctx.cw + 3, e_rx, e_tx, datainfo.max, 1);
				drawbar(ic, textx + (51 * ic->fontctx.cw) - 2, bar_y, 28 * ic->fontctx.cw + 3, datalist_i->rx, datalist_i->tx, datainfo.max, 0);
			}
		}

		texty += 8;
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

		list_draw_vdividers(ic, &cols, texty - 6, texty + ic->lineheight - imageextrapx(ic, 2), cfg.ostyle > 2);
	} else if (strlen(ic->dataend) > 0 && datainfo.count > 1 && listtype != LT_Top) {
		texty += 8;
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

		list_draw_vdividers(ic, &cols, texty - 6, texty + ic->lineheight - imageextrapx(ic, 2), cfg.ostyle > 2);
	}

	if (monthrotatenotevisible) {
		texty += ic->lineheight * 2;
		imagestring(ic, FONT_ROLE_BODY, textx + ic->fontctx.cw, texty, monthrotatenote, ic->ctext);
	}

	dbdatalistfree(&datalist);
}

void drawsummary(IMAGECONTENT *ic, const int layout, const int israte)
{
	int width, height, headermod, header_extra, digest_x, alltime_x, legend_x, legend_y, graph_x, fivegraph_x;
	int digest_day_y, digest_month_y, alltime_y;
	int monthrotatenotevisible = 0;
	int vs_fiveg_bottom;
	char monthrotatenote[96];

	monthrotatenotevisible = ismonthrotatenoteneeded();
	if (monthrotatenotevisible) {
		getmonthrotatenote(monthrotatenote, sizeof(monthrotatenote));
	}

	vs_fiveg_bottom = 31 + imageextrapx(ic, 6);

	switch (layout) {
		// horizontal
		case 1:
			width = 163 * ic->fontctx.cw + 2 + imageextrapx(ic, 2);
			height = 56 + 12 * ic->lineheight;
			break;
		// vertical
		case 2:
			width = 83 * ic->fontctx.cw + 2 + imageextrapx(ic, 2);
			height = 370 + imageextrapx(ic, 90);
			break;
		// no hours
		default:
			width = 83 * ic->fontctx.cw + 2 + imageextrapx(ic, 2);
			height = 56 + 12 * ic->lineheight;
			break;
	}

	if (monthrotatenotevisible) {
		height += ic->lineheight * 2;
	}

	if (!ic->showheader) {
		headermod = ic->fontctx.header_h + 2;
		header_extra = 0;
		height -= ic->fontctx.header_h - 2;
	} else {
		headermod = 0;
		header_extra = ic->fontctx.header_h - 24;
		height += header_extra;
	}

	if (ic->fontctx.mode == FONT_TTF) {
		if (layout == 2) {
			if (cfg.summarygraph == 1) {
				int bottom_margin;

				/* Axis labels sit below the 5-min plot; grow margin + canvas together */
				bottom_margin = ic->fontctx.axis_ch + 4 + 12 + ic->showedge;
				if (bottom_margin > vs_fiveg_bottom) {
					height += bottom_margin - vs_fiveg_bottom;
					vs_fiveg_bottom = bottom_margin;
				}
			} else {
				int graph_y, needed;

				graph_y = 215 + header_extra + imageextrapx(ic, 84) - headermod
					+ (monthrotatenotevisible * (ic->lineheight * 2));
				/* labels at graph_y+128; Tiny footer at height-12-showedge */
				needed = graph_y + 128 + ic->fontctx.axis_ch + 4 + 12 + ic->showedge;
				if (height < needed) {
					height = needed;
				}
			}
		} else {
			/* Extra bottom pad so rate/legend clear the footer */
			height += 2 * ic->lineheight;
		}
	}

	imageinit(ic, width, height);
	layoutinit(ic, "", width, height);

	if (ic->fontctx.mode == FONT_TTF) {
		alltime_x = 64 * ic->fontctx.cw;
		legend_x = 67 * ic->fontctx.cw;
		graph_x = 83 * ic->fontctx.cw;
		fivegraph_x = graph_x;
		/* body at textx - (12*cw+2) stays near the builtin left margin (~26). */
		digest_x = (12 * ic->fontctx.cw + 2) + 26;
		/* Clear tall header title; builtin keeps y=30. */
		digest_day_y = ic->fontctx.header_h + 10 - headermod;
		digest_month_y = digest_day_y - 1 + 8 * ic->lineheight;
		alltime_y = digest_day_y + 27 + imageextrapx(ic, 10);
		/* Under all-time "since" line, matching builtin legend vs since gap */
		legend_y = alltime_y + 9 * ic->lineheight;
	} else {
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
				imagestring(ic, FONT_ROLE_BODY, 13 - imageextrapx(ic, 4) + (ic->fontctx.cw * 2) + ic->showedge, height - 12 - ic->showedge - ic->lineheight, monthrotatenote, ic->ctext);
			}
			break;
		// vertical
		case 2:
			if (cfg.summarygraph == 1) {
				drawfiveminutes(ic, 8 + imageextrapx(ic, 14), height - vs_fiveg_bottom, israte, 422 + imageextrapx(ic, 154), 132 + imageextrapx(ic, 35));
			} else {
				drawhours(ic, 12, 215 + header_extra + imageextrapx(ic, 84) - headermod + (monthrotatenotevisible * (ic->lineheight * 2)), israte);
			}
			if (monthrotatenotevisible) {
				imagestring(ic, FONT_ROLE_BODY, 13 - imageextrapx(ic, 4) + (ic->fontctx.cw * 2) + ic->showedge, 215 + header_extra + imageextrapx(ic, 84) - headermod - (ic->lineheight * (1 + imageextrapx(ic, 2))), monthrotatenote, ic->ctext);
			}
			break;
		default:
			if (monthrotatenotevisible) {
				imagestring(ic, FONT_ROLE_BODY, 13 - imageextrapx(ic, 4) + (ic->fontctx.cw * 2) + ic->showedge, height - 12 - ic->showedge - ic->lineheight, monthrotatenote, ic->ctext);
			}
			break;
	}
}

/* Matches old left-aligned "rx " + getvalue(..., 12) width ("  999.99 YiB" pad). */
static const char summary_stack_sample[] = "rx   999.99 YiB";

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
			y + (4.5 * ic->lineheight),
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
	imagestring(ic, FONT_ROLE_BODY, x, y + (4.5 * ic->lineheight), buffer, ic->ctext);
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
	int imagewidth, imageheight = height, headermod = 0;

	imagewidth = resultcount + FIVEMINEXTRASPACE + imageextrapx(ic, 14);

	if (!ic->showheader) {
		headermod = ic->fontctx.header_h - 2;
	}

	imageinit(ic, imagewidth, imageheight);
	layoutinit(ic, " / 5 minute", imagewidth, imageheight);

	if (drawfiveminutes(ic, 8 + imageextrapx(ic, 14), imageheight - 30 - imageextrapx(ic, 8), israte, resultcount, imageheight - 68 + headermod - imageextrapx(ic, 8))) {
		drawlegend(ic, imagewidth / 2 - imageextrapx(ic, 10), imageheight - 17 - imageextrapx(ic, 2), 0);
	}
}

int drawfiveminutes(IMAGECONTENT *ic, const int xpos, const int ypos, const int israte, const int resultcount, const int height)
{
	int x = xpos, y = ypos, i = 0, t = 0, rxh = 0, txh = 0, step = 0, s = 0, prev = 0;
	uint64_t scaleunit, max;
	time_t timestamp;
	double ratediv, e;
	char buffer[32];
	const struct tm *d;
	dbdatalist *datalist = NULL, *datalist_i = NULL;
	dbdatalistinfo datainfo;

	if (!db_getdata(&datalist, &datainfo, ic->interface.name, "fiveminute", (uint32_t)resultcount) || datainfo.count == 0) {
		x = (resultcount + FIVEMINEXTRASPACE + imageextrapx(ic, 14)) / 2 - (13 * ic->fontctx.cw);
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
	x += 36;
	gdImageLine(ic->im, x, y, x + (resultcount + FIVEMINWIDTHFULLPADDING), y, ic->ctext);
	gdImageLine(ic->im, x + 4, y + 4, x + 4, y - height, ic->ctext);

	/* arrows */
	drawarrowup(ic, x + 4, y - 1 - height);
	drawarrowright(ic, x + 1 + (resultcount + FIVEMINWIDTHFULLPADDING), y);

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

	/* center line */
	x += 5;
	y -= txh + FIVEMINHEIGHTOFFSET;
	gdImageLine(ic->im, x, y, x + (resultcount + FIVEMINWIDTHPADDING), y, ic->ctext);
	imagestring(ic, FONT_ROLE_AXIS, x - 21 - imageextrapx(ic, 3), y - 4 - imageextrapx(ic, 3), "  0", ic->ctext);

	/* scale values */
	scaleunit = getscale(max, israte);

	s = (int)lrint(((double)scaleunit / (double)max) * t);
	if (s == 0) {
		s = 1; // force to show something when there's not much or any traffic, scale is likely to be wrong in this case
	}
	while (s * step < SCALEMINPIXELS) {
		step++;
	}

	if (debug) {
		printf("maxrx: %" PRIu64 "\n", datainfo.maxrx);
		printf("maxtx: %" PRIu64 "\n", datainfo.maxtx);
		printf("rxh: %d     txh: %d\n", rxh, txh);
		printf("max divided: %" PRIu64 "\n", max);
		printf("scaleunit:   %" PRIu64 "\nstep: %d\n", scaleunit, step);
		printf("pixels per step: %d\n", s);
		printf("mintime: %" PRIu64 "\nmaxtime: %" PRIu64 "\n", (uint64_t)datainfo.mintime, (uint64_t)datainfo.maxtime);
		printf("count: %u\n", datainfo.count);
	}

	/* upper part scale values */
	y--; // adjust to start above center line
	for (i = step; i * s <= rxh; i = i + step) {
		gdImageDashedLine(ic->im, x, y - (i * s), x + (resultcount + FIVEMINWIDTHPADDING), y - (i * s), ic->cline);
		gdImageDashedLine(ic->im, x, y - prev - (step * s) / 2, x + (resultcount + FIVEMINWIDTHPADDING), y - prev - (step * s) / 2, ic->clinel);
		imagestring(ic, FONT_ROLE_AXIS, x - 21 - imageextrapx(ic, 3), y - 3 - (i * s) - imageextrapx(ic, 3), getimagevalue(scaleunit * (unsigned int)i, 3, israte), ic->ctext);
		prev = i * s;
	}
	if ((prev + (step * s) / 2) <= rxh) {
		gdImageDashedLine(ic->im, x, y - prev - (step * s) / 2, x + (resultcount + FIVEMINWIDTHPADDING), y - prev - (step * s) / 2, ic->clinel);
	}

	y += 2; // adjust to start below center line
	prev = 0;

	/* lower part scale values */
	for (i = step; i * s <= txh; i = i + step) {
		gdImageDashedLine(ic->im, x, y + (i * s), x + (resultcount + FIVEMINWIDTHPADDING), y + (i * s), ic->cline);
		gdImageDashedLine(ic->im, x, y + prev + (step * s) / 2, x + (resultcount + FIVEMINWIDTHPADDING), y + prev + (step * s) / 2, ic->clinel);
		imagestring(ic, FONT_ROLE_AXIS, x - 21 - imageextrapx(ic, 3), y - 3 + (i * s) - imageextrapx(ic, 3), getimagevalue(scaleunit * (unsigned int)i, 3, israte), ic->ctext);
		prev = i * s;
	}
	if ((prev + (step * s) / 2) <= txh) {
		gdImageDashedLine(ic->im, x, y + prev + (step * s) / 2, x + (resultcount + FIVEMINWIDTHPADDING), y + prev + (step * s) / 2, ic->clinel);
	}

	y--; // y is now back on center line

	/* scale text */
	imagestringup(ic, FONT_ROLE_AXIS, x - 39 - imageextrapx(ic, 14), ypos - height / 2 + (israte * 10), getimagescale(scaleunit * (unsigned int)step, israte), ic->ctext);

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

		if (d->tm_min == 0 && i > 2) {
			if (d->tm_hour % 2 == 0) {
				if (d->tm_hour == 0) {
					gdImageLine(ic->im, x + i, y + txh - 1 + FIVEMINHEIGHTOFFSET, x + i, y - rxh - 1, ic->cline);
				} else {
					gdImageLine(ic->im, x + i, y + txh - 1 + FIVEMINHEIGHTOFFSET, x + i, y - rxh - 1, ic->cbgoffset);
				}

				if (i > imagefontwidth(ic, FONT_ROLE_AXIS)) {
					snprintf(buffer, 32, "%02d", d->tm_hour);
					if (datalist_i->timestamp > timestamp) {
						imagestring(ic, FONT_ROLE_AXIS, x + i - imagefontwidth(ic, FONT_ROLE_AXIS) + 1, y + txh + imagefontheight(ic, FONT_ROLE_AXIS) - imageextrapx(ic, 5), buffer, ic->cline);
					} else {
						imagestring(ic, FONT_ROLE_AXIS, x + i - imagefontwidth(ic, FONT_ROLE_AXIS) + 1, y + txh + imagefontheight(ic, FONT_ROLE_AXIS) - imageextrapx(ic, 5), buffer, ic->ctext);
					}
				}
			} else {
				gdImageLine(ic->im, x + i, y + txh - 1 + FIVEMINHEIGHTOFFSET, x + i, y - rxh - 1, ic->cbgoffset);
			}
			gdImageSetPixel(ic->im, x + i, y, ic->ctext);
		}

		if (datalist_i->timestamp > timestamp) {
			gdImageSetPixel(ic->im, x + i, y, ic->cline);
			gdImageSetPixel(ic->im, x + i, y + txh + FIVEMINHEIGHTOFFSET, ic->cline);
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
		drawpole(ic, x + i, y - 1, t, 1, ic->crx);

		t = (int)lrint(((double)datalist_i->tx / e / (double)datainfo.maxtx) * txh);
		if (t > txh) {
			t = txh;
		}
		drawpole(ic, x + i, y + 1, t, 2, ic->ctx);

		datalist_i = datalist_i->next;
	}

	dbdatalistfree(&datalist);

	return 1;
}

void draw95thpercentilegraph(IMAGECONTENT *ic, const int mode)
{
	int imagewidth, imageheight = 300, headermod = 0;

	/* width needed for all percentile entries + decoration depending on font size */
	imagewidth = PERCENTILEENTRYCOUNT + 78 + imageextrapx(ic, 14);

	if (!ic->showheader) {
		headermod = ic->fontctx.header_h - 2;
	}

	imageinit(ic, imagewidth, imageheight);
	layoutinit(ic, " / 95th percentile", imagewidth, imageheight);

	drawpercentile(ic, mode, 8 + imageextrapx(ic, 14), imageheight - 30 - imageextrapx(ic, 8), imageheight - 68 + headermod - imageextrapx(ic, 8));
}

void drawpercentile(IMAGECONTENT *ic, const int mode, const int xpos, const int ypos, const int height)
{
	int i, l, x = xpos, y = ypos, s = 0, step = 0, prev = 0, last = 0, color;
	uint64_t scaleunit, max, percentile;
	double ratediv, percentileratediv;
	const struct tm *d;
	time_t current;
	char datebuff[DATEBUFFLEN];
	dbdatalist *datalist = NULL, *datalist_i = NULL;
	dbdatalistinfo datainfo;
	percentiledata pdata;

	if (cfg.fiveminutehours < PERCENTILEENTRYCOUNT) {
		fprintf(stderr, "\nWarning: Configuration \"5MinuteHours\" needs to be at least %d for 100%% coverage.\n", PERCENTILEENTRYCOUNT);
		fprintf(stderr, "         \"5MinuteHours\" is currently set at %d.\n\n", cfg.fiveminutehours);
	}

	if (!getpercentiledata(&pdata, ic->interface.name, 0)) {
		imagestring(ic, FONT_ROLE_BODY, x + 320 - imageextrapx(ic, 30), y - 120, "failed to get percentile data", ic->ctext);
		return;
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
		imagestring(ic, FONT_ROLE_BODY, x + 320 - imageextrapx(ic, 30), y - 120, "no percentile data available", ic->ctext);
		return;
	}

	if (debug) {
		printf("mode:  %d - %d\n", mode, cfg.qmode);
		printf("count: %" PRIu32 "\n", datainfo.count);
	}

	if (mode == 0) {
		color = ic->crx;
		percentile = pdata.rxpercentile;
		max = (uint64_t)((double)datainfo.maxrx / ratediv);
	} else if (mode == 1) {
		color = ic->ctx;
		percentile = pdata.txpercentile;
		max = (uint64_t)((double)datainfo.maxtx / ratediv);
	} else {
		color = ic->ctotal;
		percentile = pdata.sumpercentile;
		max = (uint64_t)((double)datainfo.max / ratediv);
	}

	if ((uint64_t)((double)(percentile) / percentileratediv) > max) {
		max = (uint64_t)((double)(percentile) / percentileratediv);
	}

	/* scale values */
	scaleunit = getscale(max, 1);

	s = (int)lrint(((double)scaleunit / (double)max) * height);
	if (s < SCALEMINPIXELS) {
		step = 2;
	} else {
		step = 1;
	}

	/* scale text */
	imagestringup(ic, FONT_ROLE_AXIS, x - 2 - imageextrapx(ic, 14), y - (height / 2), getimagescale(scaleunit * (unsigned int)step, 1), ic->ctext);

	/* axis */
	x += 36;
	gdImageLine(ic->im, x, y, x + (PERCENTILEENTRYCOUNT + PERCENTILEMINWIDTHFULLPADDING), y, ic->ctext);
	gdImageLine(ic->im, x + 4, y + 4, x + 4, y - height, ic->ctext);

	/* arrows */
	drawarrowup(ic, x + 4, y - 4 - height);
	drawarrowright(ic, x + 1 + (PERCENTILEENTRYCOUNT + PERCENTILEMINWIDTHFULLPADDING), y);

	/* adjust cursor to first point on graph */
	x += 5;
	y -= 1;

	for (i = step; i * s <= height; i = i + step) {
		gdImageDashedLine(ic->im, x, y - (i * s), x + (PERCENTILEENTRYCOUNT + PERCENTILEMINWIDTHFULLPADDING) - 5, y - (i * s), ic->cline);
		gdImageDashedLine(ic->im, x, y - prev - (step * s) / 2, x + (PERCENTILEENTRYCOUNT + PERCENTILEMINWIDTHFULLPADDING) - 5, y - prev - (step * s) / 2, ic->clinel);
		imagestring(ic, FONT_ROLE_AXIS, x - 22 - imageextrapx(ic, 3), y - 4 - (i * s) - imageextrapx(ic, 3), getimagevalue(scaleunit * (unsigned int)i, 3, 1), ic->ctext);
		prev = i * s;
	}
	if ((prev + (step * s) / 2) <= height) {
		gdImageDashedLine(ic->im, x, y - prev - (step * s) / 2, x + (PERCENTILEENTRYCOUNT + PERCENTILEMINWIDTHFULLPADDING) - 5, y - prev - (step * s) / 2, ic->clinel);
	}

	datalist_i = datalist;
	current = pdata.monthbegin;
	prev = -24;

	/* draw data */
	for (i = 0; i < PERCENTILEENTRYCOUNT; i++, current += 3600) {
		if (datalist_i == NULL || current < datalist_i->timestamp) {
			gdImageSetPixel(ic->im, x + i, y + 1, ic->cbgoffset);
			if (i >= prev + 24 && i % 24 == 0 && current < pdata.dataend) {
				d = localtime(&current);
				strftime(datebuff, DATEBUFFLEN, "%d", d);
				if (i > 0) {
					drawpole(ic, x + i, y + 4, height, 1, ic->cbgoffset);
				}
				imagestring(ic, FONT_ROLE_AXIS, x + 12 + i - 4 - imageextrapx(ic, 1), y + 5, datebuff, ic->cline);
				prev = i;
			}
			continue;
		}

		if (i >= prev + 24 && i % 24 == 0) {
			d = localtime(&current);
			strftime(datebuff, DATEBUFFLEN, "%d", d);
			drawpole(ic, x + i, y, height, 1, ic->cbgoffset);
			if (i > 0) {
				gdImageLine(ic->im, x + i, y + 1, x + i, y + 4, ic->ctext);
			}
			imagestring(ic, FONT_ROLE_AXIS, x + 12 + i - 4 - imageextrapx(ic, 1), y + 5, datebuff, ic->ctext);
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
		drawpole(ic, x + i, y, l, 1, color);

		last = i;
		datalist_i = datalist_i->next;
	}

	dbdatalistfree(&datalist);

	/* 95th percentile line */
	l = (int)lrint(((double)(percentile) / percentileratediv / (double)max) * height);
	if (l > height) {
		l = height;
	} else if (l == 0) {
		l = 1;
	}
	gdImageLine(ic->im, x, y - l, x + last, y - l, ic->cpercentileline);

	if (debug) {
		printf("s:   %d\n", s);
		printf("l:   %d\n", l);
		printf("h:   %d\n", height);
		printf("p:   %" PRIu64 "\n", (uint64_t)((double)percentile / percentileratediv));
		printf("max: %" PRIu64 "\n", max);
		printf("max rate: %s\n", gettrafficrate((uint64_t)((double)max * ratediv), 3600, 0));
		printf("per rate: %s\n", gettrafficrate(percentile, 300, 0));
	}

	/* finally add legend with percentile text */
	drawpercentilelegend(ic, x + 300 - imageextrapx(ic, 50), y + 14 + imageextrapx(ic, 6), mode, percentile);
}
