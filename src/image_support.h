#ifndef IMAGE_SUPPORT_H
#define IMAGE_SUPPORT_H

#include "image.h"

void imageinit(IMAGECONTENT *ic, const int width, const int height);
void colorinitcheck(const char *color, const int value, const char *cfgtext, const int *rgb);
void layoutinit(IMAGECONTENT *ic, const char *title, const int width, const int height);
void drawlegend(IMAGECONTENT *ic, const int x, const int y, const short israte);
void drawpercentilelegend(IMAGECONTENT *ic, const int x, const int y, const int mode, const uint64_t percentile);
void drawbar(IMAGECONTENT *ic, const int x, const int y, const int len, const uint64_t rx, const uint64_t tx, const uint64_t max, const short isestimate);
void drawpoles(IMAGECONTENT *ic, const int x, const int y, const int len, const uint64_t rx, const uint64_t tx, const uint64_t max);
void drawdonut(IMAGECONTENT *ic, const int x, const int y, const float rxp, const float txp, const int size, const int holesize);
void drawdonut_libgd_bug_workaround(IMAGECONTENT *ic, const int x, const int y, const float rxp, const float txp, const int size, const int holesize);
void drawdonut_libgd_native(IMAGECONTENT *ic, const int x, const int y, const float rxp, const float txp, const int size, const int holesize);
void drawpole(IMAGECONTENT *ic, const int x, const int y, const int length, const int direction, const int color);
void drawarrowup(IMAGECONTENT *ic, const int x, const int y);
void drawarrowright(IMAGECONTENT *ic, const int x, const int y);
void hextorgb(const char *input, int *rgb);
void modcolor(int *rgb, const int offset, const int force);
void invertcolor(int *rgb);
char *getimagevalue(const uint64_t b, const int len, const int rate);
char *getimagescale(const uint64_t b, const int rate);
uint64_t getscale(const uint64_t input, const int rate);

#endif
