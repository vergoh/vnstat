#ifndef IMAGE_GRAPH_H
#define IMAGE_GRAPH_H

#include <stdint.h>
#include "image.h"

int drawhours(IMAGECONTENT *ic, const int xpos, const int ypos, const int israte);
void drawhourly(IMAGECONTENT *ic, const int israte);
int fiveg_barwidth(const IMAGECONTENT *ic);
void drawfivegraph(IMAGECONTENT *ic, const int israte, const int resultcount, const int height);
int drawfiveminutes(IMAGECONTENT *ic, const int xpos, const int ypos, const int israte, const int resultcount, const int height, const int barwidth);
void draw95thpercentilegraph(IMAGECONTENT *ic, const int mode);
int drawpercentile(IMAGECONTENT *ic, const int mode, const int xpos, const int ypos, const int height, uint64_t *percentile);

#endif
