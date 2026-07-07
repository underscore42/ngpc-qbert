/* main.c — QBERT NEO
 * Studio So Not Kansai — entry point and state machine.
 * Konami code on the title unlocks MELLO MODE (promo palette + 7-Eleven
 * intermissions), a nod to the Mello Yello arcade promo units.
 */

#define CARTHDR_IMPL
#include "carthdr.h"
#include "game.h"
#include "screen.h"
#include "tiles.h"
#include "sound.h"
#include "save.h"

static u8 paused;
static u8 pause_n;

/* Initials entry state */
static u8 entry_letters[3];
static u8 entry_cur;
static u8 entry_slot;

/* ---- Konami code: Up Up Down Down Left Right Left Right B A ---- */
static u8 konami_pos;
static const u8 konami_seq[10] = {
    0x01, 0x01, 0x02, 0x02, 0x04, 0x08, 0x04, 0x08, 0x20, 0x10
};

static void check_konami(void) {
    u8 expected;
    if (pad_press == 0) return;
    expected = konami_seq[konami_pos];
    if (pad_press & expected) {
        konami_pos = konami_pos + 1;
        if (konami_pos >= 10) {
            mello_mode = mello_mode ^ 1;
            konami_pos = 0;
            PlaySound(SND_CLEAR);
            draw_title();
        }
    } else {
        konami_pos = 0;
    }
}

static void begin_entry(void) {
    entry_letters[0] = 'A';
    entry_letters[1] = 'A';
    entry_letters[2] = 'A';
    entry_cur = 0;
    clear_sprites();
    draw_entry(entry_slot, entry_letters, entry_cur);
    state = STATE_ENTRY;
    skip = 10;
}

static void entry_update(void) {
    u8 ch;
    if (pad_press & J_UP) {
        ch = entry_letters[entry_cur];
        if (ch >= 'Z') ch = 'A';
        else ch = ch + 1;
        entry_letters[entry_cur] = ch;
        PlaySound(SND_HOP);
        draw_entry(entry_slot, entry_letters, entry_cur);
    }
    if (pad_press & J_DOWN) {
        ch = entry_letters[entry_cur];
        if (ch <= 'A') ch = 'Z';
        else ch = ch - 1;
        entry_letters[entry_cur] = ch;
        PlaySound(SND_HOP);
        draw_entry(entry_slot, entry_letters, entry_cur);
    }
    if (pad_press & J_LEFT) {
        if (entry_cur > 0) {
            entry_cur = entry_cur - 1;
            draw_entry(entry_slot, entry_letters, entry_cur);
        }
    }
    if (pad_press & J_RIGHT) {
        if (entry_cur < 2) {
            entry_cur = entry_cur + 1;
            draw_entry(entry_slot, entry_letters, entry_cur);
        }
    }
    if (pad_press & J_A) {
        if (entry_cur < 2) {
            entry_cur = entry_cur + 1;
            draw_entry(entry_slot, entry_letters, entry_cur);
        } else {
            insert_high_score(entry_slot, score, entry_letters);
            save_high_scores();
            PlaySound(SND_LIT);
            draw_scores();
            state = STATE_SCORES;
            skip = 10;
        }
    }
}

void main(void) {
    InitNGPC();
    SysSetSystemFont();
    install_tiles();
    setup_palettes();
    sound_init();
    load_high_scores();

    mello_mode = 0;
    konami_pos = 0;
    rand_seed = 42;
    paused = 0;

    state = STATE_TITLE;
    skip = 10;
    pad_cur = 0; pad_prev = 0;
    draw_title();

    while (1) {
        WaitVsync();
        pad_prev = pad_cur;
        pad_cur = JOYPAD & 0x7F;
        pad_press = pad_cur & (u8)(~pad_prev);
        if (skip > 0) { skip = skip - 1; continue; }

        if (state == STATE_TITLE) {
            rand_seed = rand_seed + VBCounter;
            check_konami();
            if (pad_press & J_A) {
                clear_sprites();
                state = STATE_GAME;
                skip = 10;
                paused = 0;
                game_start();
            }
            if (pad_press & J_OPTION) {
                clear_sprites();
                draw_scores();
                state = STATE_SCORES;
                skip = 10;
            }
        } else if (state == STATE_GAME) {
            if (pad_press & J_OPTION) {
                paused = paused ^ 1;
                if (paused) {
                    pause_n = sprite_text(SPR_PAUSE, 32, 64, "PAUSED");
                } else {
                    hide_sprite_text(SPR_PAUSE, pause_n);
                }
            }
            if (!paused) game_update();
        } else if (state == STATE_BONUS) {
            bonus_update();
        } else if (state == STATE_OVER) {
            if (pad_press & J_A) {
                entry_slot = score_rank(score);
                if (entry_slot < 5) {
                    begin_entry();
                } else {
                    clear_sprites();
                    state = STATE_GAME;
                    skip = 10;
                    game_start();
                }
            }
            if (pad_press & J_OPTION) {
                clear_sprites();
                state = STATE_TITLE;
                skip = 10;
                draw_title();
            }
        } else if (state == STATE_ENTRY) {
            entry_update();
        } else if (state == STATE_SCORES) {
            if (pad_press & J_A) {
                clear_sprites();
                state = STATE_GAME;
                skip = 10;
                game_start();
            }
            if (pad_press & J_OPTION) {
                clear_sprites();
                state = STATE_TITLE;
                skip = 10;
                draw_title();
            }
        }
    }
}

/* EOF */
