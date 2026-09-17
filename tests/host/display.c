#include <gint/display.h>
uint16_t host_pixels[396*224];
unsigned host_out_of_bounds;
void dclear(uint16_t color) {for(int i=0;i<396*224;i++)host_pixels[i]=color;}
void dpixel(int x,int y,int color) {if(x<0 || x>=396 || y<0 || y>=224){host_out_of_bounds++;return;} host_pixels[y*396+x]=(uint16_t)color;}
void drect(int x1,int y1,int x2,int y2,int color) {for(int y=y1;y<=y2;y++)for(int x=x1;x<=x2;x++)dpixel(x,y,color);}
void dupdate(void) {}
