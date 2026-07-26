#ifndef IMAGE_SUMMARY_H
#define IMAGE_SUMMARY_H

#include "image.h"

void drawsummary(IMAGECONTENT *ic, const int layout, const int israte);
void drawsummary_alltime(IMAGECONTENT *ic, const int x, const int y);
void drawsummary_digest(IMAGECONTENT *ic, const int x, const int y, const char *mode);
int image_summary_width(IMAGECONTENT *ic, const int layout);
int image_common_target_width(IMAGECONTENT *ic);

#endif
