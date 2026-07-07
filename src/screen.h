/* screen.h — QBERT NEO drawing */
#ifndef SCREEN_H
#define SCREEN_H

#include "game.h"

void setup_palettes(void);
void flash_target_palette(u8 bright);
void arc_print(u8 x, u8 y, const char *s);
void arc_char(u8 x, u8 y, char ch);
u8 sprite_text(u8 base, u8 x, u8 y, const char *str);
void hide_sprite_text(u8 base, u8 n);
void draw_cube(u8 r, u8 c);
void draw_cube_top(u8 r, u8 c);
void draw_pyramid(void);
void draw_hud(void);
void draw_score_hud(void);
void fresh_screen(void);
void draw_title(void);
const char *hs_init_str(u8 i);
void draw_can(u8 tx, u8 ty);
void draw_bonus_screen(void);
void draw_scores(void);
void draw_entry(u8 slot, const u8 letters[3], u8 cur);

#endif
