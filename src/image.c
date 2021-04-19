#include "common.h"
#include "dbsql.h"
#include "misc.h"
#include "image.h"
#include "image_support.h"
#include "vnstati.h"

void initimagecontent(IMAGECONTENT *ic)
{
	ic->im = NULL;
	ic->font = gdFontGetSmall();
	ic->lineheight = 12;
	ic->large = 0;
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

int drawhours(IMAGECONTENT *ic, const int xpos, const int ypos, const int rate)
{
	int i, tmax = 0, s = 0, step, prev = 0, diff = 0, chour;
	int x = xpos, y = ypos, extrax = 0, extray = 0;
	double ratediv;
	uint64_t max = 1, scaleunit = 0;
	char buffer[32];
	struct tm *d;
	dbdatalist *datalist = NULL, *datalist_i = NULL;
	dbdatalistinfo datainfo;
	HOURDATA hourdata[24];
	gdFontPtr font;

	if (ic->large) {
		font = gdFontGetSmall();
	} else {
		font = gdFontGetTiny();
	}

	for (i = 0; i < 24; i++) {
		hourdata[i].rx = hourdata[i].tx = 0;
		hourdata[i].date = 0;
	}

	if (!db_getdata(&datalist, &datainfo, ic->interface.name, "hour", 24) || datainfo.count == 0) {
		gdImageString(ic->im, ic->font, x + (32 * ic->font->w), y + 54, (unsigned char *)"no data available", ic->ctext);
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
		if (rate) {
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

	if (ic->large) {
		x += 14;
		extrax = 145;
		extray = 35;
	}

	/* scale values */
	scaleunit = getscale(max, rate);

	s = (int)lrint(((double)scaleunit / (double)max) * (124 + extray));
	if (s < SCALEMINPIXELS) {
		step = 2;
	} else {
		step = 1;
	}

	for (i = step; i * s <= (124 + extray + 4); i = i + step) {
		gdImageDashedLine(ic->im, x + 36, y + 124 - (i * s), x + 460 + extrax, y + 124 - (i * s), ic->cline);
		gdImageDashedLine(ic->im, x + 36, y + 124 - prev - (step * s) / 2, x + 460 + extrax, y + 124 - prev - (step * s) / 2, ic->clinel);
		gdImageString(ic->im, font, x + 16 - (ic->large * 3), y + 121 - (i * s) - (ic->large * 3), (unsigned char *)getimagevalue(scaleunit * (unsigned int)i, 3, rate), ic->ctext);
		prev = i * s;
	}
	if ((prev + (step * s) / 2) <= (124 + extray + 4)) {
		gdImageDashedLine(ic->im, x + 36, y + 124 - prev - (step * s) / 2, x + 460 + extrax, y + 124 - prev - (step * s) / 2, ic->clinel);
	}

	/* scale text */
	gdImageStringUp(ic->im, font, x - 2 - (ic->large * 14), y + 58 + (rate * 10) - (extray / 2), (unsigned char *)getimagescale(scaleunit * (unsigned int)step, rate), ic->ctext);

	/* axis */
	gdImageLine(ic->im, x + 36 - 4, y + 124, x + 466 + extrax, y + 124, ic->ctext);
	gdImageLine(ic->im, x + 36, y - 10 - extray, x + 36, y + 124 + 4, ic->ctext);

	/* arrows */
	drawarrowup(ic, x + 36, y - 9 - extray);
	drawarrowright(ic, x + 465 + extrax, y + 124);

	/* x-axis values and poles */
	for (i = 0; i < 24; i++) {
		s = tmax - i;
		if (s < 0) {
			s += 24;
		}
		snprintf(buffer, 32, "%02d ", s);
		if (hourdata[s].date == 0) {
			chour = ic->cline;
		} else {
			chour = ic->ctext;
		}
		gdImageString(ic->im, font, x + 440 - (i * (17 + ic->large * 6)) + extrax, y + 128, (unsigned char *)buffer, chour);
		drawpoles(ic, x + 438 - (i * (17 + ic->large * 6)) + extrax, y - extray, 124 + extray, hourdata[s].rx, hourdata[s].tx, max);
		gdImageLine(ic->im, x + 438 - 2 - (i * (17 + ic->large * 6)) + extrax, y + 124, x + 438 + 14 - (i * (17 + ic->large * 6)) + extrax, y + 124, chour);
	}

	return 1;
}

void drawhourly(IMAGECONTENT *ic, const int rate)
{
	int width, height, headermod = 0;

	width = 500 + (ic->large * 168);
	height = 200 + (ic->large * 48);

	if (!ic->showheader) {
		headermod = 26;
		height -= 22;
	}

	imageinit(ic, width, height);
	layoutinit(ic, " / hourly", width, height);

	if (drawhours(ic, 12, 46 - headermod + (ic->large * 40), rate)) {
		drawlegend(ic, width / 2 - (ic->large * 10), 183 - headermod + (ic->large * 46), 0);
	}
}

void drawlist(IMAGECONTENT *ic, const char *listname)
{
	ListType listtype = LT_None;
	int textx, texty, offsetx = 0;
	int width, height, headermod, i = 1, rowcount = 0;
	int estimateavailable = 0, estimatevisible = 0;
	int32_t limit;
	uint64_t e_rx = 0, e_tx = 0, e_secs = 0;
	char buffer[512], datebuff[16], daybuff[16];
	char stampformat[64], titlename[16], colname[8];
	struct tm *d;
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
		offsetx = 5 * ic->font->w;
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
		getestimates(&e_rx, &e_tx, listtype, ic->interface.updated, &datalist);
		if (cfg.estimatestyle > 0 && e_rx + e_tx > datainfo.max) {
			datainfo.max = e_rx + e_tx;
		}
		estimateavailable = 1;
		if (listtype == LT_Day || listtype == LT_Month || listtype == LT_Year) {
			estimatevisible = 1;
		}
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

	width = 83 * ic->font->w + 2 + (ic->large * 2);
	height = 62 + 3 * ic->lineheight;

	// less space needed when no estimate or sum is shown (Top, 5min and Hour never have estimate)
	if ((!estimatevisible && datainfo.count < 2) || (listtype == LT_Top || listtype == LT_Hour || listtype == LT_5min)) {
		height = 62 + 2 * ic->lineheight;
	}

	// exception for 5min and Hour when having sum shown
	if ((listtype == LT_5min || listtype == LT_Hour) && datainfo.count > 1 && strlen(ic->dataend) > 0) {
		height = 62 + 3 * ic->lineheight;
	}

	height += (ic->lineheight + cfg.linespaceadjust) * rowcount - cfg.linespaceadjust;

	// "no data available"
	if (!datainfo.count) {
		height = 98 + (ic->large * 12);
	}

	if (!ic->showheader) {
		headermod = 26;
		height -= 22;
	} else {
		headermod = 0;
	}

	snprintf(buffer, 512, " / %s", titlename);

	imageinit(ic, width, height);
	layoutinit(ic, buffer, width, height);

	if (datainfo.count) {
		if (listtype == LT_Top) {
			if (cfg.ostyle <= 2) {
				drawlegend(ic, 66 * ic->font->w + 2, 40 - headermod, 0);
			}
			current = time(NULL);
			d = localtime(&current);
			strftime(daybuff, 16, stampformat, d);
		} else { // everything else
			if (cfg.ostyle > 2) {
				if (estimateavailable && cfg.barshowsrate) {
					drawlegend(ic, 72 * ic->font->w, 40 - headermod, 1);
				} else {
					drawlegend(ic, 72 * ic->font->w, 40 - headermod, 0);
				}
			} else {
				drawlegend(ic, 64 * ic->font->w + 1, 40 - headermod, 0);
			}
		}
	}

	textx = 10;
	texty = 40 - headermod;

	if (listtype == LT_Top) { // top
		snprintf(buffer, 512, "   #      day        rx           tx          total");
	} else { // everything else
		snprintf(buffer, 512, " %8s       rx           tx          total", colname);
	}
	if (cfg.ostyle > 2) {
		strcat(buffer, "       avg. rate");
		gdImageString(ic->im, ic->font, textx, texty, (unsigned char *)buffer, ic->ctext);
		gdImageLine(ic->im, textx + 2, texty + ic->lineheight + 4, textx + (65 * ic->font->w) + offsetx + 2, texty + ic->lineheight + 4, ic->cline);
	} else {
		gdImageString(ic->im, ic->font, textx, texty, (unsigned char *)buffer, ic->ctext);
		gdImageLine(ic->im, textx + 2, texty + ic->lineheight + 4, textx + (50 * ic->font->w) + offsetx - 4, texty + ic->lineheight + 4, ic->cline);
	}

	texty += ic->lineheight + 8;

	if (datainfo.count) {
		gdImageLine(ic->im, textx + (24 * ic->font->w) + offsetx, texty - 6 - ic->lineheight, textx + (24 * ic->font->w) + offsetx, texty + ((ic->lineheight + cfg.linespaceadjust) * rowcount) - cfg.linespaceadjust + 5 - (ic->large * 2), ic->cline);
		gdImageLine(ic->im, textx + (37 * ic->font->w) + offsetx, texty - 6 - ic->lineheight, textx + (37 * ic->font->w) + offsetx, texty + ((ic->lineheight + cfg.linespaceadjust) * rowcount) - cfg.linespaceadjust + 5 - (ic->large * 2), ic->cline);
		if (cfg.ostyle > 2) {
			gdImageLine(ic->im, textx + (50 * ic->font->w) + offsetx, texty - 6 - ic->lineheight, textx + (50 * ic->font->w) + offsetx, texty + ((ic->lineheight + cfg.linespaceadjust) * rowcount) - cfg.linespaceadjust + 5 - (ic->large * 2), ic->cline);
		}
	} else {
		gdImageLine(ic->im, textx + (24 * ic->font->w) + offsetx, texty - 6 - ic->lineheight, textx + (24 * ic->font->w) + offsetx, texty - 4, ic->cline);
		gdImageLine(ic->im, textx + (37 * ic->font->w) + offsetx, texty - 6 - ic->lineheight, textx + (37 * ic->font->w) + offsetx, texty - 4, ic->cline);
		if (cfg.ostyle > 2) {
			gdImageLine(ic->im, textx + (50 * ic->font->w) + offsetx, texty - 6 - ic->lineheight, textx + (50 * ic->font->w) + offsetx, texty - 4, ic->cline);
		}
	}

	while (datalist_i != NULL) {
		d = localtime(&datalist_i->timestamp);

		if (listtype == LT_5min || listtype == LT_Hour) {
			strftime(datebuff, 16, cfg.dformat, d);
			if (strcmp(daybuff, datebuff) != 0) {
				snprintf(buffer, 32, " %s", datebuff);
				gdImageString(ic->im, ic->font, textx, texty, (unsigned char *)buffer, ic->ctext);
				texty += ic->lineheight + cfg.linespaceadjust;
				strcpy(daybuff, datebuff);
			}
		}

		if (listtype == LT_Top) {
			if (strftime(datebuff, 16, stampformat, d) <= 8) {
				snprintf(buffer, 32, "  %2d   %*s", i, getpadding(8, datebuff), datebuff);
				strcat(buffer, "   ");
			} else {
				snprintf(buffer, 32, "  %2d  %-*s ", i, getpadding(11, datebuff), datebuff);
			}
			if (strcmp(datebuff, daybuff) == 0) {
				if (cfg.ostyle > 2) {
					gdImageFilledRectangle(ic->im, textx + 2, texty + 2 - (ic->large * 1), textx + (65 * ic->font->w) + offsetx + 2, texty + 11 + (ic->large * 3), ic->cbgoffset);
				} else {
					gdImageFilledRectangle(ic->im, textx + 2, texty + 2 - (ic->large * 1), textx + (50 * ic->font->w) + offsetx - 4, texty + 11 + (ic->large * 3), ic->cbgoffset);
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
				e_secs = getperiodseconds(listtype, datalist_i->timestamp, ic->interface.updated, 1);
			} else {
				e_secs = getperiodseconds(listtype, datalist_i->timestamp, ic->interface.updated, 0);
			}
			strncat(buffer, gettrafficrate(datalist_i->rx + datalist_i->tx, (time_t)e_secs, 14), 32);
		}
		gdImageString(ic->im, ic->font, textx, texty, (unsigned char *)buffer, ic->ctext);
		if (listtype == LT_Top) {
			if (cfg.ostyle > 2) {
				drawbar(ic, textx + (71 * ic->font->w) + 2, texty + 4, 9 * ic->font->w - 2, datalist_i->rx, datalist_i->tx, datainfo.max, 0);
			} else {
				drawbar(ic, textx + (56 * ic->font->w), texty + 4, 23 * ic->font->w + 2, datalist_i->rx, datalist_i->tx, datainfo.max, 0);
			}
		} else { // everything else
			if (cfg.ostyle > 2) {
				if (datalist_i->next == NULL && estimateavailable && cfg.barshowsrate) {
					drawbar(ic, textx + (67 * ic->font->w) - 2, texty + 4, 13 * ic->font->w, e_rx, e_tx, datainfo.max, 0);
				} else {
					drawbar(ic, textx + (67 * ic->font->w) - 2, texty + 4, 13 * ic->font->w, datalist_i->rx, datalist_i->tx, datainfo.max, 0);
				}
			} else {
				drawbar(ic, textx + (51 * ic->font->w) - 2, texty + 4, 28 * ic->font->w + 2, datalist_i->rx, datalist_i->tx, datainfo.max, 0);
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
		i = 17 * ic->font->w;
		if (cfg.ostyle > 2) {
			i += 8 * ic->font->w;
		}
		gdImageString(ic->im, ic->font, textx + i, texty, (unsigned char *)"no data available", ic->ctext);
		texty += ic->lineheight;
	}

	if (cfg.ostyle > 2) {
		gdImageLine(ic->im, textx + 2, texty + 5 - (ic->large * 2), textx + (65 * ic->font->w) + offsetx + 2, texty + 5 - (ic->large * 2), ic->cline);
	} else {
		gdImageLine(ic->im, textx + 2, texty + 5 - (ic->large * 2), textx + (50 * ic->font->w) + offsetx - 4, texty + 5 - (ic->large * 2), ic->cline);
	}

	buffer[0] = '\0';

	/* estimate visible */
	if (estimatevisible) {
		snprintf(buffer, 32, " estimated   ");
		strncat(buffer, getvalue(e_rx, 10, RT_Estimate), 32);
		strcat(buffer, "   ");
		strncat(buffer, getvalue(e_tx, 10, RT_Estimate), 32);
		strcat(buffer, "   ");
		strncat(buffer, getvalue(e_rx + e_tx, 10, RT_Estimate), 32);

		if (cfg.estimatestyle) {
			if (cfg.ostyle > 2) {
				drawbar(ic, textx + (67 * ic->font->w) - 2, texty - ic->lineheight + 4, 13 * ic->font->w, e_rx, e_tx, datainfo.max, 1);
				drawbar(ic, textx + (67 * ic->font->w) - 2, texty - ic->lineheight + 4, 13 * ic->font->w, datalist_i->rx, datalist_i->tx, datainfo.max, 0);
			} else {
				drawbar(ic, textx + (51 * ic->font->w) - 2, texty - ic->lineheight + 4, 28 * ic->font->w + 2, e_rx, e_tx, datainfo.max, 1);
				drawbar(ic, textx + (51 * ic->font->w) - 2, texty - ic->lineheight + 4, 28 * ic->font->w + 2, datalist_i->rx, datalist_i->tx, datainfo.max, 0);
			}
		}

	/* sum visible */
	} else if (strlen(ic->dataend) > 0 && datainfo.count > 1 && listtype != LT_Top) {
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
	}

	if (strlen(buffer) > 0) {
		texty += 8;
		gdImageString(ic->im, ic->font, textx, texty, (unsigned char *)buffer, ic->ctext);

		gdImageLine(ic->im, textx + (24 * ic->font->w) + offsetx, texty - 6, textx + (24 * ic->font->w) + offsetx, texty + ic->lineheight - (ic->large * 2), ic->cline);
		gdImageLine(ic->im, textx + (37 * ic->font->w) + offsetx, texty - 6, textx + (37 * ic->font->w) + offsetx, texty + ic->lineheight - (ic->large * 2), ic->cline);
		if (cfg.ostyle > 2) {
			gdImageLine(ic->im, textx + (50 * ic->font->w) + offsetx, texty - 6, textx + (50 * ic->font->w) + offsetx, texty + ic->lineheight - (ic->large * 2), ic->cline);
		}
	}

	dbdatalistfree(&datalist);
}

void drawsummary(IMAGECONTENT *ic, const int layout, const int rate)
{
	int width, height, headermod;

	switch (layout) {
		// horizontal
		case 1:
			width = 163 * ic->font->w + 2 + (ic->large * 2);
			height = 56 + 12 * ic->lineheight;
			break;
		// vertical
		case 2:
			width = 83 * ic->font->w + 2 + (ic->large * 2);
			height = 370 + (ic->large * 90);
			break;
		// no hours
		default:
			width = 83 * ic->font->w + 2 + (ic->large * 2);
			height = 56 + 12 * ic->lineheight;
			break;
	}

	if (!ic->showheader) {
		headermod = 26;
		height -= 22;
	} else {
		headermod = 0;
	}

	imageinit(ic, width, height);
	layoutinit(ic, "", width, height);

	if (ic->interface.rxtotal == 0 && ic->interface.txtotal == 0) {
		gdImageString(ic->im, ic->font, 33 * ic->font->w, 100, (unsigned char *)"no data available", ic->ctext);
		return;
	}

	drawsummary_alltime(ic, 385 + (ic->large * 125), 57 - headermod + (ic->large * 10));
	drawlegend(ic, 410 + (ic->large * 132), 155 - headermod + (ic->large * 40), 0);

	drawsummary_digest(ic, 100, 30 - headermod, "day");
	drawsummary_digest(ic, 100, 29 + 7 * ic->lineheight - headermod, "month");

	switch (layout) {
		// horizontal
		case 1:
			if (cfg.summarygraph == 1) {
				drawfiveminutes(ic, 496 + (ic->large * 174), height - 30 - (ic->large * 8), rate, 422 + (ic->large * 154), height - 68 + headermod - (ic->large * 8));
			} else {
				drawhours(ic, 500 + (ic->large * 160), 46 + (ic->large * 40) - headermod, rate);
			}
			break;
		// vertical
		case 2:
			if (cfg.summarygraph == 1) {
				drawfiveminutes(ic, 8 + (ic->large * 14), height - 31 - (ic->large * 6), rate, 422 + (ic->large * 154), 132 + (ic->large * 35));
			} else {
				drawhours(ic, 12, 215 + (ic->large * 84) - headermod, rate);
			}
			break;
		default:
			break;
	}
}

void drawsummary_alltime(IMAGECONTENT *ic, const int x, const int y)
{
	struct tm *d;
	char buffer[512], datebuff[16], daytemp[32];
	gdFontPtr titlefont;

	if (ic->large) {
		titlefont = gdFontGetGiant();
	} else {
		titlefont = gdFontGetLarge();
	}

	gdImageString(ic->im, titlefont, x + 12 + (ic->large * 10), y, (unsigned char *)"all time", ic->ctext);
	snprintf(buffer, 4, "rx ");
	strncat(buffer, getvalue(ic->interface.rxtotal, 12, RT_Normal), 32);
	gdImageString(ic->im, ic->font, x, y + (2 * ic->lineheight), (unsigned char *)buffer, ic->ctext);
	snprintf(buffer, 4, "tx ");
	strncat(buffer, getvalue(ic->interface.txtotal, 12, RT_Normal), 32);
	gdImageString(ic->im, ic->font, x, y + (3 * ic->lineheight), (unsigned char *)buffer, ic->ctext);
	snprintf(buffer, 4, " = ");
	strncat(buffer, getvalue(ic->interface.rxtotal + ic->interface.txtotal, 12, RT_Normal), 32);
	gdImageString(ic->im, ic->font, x, y + (4 * ic->lineheight) + 2 + (ic->large * 4), (unsigned char *)buffer, ic->ctext);
	d = localtime(&ic->interface.created);
	strftime(datebuff, 16, cfg.tformat, d);
	snprintf(daytemp, 24, "since %s", datebuff);
	snprintf(buffer, 32, "%23s", daytemp);
	gdImageString(ic->im, ic->font, x - 8 * ic->font->w, y + (5 * ic->lineheight) + 10 + (ic->large * 4), (unsigned char *)buffer, ic->ctext);
}

void drawsummary_digest(IMAGECONTENT *ic, const int x, const int y, const char *mode)
{
	int textx, texty, offset = 0;
	double rxp = 50, txp = 50, mod;
	char buffer[512], datebuff[16], daytemp[32];
	time_t yesterday;
	struct tm *d = NULL;
	dbdatalist *datalist = NULL;
	dbdatalist *data_current = NULL, *data_previous = NULL;
	dbdatalistinfo datainfo;
	gdFontPtr titlefont;

	if (ic->large) {
		titlefont = gdFontGetGiant();
	} else {
		titlefont = gdFontGetLarge();
	}

	yesterday = ic->current - 86400;

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
		gdImageString(ic->im, ic->font, 25 * ic->font->w, y + 30, (unsigned char *)"no data available", ic->ctext);
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
		offset = 85 + (ic->large * 25);
	}

	textx = x + offset;
	texty = y;

	drawdonut(ic, textx + 50 + (ic->large * 40), texty + 45 + (ic->large * 10), (float)rxp, (float)txp, 49 + (ic->large * 10), 15 + (ic->large * 3));

	if (mode[0] == 'd') {
		/* get formatted date for today */
		d = localtime(&ic->current);
		strftime(datebuff, 16, cfg.dformat, d);

		/* get formatted date for current day in database */
		d = localtime(&data_current->timestamp);
		strftime(daytemp, 16, cfg.dformat, d);

		/* change daytemp to today if formatted days match */
		if (strcmp(datebuff, daytemp) == 0) {
			strncpy_nt(daytemp, "today", 32);
		}
	} else if (mode[0] == 'm') {
		d = localtime(&data_current->timestamp);
		strftime(daytemp, 16, cfg.mformat, d);
	}

	snprintf(buffer, 32, "%*s", getpadding(12, daytemp), daytemp);
	gdImageString(ic->im, titlefont, textx - 54 + (ic->large * (ic->font->w * 3 - 4)), texty - 1, (unsigned char *)buffer, ic->ctext);

	if (cfg.summaryrate) {
		d = localtime(&ic->interface.updated);
		if (mode[0] == 'd') {
			snprintf(buffer, 16, "%15s", gettrafficrate(data_current->rx + data_current->tx, d->tm_sec + (d->tm_min * 60) + (d->tm_hour * 3600), 15));
		} else if (mode[0] == 'm') {
			snprintf(buffer, 16, "%15s", gettrafficrate(data_current->rx + data_current->tx, mosecs(data_current->timestamp, ic->interface.updated), 15));
		}
		gdImageString(ic->im, ic->font, textx - 74, texty + 4 * ic->lineheight + 10, (unsigned char *)buffer, ic->ctext);
	} else {
		texty += 7;
	}

	snprintf(buffer, 4, "rx ");
	strncat(buffer, getvalue(data_current->rx, 12, RT_Normal), 32);
	gdImageString(ic->im, ic->font, textx - 74, texty + ic->lineheight + 6, (unsigned char *)buffer, ic->ctext);
	snprintf(buffer, 4, "tx ");
	strncat(buffer, getvalue(data_current->tx, 12, RT_Normal), 32);
	gdImageString(ic->im, ic->font, textx - 74, texty + 2 * ic->lineheight + 6, (unsigned char *)buffer, ic->ctext);
	snprintf(buffer, 4, " = ");
	strncat(buffer, getvalue(data_current->rx + data_current->tx, 12, RT_Normal), 32);
	gdImageString(ic->im, ic->font, textx - 74, texty + 3 * ic->lineheight + 8, (unsigned char *)buffer, ic->ctext);

	/* previous entry */
	if (data_previous != NULL) {
		if (data_previous->rx + data_previous->tx == 0) {
			rxp = txp = 0;
		} else {
			rxp = (double)data_previous->rx / (double)(data_previous->rx + data_previous->tx) * 100;
			txp = (double)100 - rxp;
		}

		/* do scaling if needed */
		if ((data_previous->rx + data_previous->tx) < (data_current->rx + data_current->tx)) {
			mod = (double)(data_previous->rx + data_previous->tx) / (double)(data_current->rx + data_current->tx);
			rxp = rxp * mod;
			txp = txp * mod;
		}

		textx += 180 + (ic->large * 60);

		drawdonut(ic, textx + 50 + (ic->large * 40), texty + 45 + (ic->large * 10), (float)rxp, (float)txp, 49 + (ic->large * 10), 15 + (ic->large * 3));

		if (mode[0] == 'd') {
			/* get formatted date for yesterday */
			d = localtime(&yesterday);
			strftime(datebuff, 16, cfg.dformat, d);

			/* get formatted date for previous day in database */
			d = localtime(&data_previous->timestamp);
			strftime(daytemp, 16, cfg.dformat, d);

			/* change daytemp to yesterday if formatted days match */
			if (strcmp(datebuff, daytemp) == 0) {
				strncpy_nt(daytemp, "yesterday", 32);
			}
		} else if (mode[0] == 'm') {
			d = localtime(&data_previous->timestamp);
			strftime(daytemp, 16, cfg.mformat, d);
		}

		snprintf(buffer, 32, "%*s", getpadding(12, daytemp), daytemp);
		gdImageString(ic->im, titlefont, textx - 54 + (ic->large * (ic->font->w * 3 - 4)), texty - 1, (unsigned char *)buffer, ic->ctext);

		if (cfg.summaryrate) {
			if (mode[0] == 'd') {
				snprintf(buffer, 16, "%15s", gettrafficrate(data_previous->rx + data_previous->tx, 86400, 15));
			} else if (mode[0] == 'm') {
				snprintf(buffer, 16, "%15s", gettrafficrate(data_previous->rx + data_previous->tx, dmonth(d->tm_mon) * 86400, 15));
			}
			gdImageString(ic->im, ic->font, textx - 74, texty + 4 * ic->lineheight + 10, (unsigned char *)buffer, ic->ctext);
		} else {
			texty += 7;
		}

		snprintf(buffer, 4, "rx ");
		strncat(buffer, getvalue(data_previous->rx, 12, RT_Normal), 32);
		gdImageString(ic->im, ic->font, textx - 74, texty + ic->lineheight + 6, (unsigned char *)buffer, ic->ctext);
		snprintf(buffer, 4, "tx ");
		strncat(buffer, getvalue(data_previous->tx, 12, RT_Normal), 32);
		gdImageString(ic->im, ic->font, textx - 74, texty + 2 * ic->lineheight + 6, (unsigned char *)buffer, ic->ctext);
		snprintf(buffer, 4, " = ");
		strncat(buffer, getvalue(data_previous->rx + data_previous->tx, 12, RT_Normal), 32);
		gdImageString(ic->im, ic->font, textx - 74, texty + 3 * ic->lineheight + 8, (unsigned char *)buffer, ic->ctext);
	}

	data_current = NULL;
	data_previous = NULL;
	dbdatalistfree(&datalist);
}

void drawfivegraph(IMAGECONTENT *ic, const int rate, const int resultcount, const int height)
{
	int imagewidth, imageheight = height, headermod = 0;

	imagewidth = resultcount + FIVEMINEXTRASPACE + (ic->large * 14);

	if (!ic->showheader) {
		headermod = 22;
	}

	imageinit(ic, imagewidth, imageheight);
	layoutinit(ic, " / 5 minute", imagewidth, imageheight);

	if (drawfiveminutes(ic, 8 + (ic->large * 14), imageheight - 30 - (ic->large * 8), rate, resultcount, imageheight - 68 + headermod - (ic->large * 8))) {
		drawlegend(ic, imagewidth / 2 - (ic->large * 10), imageheight - 17 - (ic->large * 2), 0);
	}
}

int drawfiveminutes(IMAGECONTENT *ic, const int xpos, const int ypos, const int rate, const int resultcount, const int height)
{
	int x = xpos, y = ypos, i = 0, t = 0, rxh = 0, txh = 0, step = 0, s = 0, prev = 0;
	uint64_t scaleunit, max;
	time_t timestamp;
	double ratediv, e;
	char buffer[32];
	struct tm *d;
	dbdatalist *datalist = NULL, *datalist_i = NULL;
	dbdatalistinfo datainfo;
	gdFontPtr font;

	if (ic->large) {
		font = gdFontGetSmall();
	} else {
		font = gdFontGetTiny();
	}

	if (!db_getdata(&datalist, &datainfo, ic->interface.name, "fiveminute", (uint32_t)resultcount) || datainfo.count == 0) {
		gdImageString(ic->im, ic->font, x + 330 - (8 * ic->font->w), y - (height / 2) - ic->font->h, (unsigned char *)"no data available", ic->ctext);
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
	gdImageString(ic->im, font, x - 21 - (ic->large * 3), y - 4 - (ic->large * 3), (unsigned char *)"  0", ic->ctext);

	/* scale values */
	scaleunit = getscale(max, rate);

	s = (int)lrint(((double)scaleunit / (double)max) * t);
	if (s == 0) {
		s = 1; // force to show something when there's not much or any traffic, scale is likely to be wrong in this case
	}
	while (s * step < SCALEMINPIXELS) {
		step++;
	}

	if (debug) { // TODO: cleanup
		printf("maxrx: %lu\n", datainfo.maxrx);
		printf("maxtx: %lu\n", datainfo.maxtx);
		printf("rxh: %d     txh: %d\n", rxh, txh);
		printf("max divided: %lu\n", max);
		printf("scaleunit:   %lu\nstep: %d\n", scaleunit, step);
		printf("pixels per step: %d\n", s);
		printf("mintime: %lu\nmaxtime: %lu\n", (uint64_t)datainfo.mintime, (uint64_t)datainfo.maxtime);
		printf("count: %u\n", datainfo.count);
	}

	/* upper part scale values */
	y--; // adjust to start above center line
	for (i = step; i * s <= rxh; i = i + step) {
		gdImageDashedLine(ic->im, x, y - (i * s), x + (resultcount + FIVEMINWIDTHPADDING), y - (i * s), ic->cline);
		gdImageDashedLine(ic->im, x, y - prev - (step * s) / 2, x + (resultcount + FIVEMINWIDTHPADDING), y - prev - (step * s) / 2, ic->clinel);
		gdImageString(ic->im, font, x - 21 - (ic->large * 3), y - 3 - (i * s) - (ic->large * 3), (unsigned char *)getimagevalue(scaleunit * (unsigned int)i, 3, rate), ic->ctext);
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
		gdImageString(ic->im, font, x - 21 - (ic->large * 3), y - 3 + (i * s) - (ic->large * 3), (unsigned char *)getimagevalue(scaleunit * (unsigned int)i, 3, rate), ic->ctext);
		prev = i * s;
	}
	if ((prev + (step * s) / 2) <= txh) {
		gdImageDashedLine(ic->im, x, y + prev + (step * s) / 2, x + (resultcount + FIVEMINWIDTHPADDING), y + prev + (step * s) / 2, ic->clinel);
	}

	y--; // y is now back on center line

	/* scale text */
	gdImageStringUp(ic->im, font, x - 39 - (ic->large * 14), ypos - height / 2 + (rate * 10), (unsigned char *)getimagescale(scaleunit * (unsigned int)step, rate), ic->ctext);

	timestamp = datainfo.maxtime - (resultcount * 300);

	while (datalist_i != NULL && datalist_i->timestamp < timestamp + 300) {
		if (debug) {
			printf("Skip data, %lu < %lu\n", datalist_i->timestamp, timestamp + 300);
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

				if (i > font->w) {
					snprintf(buffer, 32, "%02d", d->tm_hour);
					if (datalist_i->timestamp > timestamp) {
						gdImageString(ic->im, font, x + i - font->w + 1, y + txh + font->h - (ic->large * 5), (unsigned char *)buffer, ic->cline);
					} else {
						gdImageString(ic->im, font, x + i - font->w + 1, y + txh + font->h - (ic->large * 5), (unsigned char *)buffer, ic->ctext);
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
