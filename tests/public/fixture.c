/* Original MIT-licensed illustration fixture, NOT an upstream puzzle.
   This map is linked only into capture_public, never into the calculator app. */
#include "maps.h"
#include <stddef.h>
static const uint8_t terrain[] = {
0,0,0,0,85,85,5,144,170,106,0,185,170,6,144,150,110,0,169,170,6,144,106,110,0,233,170,6,144,170,106,0,85,85,5,0,0,0,0,0,0,0
};
static const uint16_t crates[]={47,49,88,105};
const SokMap sok_maps[SOK_LEVEL_COUNT]={
    {14,12,4,78,terrain,crates},
    {14,12,4,78,terrain,crates},
    {14,12,4,78,terrain,crates},
    {14,12,4,78,terrain,crates},
    {14,12,4,78,terrain,crates},
    {14,12,4,78,terrain,crates},
    {14,12,4,78,terrain,crates},
    {14,12,4,78,terrain,crates},
    {14,12,4,78,terrain,crates},
    {14,12,4,78,terrain,crates},
    {14,12,4,78,terrain,crates},
    {14,12,4,78,terrain,crates},
    {14,12,4,78,terrain,crates},
    {14,12,4,78,terrain,crates},
    {14,12,4,78,terrain,crates},
    {14,12,4,78,terrain,crates},
    {14,12,4,78,terrain,crates},
    {14,12,4,78,terrain,crates},
    {14,12,4,78,terrain,crates},
    {14,12,4,78,terrain,crates},
    {14,12,4,78,terrain,crates},
    {14,12,4,78,terrain,crates},
    {14,12,4,78,terrain,crates},
    {14,12,4,78,terrain,crates},
    {14,12,4,78,terrain,crates},
    {14,12,4,78,terrain,crates},
    {14,12,4,78,terrain,crates},
    {14,12,4,78,terrain,crates},
    {14,12,4,78,terrain,crates},
    {14,12,4,78,terrain,crates},
    {14,12,4,78,terrain,crates},
    {14,12,4,78,terrain,crates},
    {14,12,4,78,terrain,crates},
    {14,12,4,78,terrain,crates},
    {14,12,4,78,terrain,crates},
    {14,12,4,78,terrain,crates},
    {14,12,4,78,terrain,crates},
    {14,12,4,78,terrain,crates},
    {14,12,4,78,terrain,crates},
    {14,12,4,78,terrain,crates},
    {14,12,4,78,terrain,crates},
    {14,12,4,78,terrain,crates},
    {14,12,4,78,terrain,crates},
    {14,12,4,78,terrain,crates},
    {14,12,4,78,terrain,crates},
    {14,12,4,78,terrain,crates},
    {14,12,4,78,terrain,crates},
    {14,12,4,78,terrain,crates},
    {14,12,4,78,terrain,crates},
    {14,12,4,78,terrain,crates},
    {14,12,4,78,terrain,crates},
    {14,12,4,78,terrain,crates},
    {14,12,4,78,terrain,crates},
    {14,12,4,78,terrain,crates},
    {14,12,4,78,terrain,crates},
    {14,12,4,78,terrain,crates},
    {14,12,4,78,terrain,crates},
    {14,12,4,78,terrain,crates},
    {14,12,4,78,terrain,crates},
    {14,12,4,78,terrain,crates},
};
const uint8_t sok_map_pack_hash[32]={0};
uint8_t sok_map_terrain(const SokMap *map,uint16_t cell)
{
    if(!map || cell >= (unsigned)map->width*map->height)return SOK_VOID;
    return (uint8_t)((map->terrain[cell/4]>>(2u*(cell%4)))&3u);
}
const SokMap *sok_get_map(unsigned id)
{return id>=1 && id<=60 ? &sok_maps[id-1]:NULL;}
