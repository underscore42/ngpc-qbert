/* sound.c — QBERT NEO sound effects */
#include "sound.h"

static SOUNDEFFECT game_sounds[6] = {
    /* 1 hop:   quick rising blip */
    { 1, 3, 0, 0x0140, 0x0020, 1, 0, 0x00C0, 0x01C0, 13, 3, 1, 0, 0, 15 },
    /* 2 lit:   bright chirp */
    { 0, 3, 0, 0x00C0, 0x0018, 1, 1, 0x0090, 0x0110, 14, 2, 1, 0, 0, 15 },
    /* 3 fall:  long descending whistle */
    { 2, 12, 0, 0x0080, 0x0010, 1, 0, 0x0060, 0x0240, 15, 1, 4, 0, 0, 15 },
    /* 4 death: harsh low buzz */
    { 2, 8, 0, 0x0300, 0x0018, 1, 0, 0x0200, 0x03C0, 15, 1, 2, 0, 0, 15 },
    /* 5 clear: sweeping fanfare */
    { 0, 10, 0, 0x0200, 0x0028, 1, 1, 0x00A0, 0x0240, 14, 1, 3, 1, 6, 15 },
    /* 6 enemy hop: dull low blip */
    { 1, 2, 0, 0x0240, 0x0020, 1, 0, 0x01C0, 0x02C0, 10, 3, 1, 0, 0, 15 }
};

void sound_init(void) {
    InstallSoundDriver();
    WaitVsync();
    WaitVsync();
    InstallSounds(game_sounds, 6);
}
