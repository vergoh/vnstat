#ifndef IMAGE_H
#define IMAGE_H

void colorinit(void);
void colorinitcheck(char *color, int value, char *cfgtext, int *rgb);
void layoutinit(char *title, int width, int height, int showheader, int showedge);
void drawlegend(int x, int y);
void drawbar(int x, int y, int len, uint64_t rx, int rxk, uint64_t tx, int txk, uint64_t max);
void drawpole(int x, int y, int len, uint64_t rx, uint64_t tx, uint64_t max);
void drawdonut(int x, int y, float rxp, float txp);
void drawhours(int x, int y, int rate);
void drawsummary(int type, int showheader, int showedge, int rate);
void drawoldsummary(int type, int showheader, int showedge, int rate);
void drawhourly(int showheader, int showedge, int rate);
void drawdaily(int showheader, int showedge);
void drawmonthly(int showheader, int showedge);
void drawtop(int showheader, int showedge);
void addtraffic(uint64_t *destmb, int *destkb, uint64_t srcmb, int srckb);
void hextorgb(char *input, int *rgb);
void modcolor(int *rgb, int offset, int force);
char *getimagevalue(uint64_t kb, int len, int rate);
char *getimagescale(uint64_t kb, int rate);

#endif
