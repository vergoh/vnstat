#ifndef IMAGE_FONT_H
#define IMAGE_FONT_H

#include "image.h"

/* sample text for TTF height measurement with ascenders and descenders */
#define TTF_HEIGHT_SAMPLE "Ayjp"

int imagefontinit(IMAGECONTENT *ic, const int largefonts);
void imagefontcleanup(void);
void imagestring(IMAGECONTENT *ic, const fontrole_t role, const int x, const int y, const char *text, const int color);
void imagestringup(IMAGECONTENT *ic, const fontrole_t role, const int x, const int y, const char *text, const int color);
int imagetextwidth(const IMAGECONTENT *ic, const fontrole_t role, const char *text);
int imagefontwidth(IMAGECONTENT *ic, const fontrole_t role);
int imagefontheight(IMAGECONTENT *ic, const fontrole_t role);
int imageextrapx(const IMAGECONTENT *ic, const int extra);
int imageuipx(const IMAGECONTENT *ic, const int base);
int imagecentery(IMAGECONTENT *ic, const fontrole_t role, const char *text, const int rect_top, const int rect_bottom);

#endif
