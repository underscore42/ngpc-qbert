/* game.h — QBERT NEO: shared types, constants, state */
#ifndef GAME_H
#define GAME_H

#include "ngpc.h"
#include "library.h"

/* ---- Game states ---- */
#define STATE_TITLE  0
#define STATE_GAME   1
#define STATE_BONUS  2
#define STATE_OVER   3
#define STATE_ENTRY  4
#define STATE_SCORES 5

/* ---- BG palettes ---- */
#define PAL_TEXT     0
#define PAL_CUBE_OFF 1
#define PAL_CUBE_MID 2
#define PAL_CUBE_ON  3
#define PAL_AFONT    4
#define PAL_CAN      5

/* ---- Sprite palettes ---- */
#define SPAL_Q      1
#define SPAL_BALL   2
#define SPAL_COILY  3
#define SPAL_BUBBLE 4
#define SPAL_BIGTXT 5
#define SPAL_DISC   6

/* ---- Sounds (1-based for PlaySound) ---- */
#define SND_HOP   1
#define SND_LIT   2
#define SND_FALL  3
#define SND_DEATH 4
#define SND_CLEAR 5
#define SND_EHOP  6
#define SND_HOP2  7
#define SND_HOP3  8
#define SND_DISC  9
#define SND_LURE  10

/* ---- Q*Bert states ---- */
#define Q_IDLE 0
#define Q_HOP  1
#define Q_FALL 2
#define Q_RIDE 3

/* ---- Poses ---- */
#define POSE_DL 0
#define POSE_DR 1
#define POSE_UL 2
#define POSE_UR 3

/* ---- Enemies ---- */
#define MAX_ENEMIES 3
#define E_NONE  0
#define E_BALL  1
#define E_EGG   2
#define E_COILY 3

#define HOP_FRAMES 8
#define NUM_SCHEMES 9

/* ---- Sprite slot layout ---- */
/* Q*Bert 16x16: sprites 0-3; enemy slot i: 4+6*i (6 each); bubble: 22-29;
 * discs: 30-33; pause sprite-text: 34+ */
#define SPR_Q      0
#define SPR_ENEMY  4
#define SPR_BUBBLE 22
#define SPR_DISC   30
#define SPR_PAUSE  34

/* ---- Shared state (defined in game.c) ---- */
extern u8  state, skip;
extern u8  pad_cur, pad_prev, pad_press;
extern u8  rand_seed;
extern u8  mello_mode;

extern u8  cube_st[7][7];
extern u8  lit_count;
extern u16 score;
extern u8  lives;
extern u8  level;
extern u16 bonus_amt;
extern u8  bonus_timer;

extern u8  q_state;
extern u8  q_r, q_c;
extern s16 q_px, q_py;
extern u8  q_pose;

extern u8  e_type[MAX_ENEMIES];

/* ---- game.c API ---- */
u8   cheap_rand(u8 max);
u16  cube_x(u8 r, u8 c);
u16  cube_y(u8 r);
void game_start(void);
void start_level(void);
void game_update(void);
void bonus_update(void);
void clear_sprites(void);

/* ---- High score table (save.c) ---- */
extern u16 hs_score[5];
extern u8  hs_init[5][3];

#endif
