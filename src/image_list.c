#include "common.h"
#include "dbsql.h"
#include "misc.h"
#include "image_list.h"
#include "image_font.h"
#include "image_widget.h"
#include "image_support.h"

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
	cols->rank_center = 0;

	if (ic->fontctx.mode == FONT_TTF) {
		const int colpad = imageuipx(ic, 8);
		const char *sample = "00.00 GiB";
		int sample_w, prefix_w;

		cols->rx_edge = cols->d24 - colpad;
		cols->tx_edge = cols->d37 - colpad;
		cols->total_edge = cols->d50 - colpad;
		cols->rate_edge = textx + (65 * cw) + offsetx - colpad;
		cols->date_field_right = textx + 10 * cw + offsetx;

		/* top list: center-align #; right-align day with a clear gap after #,
		 * date field ends at 16*cw so 10-char dates clear the rank column */
		if (offsetx > 0) {
			cols->rank_center = textx + 3 * cw;
			cols->date_field_right = textx + 11 * cw + offsetx;
		}

		/* one measured digit left of values, mirrors built-in alignment */
		cols->header_field_right = cols->date_field_right - imagetextwidth(ic, FONT_ROLE_BODY, "0");

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

static int list_design_bar_len(const IMAGECONTENT *ic, const ListType listtype)
{
	const int cw = ic->fontctx.cw;

	if (listtype == LT_Top) {
		if (cfg.ostyle > 2) {
			return 9 * cw - 1;
		}
		return 23 * cw + 3;
	}
	if (cfg.ostyle > 2) {
		return 13 * cw + 1;
	}
	return 28 * cw + 3;
}

void drawlist(IMAGECONTENT *ic, const char *listname)
{
	ListType listtype = LT_None;
	ListColumns cols;
	int textx, texty, offsetx = 0;
	int width, height, headermod, i = 1, liney, mid_y, v_top, rowcount = 0;
	int estimateavailable = 0, estimatevisible = 0, monthrotatenotevisible = 0;
	int natural_width, bar_extra = 0;
	int32_t limit;
	uint64_t e_rx = 0, e_tx = 0, e_secs;
	char buffer[512], datebuff[16], daybuff[16], monthrotatenote[96];
	char stampformat[64], titlename[16], colname[8];
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

	natural_width = image_list_width(ic);
	width = natural_width;
	if (ic->commonwidth > 0) {
		width = ic->commonwidth;
		bar_extra = image_list_bar_extra(ic, natural_width, list_design_bar_len(ic, listtype));
	}
	height = 62 + (ic->fontctx.header_h - 24) + 3 * ic->lineheight;

	// less space needed when no estimate or sum is shown
	if (!estimatevisible && !(strlen(ic->dataend) > 0 && datainfo.count > 1 && listtype != LT_Top)) {
		height = 62 + (ic->fontctx.header_h - 24) + 2 * ic->lineheight;
	}

	// exception for 5min and Hour when having sum shown
	if ((listtype == LT_5min || listtype == LT_Hour) && datainfo.count > 1 && strlen(ic->dataend) > 0) {
		height = 62 + (ic->fontctx.header_h - 24) + 3 * ic->lineheight;
	}

	if (monthrotatenotevisible) {
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
	if (bar_extra != 0) {
		cols.hline_right_rate += bar_extra;
		cols.hline_right_norate += bar_extra;
	}

	/* column headers */
	if (ic->fontctx.mode == FONT_TTF) {
		if (listtype == LT_Top) {
			imagestring(ic, FONT_ROLE_BODY, cols.rank_center - imagetextwidth(ic, FONT_ROLE_BODY, "#") / 2, texty, "#", ic->ctext);
			imagestring(ic, FONT_ROLE_BODY, cols.header_field_right - imagetextwidth(ic, FONT_ROLE_BODY, "day"), texty, "day", ic->ctext);
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
	/* top of vdividers: into the column-header row (day/rx/tx/total), above the rule */
	v_top = texty - imageuipx(ic, 6) - ic->lineheight;

	/* end vdividers on the mid rule so they meet that hline, avoid imageextrapx()
	 * for the end Y: for TTF it grows with cw and leaves a gap */
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
				/* built-in: " %s" left at textx -> 10-char dates end one cell past values,
				 * TTF: right-align to the same edge (date_field_right + one digit) */
				if (ic->fontctx.mode == FONT_TTF) {
					imagestring(ic, FONT_ROLE_BODY,
						cols.date_field_right + imagetextwidth(ic, FONT_ROLE_BODY, "0") - imagetextwidth(ic, FONT_ROLE_BODY, datebuff),
						texty, datebuff, ic->ctext);
				} else {
					snprintf(buffer, 32, " %s", datebuff);
					imagestring(ic, FONT_ROLE_BODY, textx, texty, buffer, ic->ctext);
				}
				texty += ic->lineheight + cfg.linespaceadjust;
				strcpy(daybuff, datebuff);
			}
		}

		if (ic->fontctx.mode == FONT_TTF) {
			if (listtype == LT_Top) {
				char rankbuf[16];

				strftime(datebuff, 16, stampformat, d);

				if (strcmp(datebuff, daybuff) == 0) {
					int pad2 = imageuipx(ic, 2);

					if (cfg.ostyle > 2) {
						gdImageFilledRectangle(ic->im, textx + pad2, texty + pad2, textx + (65 * ic->fontctx.cw) + offsetx + pad2, texty + ic->fontctx.ch - pad2, ic->cbgoffset);
					} else {
						gdImageFilledRectangle(ic->im, textx + pad2, texty + pad2, textx + (50 * ic->fontctx.cw) + offsetx - imageuipx(ic, 4), texty + ic->fontctx.ch - pad2, ic->cbgoffset);
					}
				}
				snprintf(rankbuf, 16, "%d", i);
				imagestring(ic, FONT_ROLE_BODY, cols.rank_center - imagetextwidth(ic, FONT_ROLE_BODY, rankbuf) / 2, texty, rankbuf, ic->ctext);
				imagestring(ic, FONT_ROLE_BODY, cols.date_field_right - imagetextwidth(ic, FONT_ROLE_BODY, datebuff), texty, datebuff, ic->ctext);
			} else {
				strftime(datebuff, 16, stampformat, d);
				imagestring(ic, FONT_ROLE_BODY, cols.date_field_right - imagetextwidth(ic, FONT_ROLE_BODY, datebuff), texty, datebuff, ic->ctext);
			}

			char num[64], unit[16];

			getvalueparts(datalist_i->rx, RT_Normal, num, sizeof(num), unit, sizeof(unit), NULL);
			imagestring_value_right(ic, FONT_ROLE_BODY, cols.rx_edge, texty, num, unit, ic->ctext);
			getvalueparts(datalist_i->tx, RT_Normal, num, sizeof(num), unit, sizeof(unit), NULL);
			imagestring_value_right(ic, FONT_ROLE_BODY, cols.tx_edge, texty, num, unit, ic->ctext);
			getvalueparts(datalist_i->rx + datalist_i->tx, RT_Normal, num, sizeof(num), unit, sizeof(unit), NULL);
			imagestring_value_right(ic, FONT_ROLE_BODY, cols.total_edge, texty, num, unit, ic->ctext);
			if (cfg.ostyle > 2) {
				if (datalist_i->next == NULL && issametimeslot(listtype, datalist_i->timestamp, ic->interface.updated)) {
					e_secs = getperiodseconds(listtype, datalist_i->timestamp, ic->interface.updated, ic->interface.created, 1);
				} else {
					e_secs = getperiodseconds(listtype, datalist_i->timestamp, ic->interface.updated, ic->interface.created, 0);
				}
				gettrafficrateparts(datalist_i->rx + datalist_i->tx, (time_t)e_secs, num, sizeof(num), unit, sizeof(unit));
				imagestring_value_right(ic, FONT_ROLE_BODY, cols.rate_edge, texty, num, unit, ic->ctext);
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
				drawbar(ic, textx + (71 * ic->fontctx.cw) + 2, bar_y, 9 * ic->fontctx.cw - 1 + bar_extra, datalist_i->rx, datalist_i->tx, datainfo.max, 0);
			} else {
				drawbar(ic, textx + (56 * ic->fontctx.cw), bar_y, 23 * ic->fontctx.cw + 3 + bar_extra, datalist_i->rx, datalist_i->tx, datainfo.max, 0);
			}
		} else {
			if (cfg.ostyle > 2) {
				if (datalist_i->next == NULL && estimateavailable && cfg.barshowsrate) {
					drawbar(ic, textx + (67 * ic->fontctx.cw) - 2, bar_y, 13 * ic->fontctx.cw + 1 + bar_extra, e_rx, e_tx, datainfo.max, 0);
				} else {
					drawbar(ic, textx + (67 * ic->fontctx.cw) - 2, bar_y, 13 * ic->fontctx.cw + 1 + bar_extra, datalist_i->rx, datalist_i->tx, datainfo.max, 0);
				}
			} else {
				drawbar(ic, textx + (51 * ic->fontctx.cw) - 2, bar_y, 28 * ic->fontctx.cw + 3 + bar_extra, datalist_i->rx, datalist_i->tx, datainfo.max, 0);
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
				drawbar(ic, textx + (67 * ic->fontctx.cw) - 2, bar_y, 13 * ic->fontctx.cw + 1 + bar_extra, e_rx, e_tx, datainfo.max, 1);
				drawbar(ic, textx + (67 * ic->fontctx.cw) - 2, bar_y, 13 * ic->fontctx.cw + 1 + bar_extra, datalist_i->rx, datalist_i->tx, datainfo.max, 0);
			} else {
				drawbar(ic, textx + (51 * ic->fontctx.cw) - 2, bar_y, 28 * ic->fontctx.cw + 3 + bar_extra, e_rx, e_tx, datainfo.max, 1);
				drawbar(ic, textx + (51 * ic->fontctx.cw) - 2, bar_y, 28 * ic->fontctx.cw + 3 + bar_extra, datalist_i->rx, datalist_i->tx, datainfo.max, 0);
			}
		}

		texty += imageuipx(ic, 8);
		if (ic->fontctx.mode == FONT_TTF) {
			char num[64], unit[16];

			getvalueparts(e_rx, RT_Estimate, num, sizeof(num), unit, sizeof(unit), NULL);
			imagestring(ic, FONT_ROLE_BODY, cols.date_field_right - imagetextwidth(ic, FONT_ROLE_BODY, cfg.estimatetext), texty, cfg.estimatetext, ic->ctext);
			imagestring_value_right(ic, FONT_ROLE_BODY, cols.rx_edge, texty, num, unit, ic->ctext);
			getvalueparts(e_tx, RT_Estimate, num, sizeof(num), unit, sizeof(unit), NULL);
			imagestring_value_right(ic, FONT_ROLE_BODY, cols.tx_edge, texty, num, unit, ic->ctext);
			getvalueparts(e_rx + e_tx, RT_Estimate, num, sizeof(num), unit, sizeof(unit), NULL);
			imagestring_value_right(ic, FONT_ROLE_BODY, cols.total_edge, texty, num, unit, ic->ctext);
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
			char sumlabel[16], num[64], unit[16];

			if (datainfo.count < 100) {
				snprintf(sumlabel, 16, "sum of %" PRIu32 "", datainfo.count);
			} else {
				snprintf(sumlabel, 16, "sum");
			}
			imagestring(ic, FONT_ROLE_BODY, cols.date_field_right - imagetextwidth(ic, FONT_ROLE_BODY, sumlabel), texty, sumlabel, ic->ctext);
			getvalueparts(datainfo.sumrx, RT_Normal, num, sizeof(num), unit, sizeof(unit), NULL);
			imagestring_value_right(ic, FONT_ROLE_BODY, cols.rx_edge, texty, num, unit, ic->ctext);
			getvalueparts(datainfo.sumtx, RT_Normal, num, sizeof(num), unit, sizeof(unit), NULL);
			imagestring_value_right(ic, FONT_ROLE_BODY, cols.tx_edge, texty, num, unit, ic->ctext);
			getvalueparts(datainfo.sumrx + datainfo.sumtx, RT_Normal, num, sizeof(num), unit, sizeof(unit), NULL);
			imagestring_value_right(ic, FONT_ROLE_BODY, cols.total_edge, texty, num, unit, ic->ctext);
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

