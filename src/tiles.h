/* tiles.h -- QBERT NEO tile data */
#ifndef TILES_H
#define TILES_H
#include "ngpc.h"
#include "library.h"

/* Sprite tiles (must stay < 256) */
#define T_QB_DL  144   /* 16x16, 4 tiles  */
#define T_QB_DR  148
#define T_QB_UL  152
#define T_QB_UR  156
#define T_BALL   160   /* 16x16, 4 tiles  */
#define T_COILY  164   /* 16x24, 6 tiles  */
#define T_BUBBLE 170   /* 32x16, 8 tiles  */
#define T_DISC   178   /* 16x8, 2 tiles   */
#define T_SPRTXT 208   /* runtime sprite-text glyphs, 48 tile slots */
/* BG tiles */
#define T_CUBE   200   /* +0/1 top open, +2/3 top filled, +4/5 body, +6/7 cap */
#define T_CAN    340   /* 24x32, 12 tiles */
#define T_AFONT  352   /* 38 arcade glyphs x 4 tiles = 152 (IDs 352-503) */

void install_tiles(void);
extern const u16 qb_dl[4][8];
extern const u16 qb_dr[4][8];
extern const u16 qb_ul[4][8];
extern const u16 qb_ur[4][8];
extern const u16 ball_s[4][8];
extern const u16 coily_s[6][8];
extern const u16 bubble_s[8][8];
extern const u16 can_t[12][8];
extern const u16 afont[152][8];
extern const u16 cube_open[2][8];
extern const u16 cube_fill[2][8];
extern const u16 cube_body[2][8];
extern const u16 cube_cap[2][8];
extern const u16 disc_s[2][8];

#endif
