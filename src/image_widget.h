#ifndef IMAGE_WIDGET_H
#define IMAGE_WIDGET_H

#include <stdint.h>
#include "image.h"

void graph_draw_axis_value(IMAGECONTENT *ic, const int axis_x, const int line_y, const char *val, const int builtin_x, const int builtin_y);
void graph_draw_axis_unit(IMAGECONTENT *ic, const int x_ttf, const int x_builtin, const int y, const char *text);
void drawlegend(IMAGECONTENT *ic, const int x, const int y, const short israte);
int percentilelegendwidth(IMAGECONTENT *ic, const int mode, const uint64_t percentile);
void drawpercentilelegend(IMAGECONTENT *ic, const int x, const int y, const int mode, const uint64_t percentile);
void drawbar(IMAGECONTENT *ic, const int x, const int y, const int len, const uint64_t rx, const uint64_t tx, const uint64_t max, const short isestimate);
void drawpoles(IMAGECONTENT *ic, const int x, const int y, const int len, const uint64_t rx, const uint64_t tx, const uint64_t max);
void drawdonut(IMAGECONTENT *ic, const int x, const int y, const float rxp, const float txp, const int size, const int holesize);
void drawdonut_libgd_bug_workaround(IMAGECONTENT *ic, const int x, const int y, const float rxp, const float txp, const int size, const int holesize);
void drawdonut_libgd_native(IMAGECONTENT *ic, const int x, const int y, const float rxp, const float txp, const int size, const int holesize);
void drawpole(IMAGECONTENT *ic, const int x, const int y, const int length, const int direction, const int color);
void drawarrowup(IMAGECONTENT *ic, const int x, const int y);
void drawarrowright(IMAGECONTENT *ic, const int x, const int y);

#endif
