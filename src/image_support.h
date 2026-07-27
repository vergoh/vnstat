#ifndef IMAGE_SUPPORT_H
#define IMAGE_SUPPORT_H

#include <stdint.h>
#include "image.h"

void imageinit(IMAGECONTENT *ic, const int width, const int height);
void colorinitcheck(const char *color, const int value, const char *cfgtext, const int *rgb);
void imagedrawhline(IMAGECONTENT *ic, const int x1, const int x2, const int y, const int color);
void imagedrawvline(IMAGECONTENT *ic, const int x, const int y1, const int y2, const int color);
void imagedrawrect(IMAGECONTENT *ic, const int x1, const int y1, const int x2, const int y2, const int color);
void imagedrawdashedhline(IMAGECONTENT *ic, const int x1, const int x2, const int y, const int color);
int graph_axis_left(const IMAGECONTENT *ic);
int graph_stroke_half(const IMAGECONTENT *ic);
int graph_xpos_margin(const IMAGECONTENT *ic);
int graph_extra_space(const IMAGECONTENT *ic);
int hourly_plot_extrax(const IMAGECONTENT *ic);
int hourly_graph_left(const IMAGECONTENT *ic);
int hourly_graph_width(const IMAGECONTENT *ic);
int hourly_hour_step(const IMAGECONTENT *ic);
int hourly_map_px(const IMAGECONTENT *ic, const int design);
int image_list_width(const IMAGECONTENT *ic);
int image_list_bar_extra(const IMAGECONTENT *ic, const int natural_width, const int design_bar_len);
void layoutinit(IMAGECONTENT *ic, const char *title, const int width, const int height);
void hextorgb(const char *input, int *rgb);
void modcolor(int *rgb, const int offset, const int force);
void invertcolor(int *rgb);
void rtrimspaces(char *s);
char *getimagevalue(const uint64_t b, const int len, const int israte);
char *getimagescale(const uint64_t b, const int israte);
uint64_t getscale(const uint64_t input, const int israte);

#endif
