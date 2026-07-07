/* save.c — QBERT NEO flash high score save
 * Hardware-validated config: rbc3=2, u16[256] buffer, offset within ROM.
 * Layout: [0]=magic, then 5 entries of 4 words (score, i0, i1, i2). */
#include "save.h"

#define SAVE_MAGIC 0x51B7

u16 hs_score[5];
u8  hs_init[5][3];

static u16 save_buf[256];

static void default_table(void) {
    u8 i, j;
    static const u16 ds[5] = { 2000, 1500, 1000, 750, 500 };
    static const u8 di[5][3] = {
        {'S','N','K'}, {'Q','B','T'}, {'C','O','Y'}, {'U','G','G'}, {'S','A','M'}
    };
    for (i = 0; i < 5; i++) {
        hs_score[i] = ds[i];
        for (j = 0; j < 3; j++) hs_init[i][j] = di[i][j];
    }
}

void load_high_scores(void) {
    u8 i, j, w;
    GetSavedData((void*)save_buf);
    if (save_buf[0] == SAVE_MAGIC) {
        w = 1;
        for (i = 0; i < 5; i++) {
            hs_score[i] = save_buf[w]; w = w + 1;
            for (j = 0; j < 3; j++) { hs_init[i][j] = (u8)save_buf[w]; w = w + 1; }
        }
    } else {
        default_table();
    }
}

void save_high_scores(void) {
    u8 i, j, w;
    u16 k;
    for (k = 0; k < 256; k++) save_buf[k] = 0;
    save_buf[0] = SAVE_MAGIC;
    w = 1;
    for (i = 0; i < 5; i++) {
        save_buf[w] = hs_score[i]; w = w + 1;
        for (j = 0; j < 3; j++) { save_buf[w] = (u16)hs_init[i][j]; w = w + 1; }
    }
    Flash((void*)save_buf);
}

u8 score_rank(u16 s) {
    u8 i;
    for (i = 0; i < 5; i++) {
        if (s > hs_score[i]) return i;
    }
    return 5;
}

void insert_high_score(u8 slot, u16 s, const u8 letters[3]) {
    u8 i, j;
    i = 4;
    while (i > slot) {
        hs_score[i] = hs_score[i-1];
        for (j = 0; j < 3; j++) hs_init[i][j] = hs_init[i-1][j];
        i = i - 1;
    }
    hs_score[slot] = s;
    for (j = 0; j < 3; j++) hs_init[slot][j] = letters[j];
}
