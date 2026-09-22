#ifndef SOK_RENDER_H
#define SOK_RENDER_H
#include "app.h"
enum {
    SOK_PLAY_TOP=4, SOK_PLAY_HEIGHT=196,
    SOK_BOARD_LEFT=124, SOK_BOARD_WIDTH=268,
    SOK_SOFTKEY_TOP=204
};
typedef struct {int x,y,tile,width,height;} SokBoardLayout;
SokBoardLayout sok_board_layout(const SokMap *map);
int sok_text_width(const char *text,int scale);
void sok_draw_player(int x,int y,int size);
void sok_render(const SokApp *app);
#endif
