/* sound.c — QBERT NEO sound effects */
#include "sound.h"

static SOUNDEFFECT game_sounds[10] = {
    /* 1 hop A: quick rising blip */
    { 1, 3, 0, 0x0140, 0x0020, 1, 0, 0x00C0, 0x01C0, 13, 3, 1, 0, 0, 15 },
    /* 2 lit:   bright chirp */
    { 0, 3, 0, 0x00C0, 0x0018, 1, 1, 0x0090, 0x0110, 14, 2, 1, 0, 0, 15 },
    /* 3 fall:  long descending whistle */
    { 2, 12, 0, 0x0080, 0x0010, 1, 0, 0x0060, 0x0240, 15, 1, 4, 0, 0, 15 },
    /* 4 death: warbling garble (the printable swear) */
    { 2, 10, 1, 0x0280, 0x0060, 1, 1, 0x01C0, 0x0340, 15, 2, 1, 1, 6, 15 },
    /* 5 clear: sweeping fanfare */
    { 0, 10, 0, 0x0200, 0x0028, 1, 1, 0x00A0, 0x0240, 14, 1, 3, 1, 6, 15 },
    /* 6 enemy hop: dull low blip */
    { 1, 2, 0, 0x0240, 0x0020, 1, 0, 0x01C0, 0x02C0, 10, 3, 1, 0, 0, 15 },
    /* 7 hop B: slightly lower */
    { 1, 3, 0, 0x0170, 0x0020, 1, 0, 0x00D0, 0x0200, 13, 3, 1, 0, 0, 15 },
    /* 8 hop C: slightly higher */
    { 1, 3, 0, 0x0110, 0x0020, 1, 0, 0x00A0, 0x0190, 13, 3, 1, 0, 0, 15 },
    /* 9 disc:  long rising glide */
    { 0, 14, 0, 0x0260, 0x0014, 1, 0, 0x0080, 0x02A0, 13, 1, 5, 0, 0, 15 },
    /* 10 lure: long descending taunt for a falling Coily */
    { 2, 14, 0, 0x00A0, 0x0014, 1, 0, 0x0080, 0x02C0, 15, 1, 4, 0, 0, 15 }
};

void sound_init(void) {
    InstallSoundDriver();
    WaitVsync();
    WaitVsync();
    InstallSounds(game_sounds, 10);
}
