/* save.h — QBERT NEO flash high score save */
#ifndef SAVE_H
#define SAVE_H
#include "game.h"
void load_high_scores(void);
void save_high_scores(void);
u8   score_rank(u16 s);   /* 0-4 = table slot, 5 = didn't qualify */
void insert_high_score(u8 slot, u16 s, const u8 letters[3]);
#endif
