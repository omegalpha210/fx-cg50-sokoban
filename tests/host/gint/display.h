#ifndef HOST_DISPLAY_H
#define HOST_DISPLAY_H
#include <stdint.h>
#define DWIDTH 396
#define DHEIGHT 224
#define C_BLACK 0
#define C_WHITE 0xffff
#define C_RGB(r,g,b) (((r)<<11)|((g)<<6)|(b))
void dclear(uint16_t color);
void drect(int x1,int y1,int x2,int y2,int color);
void dpixel(int x,int y,int color);
void dupdate(void);
extern uint16_t host_pixels[396*224];
extern unsigned host_out_of_bounds;
#endif
