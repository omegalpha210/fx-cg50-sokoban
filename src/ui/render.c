#include "render.h"
#include <gint/display.h>
#include <stdio.h>
#include <string.h>
#include "font_data.h"
#define INK C_RGB(3,5,7)
#define MUTED C_RGB(12,14,15)
#define LINE C_RGB(24,26,26)
#define PAPER C_RGB(29,30,30)
#define ORANGE C_RGB(31,18,3)
static const int pale[4]={C_RGB(25,30,25),C_RGB(25,29,31),C_RGB(31,29,22),C_RGB(31,26,25)};
static const int group_color[4]={C_RGB(3,16,8),C_RGB(3,12,23),C_RGB(16,11,0),C_RGB(23,5,5)};
static const int walls[4]={C_RGB(7,21,11),C_RGB(6,16,28),C_RGB(26,20,4),C_RGB(27,8,7)};
static const char *const names[4]={"BASIC","INTERMEDIATE","ADVANCED","MASTER"};
static void rect(int x,int y,int w,int h,int color)
{if(w>0 && h>0)drect(x,y,x+w-1,y+h-1,color);}
static void border(int x,int y,int w,int h,int color,int thick)
{
    rect(x,y,w,thick,color);rect(x,y+h-thick,w,thick,color);
    rect(x,y,thick,h,color);rect(x+w-thick,y,thick,h,color);
}
int sok_text_width(const char *text,int scale)
{
    int w=0;for(;*text;text++) {unsigned c=(unsigned char)*text;if(c<32 || c>126)c='?';w+=(glyph_width[c-32]+1)*scale;}
    return w ? w-scale:0;
}
static void text_ratio(int x,int y,const char *s,int color,int numerator,int denominator)
{
    int advance=0;
    for(;*s;s++) {
        unsigned c=(unsigned char)*s;if(c<32 || c>126)c='?';c-=32;
        for(int row=0;row<11;row++)for(int col=0;col<glyph_width[c];col++)
            if(glyph_rows[c][row]&(1u<<col)) {
                int x1=(advance+col)*numerator/denominator;
                int x2=(advance+col+1)*numerator/denominator;
                int y1=row*numerator/denominator,y2=(row+1)*numerator/denominator;
                rect(x+x1,y+y1,x2-x1,y2-y1,color);
            }
        advance+=glyph_width[c]+1;
    }
}
static void text(int x,int y,const char *s,int color,int scale)
{text_ratio(x,y,s,color,scale,1);}
static void centered(int x,int y,int w,const char *s,int color,int scale)
{text(x+(w-sok_text_width(s,scale))/2,y,s,color,scale);}
static void title(const char *s,const char *right)
{
    rect(0,0,396,24,INK);text(8,6,s,C_WHITE,1);
    if(right)text(388-sok_text_width(right,1),6,right,C_RGB(25,28,28),1);
}
static void softkeys(bool play)
{
    const char *labels[6]={"","","","","","OPEN"};
    if(play){labels[0]="INIT";labels[1]="UNDO";labels[4]="LEVEL-";labels[5]="LEVEL+";}
    rect(0,SOK_SOFTKEY_TOP,396,20,C_WHITE);
    for(int i=0;i<6;i++) {
        if(!labels[i][0])continue;
        int bg=i==0 && play ? 0xffe0:i==1 && play ? 0xf81f:INK;
        rect(i*66+1,205,64,18,bg);
        centered(i*66+1,209,64,labels[i],play && i<2 ? C_BLACK:C_WHITE,1);
    }
}
static void main_screen(const SokApp *app)
{
    title("SOKOBAN","60 LEVELS");
    for(unsigned i=0;i<4;i++) {
        int x=7+(int)(i%2)*195,y=31+(int)(i/2)*77;
        rect(x,y,187,70,pale[i]);border(x,y,187,70,i==app->group ? INK:LINE,i==app->group ? 3:1);
        char n[2]={(char)('1'+i),0};text(x+171,y+7,n,group_color[i],1);
        text_ratio(x+(187-sok_text_width(names[i],1)*3/2)/2,y+28,names[i],INK,3,2);
    }
    text(9,190,"ARROWS: SELECT     MENU: CASIO MAIN MENU",MUTED,1);
    softkeys(false);
}
static void level_screen(const SokApp *app)
{
    char range[16];snprintf(range,sizeof(range),"%u - %u",app->group*15+1,app->group*15+15);
    title(names[app->group],range);
    for(unsigned i=0;i<15;i++) {
        unsigned id=app->group*15+i;
        int x=8+(int)(i%5)*78,y=31+(int)(i/5)*54;
        bool clear=app->progress.cleared[id]!=0,select=i==app->selection;
        rect(x,y,68,45,clear ? group_color[app->group]:C_WHITE);
        border(x,y,68,45,LINE,1);
        if(select){border(x-3,y-3,74,51,INK,2);}
        char number[4];snprintf(number,sizeof(number),"%u",id+1);
        centered(x,y+12,68,number,clear ? C_WHITE:group_color[app->group],2);
    }
    text(9,193,"EXE: OPEN    EXIT: GROUPS",MUTED,1);softkeys(false);
}
SokBoardLayout sok_board_layout(const SokMap *map)
{
    int t=SOK_BOARD_WIDTH/map->width,h=SOK_PLAY_HEIGHT/map->height;if(h<t)t=h;
    SokBoardLayout l={SOK_BOARD_LEFT+(SOK_BOARD_WIDTH-map->width*t)/2,
        SOK_PLAY_TOP+(SOK_PLAY_HEIGHT-map->height*t)/2,t,map->width*t,map->height*t};
    return l;
}
static void cell(const SokApp *app,const SokMap *map,unsigned p,int x,int y,int s)
{
    unsigned terrain=sok_map_terrain(map,(uint16_t)p);
    if(terrain==SOK_VOID)return;
    if(terrain==SOK_WALL) {
        rect(x,y,s,s,walls[(app->level-1)/15]);
        rect(x,y,s,1,pale[(app->level-1)/15]);
        rect(x,y,1,s,pale[(app->level-1)/15]);
        rect(x+s-1,y,1,s,group_color[(app->level-1)/15]);
        rect(x,y+s-1,s,1,group_color[(app->level-1)/15]);return;
    }
    rect(x,y,s,s,C_RGB(30,29,26));
    if(terrain==SOK_GOAL) {
        int g=s>=12 ? 3:2;
        rect(x+(s-g)/2,y+(s-g)/2,g,g,C_RGB(9,7,5));
    }
    if(sok_crate_at(map,&app->game,(uint16_t)p)>=0) {
        rect(x+1,y+1,s-2,s-2,ORANGE);border(x+1,y+1,s-2,s-2,C_RGB(16,8,1),1);
        if(terrain==SOK_GOAL) {
            int g=s>=12 ? 3:2;rect(x+(s-g)/2,y+(s-g)/2,g,g,C_WHITE);
        } else {
            for(int d=2;d<s-2;d++)dpixel(x+d,y+d,C_RGB(21,11,1));
        }
    }
    if(app->game.player==p) {
        int head=s>=12 ? 3:2,body=s>=12 ? 5:3;
        rect(x+(s-head)/2,y+1,head,head,C_BLACK);
        rect(x+(s-body)/2,y+head+2,body,s-head-3,C_BLACK);
    }
}
static void stat(int y,const char *label,const char *value)
{
    int labelw=sok_text_width(label,1),valuew=sok_text_width(value,1);
    text(8,y,label,MUTED,1);
    if(labelw+4+valuew<=106)text(8+labelw+4,y,value,INK,1);
    else text(114-valuew,y+13,value,INK,1);
}
static void play_screen(const SokApp *app)
{
    const SokMap *map=sok_get_map(app->level);
    rect(6,7,110,35,pale[(app->level-1)/15]);border(6,7,110,35,group_color[(app->level-1)/15],2);
    char label[32];snprintf(label,sizeof(label),"LEVEL %u",app->level);centered(6,18,110,label,INK,1);
    snprintf(label,sizeof(label),"%u/%u",sok_on_goals(map,&app->game),map->crate_count);stat(56,"CRATES :",label);
    snprintf(label,sizeof(label),"%lu",(unsigned long)app->game.moves);stat(89,"MOVES :",label);
    snprintf(label,sizeof(label),"%lu",(unsigned long)app->game.pushes);stat(122,"PUSHES :",label);
    snprintf(label,sizeof(label),"UNDO %u/5",app->game.undo_count);text(9,161,label,MUTED,1);
    rect(120,SOK_PLAY_TOP,1,SOK_PLAY_HEIGHT,LINE);
    SokBoardLayout l=sok_board_layout(map);
    for(unsigned p=0;p<(unsigned)map->width*map->height;p++)
        cell(app,map,p,l.x+(int)(p%map->width)*l.tile,l.y+(int)(p/map->width)*l.tile,l.tile);
    softkeys(true);
}
static void modal(const SokApp *app)
{
    int w=244,h=88;
    if(app->modal==SM_SAVE_ERROR || app->modal==SM_LOAD_NOTICE)h=103;
    int x=(396-w)/2;
    int y=app->screen==SOK_PLAY ? SOK_PLAY_TOP+(SOK_PLAY_HEIGHT-h)/2:(224-h)/2;
    rect(x+4,y+4,w,h,C_RGB(12,14,14));rect(x,y,w,h,C_WHITE);border(x,y,w,h,INK,2);
    rect(x+2,y+2,w-4,5,app->modal==SM_SAVE_ERROR ? C_RGB(27,5,5):C_RGB(8,19,14));
    const char *heading="",*a="",*b="",*c=NULL;
    switch(app->modal) {
    case SM_INIT:heading="YOU SURE?";a="EXE: YES";b="EXIT: NO";break;
    case SM_WIN:heading="Congratulations!";
        a=app->level==60 ? "EXE: LEVEL MENU":"EXE: NEXT LEVEL";b="EXIT: LEVEL MENU";break;
    case SM_SAVE_ERROR:heading="SAVE FAILED";a="EXE: RETRY";b="F6: WITHOUT SAVING";c="EXIT: STAY";break;
    case SM_LOAD_NOTICE:heading=app->recovered_notice ? "BACKUP RECOVERED":"SAVE UNAVAILABLE";
        a=app->recovered_notice ? "Using the last valid save.":"Starting with fresh progress.";
        b="EXE / EXIT: CONTINUE";break;
    default:break;
    }
    centered(x,y+17,w,heading,INK,1);text(x+22,y+43,a,INK,1);text(x+22,y+60,b,INK,1);
    if(c)text(x+22,y+77,c,INK,1);
}
void sok_render(const SokApp *app)
{
    dclear(PAPER);
    if(app->screen==SOK_MAIN)main_screen(app);
    else if(app->screen==SOK_LEVELS)level_screen(app);
    else play_screen(app);
    if(app->modal!=SM_NONE)modal(app);
    dupdate();
}
