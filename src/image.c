#include "common.h"
#include "image.h"
#include "image_font.h"
#include "image_list.h"
#include "image_summary.h"
#include "image_graph.h"

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
	ic->commonwidth = 0;
	ic->headertext[0] = '\0';
	ic->databegin[0] = '\0';
	ic->dataend[0] = '\0';
	ic->interface.name[0] = '\0';
	ic->interface.alias[0] = '\0';
}

void drawimage(IMAGECONTENT *ic)
{
	if (cfg.commonwidth) {
		ic->commonwidth = image_common_target_width(ic);
	} else {
		ic->commonwidth = 0;
	}

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

