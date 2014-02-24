#ifndef VNSTATI_H
#define VNSTATI_H

#include <sys/stat.h>  /* fstat() */
#include <gd.h>        /* libgd2-dev libgd2 */
#include <gdfontt.h>   /* gdFontGetTiny() */
#include <gdfonts.h>   /* gdFontGetSmall() */
#include <gdfontmb.h>  /* gdFontGetMediumBold() */
#include <gdfontl.h>   /* gdFontGetLarge() */
#include <gdfontg.h>   /* gdFontGetGiant() */

#define YBEGINOFFSET -1
#define YENDOFFSET 6

#define DOUTRAD 49
#define DINRAD 15 

/* global variables for vnstati */
gdImagePtr im;
int cbackground, cedge, cheader, cheadertitle, cheaderdate, ctext, cline, clinel, cvnstat;
int crx, crxd, ctx, ctxd, cbgoffset;
time_t current;

#endif
