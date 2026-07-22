#ifndef IMAGE_SUPPORT_H
#define IMAGE_SUPPORT_H

#include <stdint.h>
#include "image.h"

void imageinit(IMAGECONTENT *ic, const int width, const int height);
void colorinitcheck(const char *color, const int value, const char *cfgtext, const int *rgb);
int imagefontinit(IMAGECONTENT *ic, const int largefonts);
void imagefontcleanup(void);
void imagestring(IMAGECONTENT *ic, const fontrole_t role, const int x, const int y, const char *text, const int color);
void imagestringup(IMAGECONTENT *ic, const fontrole_t role, const int x, const int y, const char *text, const int color);
int imagetextwidth(IMAGECONTENT *ic, const fontrole_t role, const char *text);
int imagefontwidth(IMAGECONTENT *ic, const fontrole_t role);
int imagefontheight(IMAGECONTENT *ic, const fontrole_t role);
int imageextrapx(const IMAGECONTENT *ic, const int extra);
int imageuipx(const IMAGECONTENT *ic, const int base);
void imagedrawhline(IMAGECONTENT *ic, const int x1, const int x2, const int y, const int color);
void imagedrawvline(IMAGECONTENT *ic, const int x, const int y1, const int y2, const int color);
void imagedrawrect(IMAGECONTENT *ic, const int x1, const int y1, const int x2, const int y2, const int color);
void imagedrawdashedhline(IMAGECONTENT *ic, const int x1, const int x2, const int y, const int color);
int graph_axis_left(const IMAGECONTENT *ic);
int graph_xpos_margin(const IMAGECONTENT *ic);
int graph_extra_space(const IMAGECONTENT *ic);
int hourly_plot_extrax(const IMAGECONTENT *ic);
int hourly_graph_width(const IMAGECONTENT *ic);
void graph_draw_axis_value(IMAGECONTENT *ic, const int axis_x, const int line_y, const char *val, const int builtin_x, const int builtin_y);
void graph_draw_axis_unit(IMAGECONTENT *ic, const int x_ttf, const int x_builtin, const int y, const char *text);
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
char *getimagevalue(const uint64_t b, const int len, const int israte);
char *getimagescale(const uint64_t b, const int israte);
uint64_t getscale(const uint64_t input, const int israte);

#endif
