/* main.c — QBERT NEO
 * Studio So Not Kansai — one-shot build for NGPC
 * Isometric pyramid hopper. Light all 28 cubes, dodge the balls,
 * and never let Coily catch you.
 *
 * Controls (d-pad rotated 45 degrees, classic port style):
 *   UP    = hop up-right      DOWN  = hop down-left
 *   LEFT  = hop up-left       RIGHT = hop down-right
 *   (diagonal presses also work: UP+LEFT = up-left, etc.)
 *   OPTION = pause
 * Konami code on title toggles retro green mode.
 *
 * Framework: ameliandev ngpc-project-template (library.c/h, ngpc.h)
 * Strict cc900 C89: no signed mul/div, no decls after statements,
 * no const SOUNDEFFECT, JOYPAD active-high, tiles installed after
 * SysSetSystemFont, sprite tile IDs < 256.
 */

#define CARTHDR_IMPL
#include "carthdr.h"
#include "ngpc.h"
#include "library.h"

/* ================= Constants ================= */

#define STATE_TITLE 0
#define STATE_GAME  1
#define STATE_OVER  2

/* BG palettes */
#define PAL_TEXT     0
#define PAL_CUBE_OFF 1
#define PAL_CUBE_ON  2

/* Sprite palettes */
#define SPAL_Q     1
#define SPAL_BALL  2
#define SPAL_COILY 3

/* Tile IDs (after system font, sprite IDs < 256) */
#define T_CUBE  144
#define T_QR    160
#define T_QL    164
#define T_BALL  168
#define T_COILY 172

/* Sounds (1-based for PlaySound) */
#define SND_HOP   1
#define SND_LIT   2
#define SND_FALL  3
#define SND_DEATH 4
#define SND_CLEAR 5
#define SND_EHOP  6

/* Q*Bert states */
#define Q_IDLE 0
#define Q_HOP  1
#define Q_FALL 2

/* Enemy slots */
#define MAX_ENEMIES 3
#define E_NONE  0
#define E_BALL  1
#define E_EGG   2   /* purple ball, becomes Coily at bottom row */
#define E_COILY 3

#define HOP_FRAMES 8

/* ================= Tile data ================= */

static const u16 cube_t[4][8] = {
    { 0x0005, 0x0055, 0x0555, 0x5555, 0xA555, 0xAA55, 0xAAA5, 0xAAAA },
    { 0x5000, 0x5500, 0x5550, 0x5555, 0x555F, 0x55FF, 0x5FFF, 0xFFFF },
    { 0xAAAA, 0xAAAA, 0xAAAA, 0xAAAA, 0xAAAA, 0xAAAA, 0xAAAA, 0xAAAA },
    { 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF },
};

static const u16 qbert_r[4][8] = {
    { 0x0000, 0x0055, 0x0155, 0x0569, 0x056D, 0x1569, 0x1555, 0x1555 },
    { 0x0000, 0x5000, 0x5400, 0x6900, 0x6D00, 0x6940, 0x5554, 0x55FC },
    { 0x1555, 0x1555, 0x0555, 0x0555, 0x0155, 0x0141, 0x0F03, 0x3F0F },
    { 0x5570, 0x5540, 0x5500, 0x5400, 0x5000, 0x4000, 0xC000, 0xC000 },
};

static const u16 qbert_l[4][8] = {
    { 0x0000, 0x0005, 0x0015, 0x0069, 0x0079, 0x0169, 0x1555, 0x3F55 },
    { 0x0000, 0x5500, 0x5540, 0x6950, 0x7950, 0x6954, 0x5554, 0x5554 },
    { 0x0D55, 0x0155, 0x0055, 0x0015, 0x0005, 0x0001, 0x0003, 0x0003 },
    { 0x5554, 0x5554, 0x5550, 0x5550, 0x5540, 0x4140, 0xC0F0, 0xF0FC },
};

static const u16 ball_t[4][8] = {
    { 0x0000, 0x0000, 0x0000, 0x0000, 0x0015, 0x005A, 0x0169, 0x0165 },
    { 0x0000, 0x0000, 0x0000, 0x0000, 0x5400, 0x5500, 0x5540, 0x5540 },
    { 0x0155, 0x0155, 0x0155, 0x0055, 0x0015, 0x0000, 0x0000, 0x0000 },
    { 0x5540, 0x55C0, 0x57C0, 0x5F00, 0x7C00, 0x0000, 0x0000, 0x0000 },
};

static const u16 coily_t[4][8] = {
    { 0x0050, 0x00A5, 0x00B5, 0x0055, 0x0015, 0x0005, 0x0015, 0x0055 },
    { 0x5000, 0xA400, 0xB400, 0x5400, 0x5000, 0x4000, 0x5000, 0x5400 },
    { 0x0015, 0x0055, 0x0155, 0x0055, 0x0155, 0x0555, 0x0155, 0x0555 },
    { 0x5000, 0x5400, 0x5500, 0x5400, 0x5500, 0x5540, 0x5500, 0x5540 },
};

/* Hop arc: vertical lift subtracted from Y during the 8-frame hop */
static const u8 hop_arc[HOP_FRAMES] = { 4, 7, 9, 10, 10, 9, 7, 4 };

/* ================= Sound ================= */

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

/* ================= Game state ================= */

static u8 state, skip;
static u8 pad_cur, pad_prev, pad_press;
static u8 rand_seed;
static u8 retro_mode;

static u8 cube_lit[7][7];
static u8 lit_count;

static u16 score;
static u16 hi_score;
static u8  lives;
static u8  level;

/* Q*Bert */
static u8  q_state;
static u8  q_r, q_c;         /* grid position (valid when idle)  */
static s16 q_px, q_py;       /* pixel position (top-left, 16x16) */
static s8  q_sx, q_sy;       /* per-frame hop steps              */
static u8  q_hf;             /* hop frame 0..7                   */
static u8  q_face;           /* 0=left 1=right                   */
static s8  q_nr, q_nc;       /* hop target grid (may be off)     */
static u8  q_valid;          /* target on pyramid?               */
static u8  death_timer;      /* freeze frames after death        */
static u8  death_fell;       /* 1 = fell off (no balloon)        */

/* Enemies */
static u8  e_type[MAX_ENEMIES];
static u8  e_r[MAX_ENEMIES], e_c[MAX_ENEMIES];
static s16 e_px[MAX_ENEMIES], e_py[MAX_ENEMIES];
static s8  e_sx[MAX_ENEMIES], e_sy[MAX_ENEMIES];
static u8  e_hop[MAX_ENEMIES];   /* 0 = grounded, else frames left */
static u8  e_wait[MAX_ENEMIES];  /* frames until next hop          */
static u8  e_fall[MAX_ENEMIES];  /* 1 = falling off the pyramid    */
static s8  e_nr[MAX_ENEMIES], e_nc[MAX_ENEMIES];
static u8  coily_alive;
static u8  spawn_timer;

/* Konami code */
static u8 konami_pos;
static const u8 konami_seq[10] = {
    0x01, 0x01, 0x02, 0x02, 0x04, 0x08, 0x04, 0x08, 0x20, 0x10
};

/* ================= Utilities ================= */

static u8 cheap_rand(u8 max) {
    u16 v;
    rand_seed = (u8)(rand_seed * 13 + 7);
    v = rand_seed;
    return (u8)(v % max);
}

/* Cube (r,c) top-left pixel — only for valid on-pyramid coords */
static u16 cube_x(u8 r, u8 c) {
    u16 x;
    x = 72;
    x = x - ((u16)r << 3);
    x = x + ((u16)c << 4);
    return x;
}

static u16 cube_y(u8 r) {
    u16 y;
    y = 16 + ((u16)r << 4);
    return y;
}

static u8 on_board(s8 r, s8 c) {
    if (r < 0) return 0;
    if (r > 6) return 0;
    if (c < 0) return 0;
    if (c > r) return 0;
    return 1;
}

/* ================= Tiles / palettes ================= */

static void install_tiles(void) {
    InstallTileSetAt((const unsigned short (*)[8])cube_t,  32, T_CUBE);
    InstallTileSetAt((const unsigned short (*)[8])qbert_r, 32, T_QR);
    InstallTileSetAt((const unsigned short (*)[8])qbert_l, 32, T_QL);
    InstallTileSetAt((const unsigned short (*)[8])ball_t,  32, T_BALL);
    InstallTileSetAt((const unsigned short (*)[8])coily_t, 32, T_COILY);
}

static void setup_palettes(void) {
    if (retro_mode) {
        SetBackgroundColour(RGB(0, 1, 0));
        SetPalette(SCR_1_PLANE, PAL_TEXT,     0, RGB(0,15,0), RGB(0,13,0), RGB(0,15,0));
        SetPalette(SCR_1_PLANE, PAL_CUBE_OFF, 0, RGB(0,6,0),  RGB(0,10,0), RGB(0,4,0));
        SetPalette(SCR_1_PLANE, PAL_CUBE_ON,  0, RGB(0,15,0), RGB(0,10,0), RGB(0,4,0));
        SetPalette(SPRITE_PLANE, SPAL_Q,     0, RGB(0,14,0), RGB(0,15,0), RGB(0,5,0));
        SetPalette(SPRITE_PLANE, SPAL_BALL,  0, RGB(0,11,0), RGB(0,15,0), RGB(0,7,0));
        SetPalette(SPRITE_PLANE, SPAL_COILY, 0, RGB(0,13,0), RGB(0,15,0), RGB(0,3,0));
    } else {
        SetBackgroundColour(RGB(0, 0, 0));
        SetPalette(SCR_1_PLANE, PAL_TEXT,     0, RGB(15,15,15), RGB(15,15,15), RGB(15,15,15));
        /* Col1 = cube top, Col2 = left face, Col3 = right face */
        SetPalette(SCR_1_PLANE, PAL_CUBE_OFF, 0, RGB(3,6,15),  RGB(10,10,13), RGB(5,5,8));
        SetPalette(SCR_1_PLANE, PAL_CUBE_ON,  0, RGB(15,13,2), RGB(10,10,13), RGB(5,5,8));
        SetPalette(SPRITE_PLANE, SPAL_Q,     0, RGB(15,8,0),  RGB(15,15,15), RGB(3,2,2));
        SetPalette(SPRITE_PLANE, SPAL_BALL,  0, RGB(15,2,2),  RGB(15,10,10), RGB(8,0,0));
        SetPalette(SPRITE_PLANE, SPAL_COILY, 0, RGB(11,4,15), RGB(15,15,15), RGB(2,0,3));
    }
}

/* ================= Drawing ================= */

static void draw_cube(u8 r, u8 c) {
    u8 tx, ty, pal;
    tx = (u8)(cube_x(r, c) >> 3);
    ty = (u8)(cube_y(r) >> 3);
    if (cube_lit[r][c]) pal = PAL_CUBE_ON;
    else pal = PAL_CUBE_OFF;
    PutTile(SCR_1_PLANE, pal, tx,   ty,   T_CUBE);
    PutTile(SCR_1_PLANE, pal, tx+1, ty,   T_CUBE+1);
    PutTile(SCR_1_PLANE, pal, tx,   ty+1, T_CUBE+2);
    PutTile(SCR_1_PLANE, pal, tx+1, ty+1, T_CUBE+3);
}

/* Only the two top-face tiles change when a cube lights */
static void draw_cube_top(u8 r, u8 c) {
    u8 tx, ty, pal;
    tx = (u8)(cube_x(r, c) >> 3);
    ty = (u8)(cube_y(r) >> 3);
    if (cube_lit[r][c]) pal = PAL_CUBE_ON;
    else pal = PAL_CUBE_OFF;
    PutTile(SCR_1_PLANE, pal, tx,   ty, T_CUBE);
    PutTile(SCR_1_PLANE, pal, tx+1, ty, T_CUBE+1);
}

static void draw_pyramid(void) {
    u8 r, c;
    for (r = 0; r < 7; r++) {
        for (c = 0; c <= r; c++) {
            draw_cube(r, c);
        }
    }
}

static void draw_hud(void) {
    PrintString(SCR_1_PLANE, PAL_TEXT, 0, 0, "S:");
    PrintDecimal(SCR_1_PLANE, PAL_TEXT, 2, 0, score, 5);
    PrintString(SCR_1_PLANE, PAL_TEXT, 9, 0, "H:");
    PrintDecimal(SCR_1_PLANE, PAL_TEXT, 11, 0, hi_score, 5);
    PrintString(SCR_1_PLANE, PAL_TEXT, 17, 0, "L");
    PrintDecimal(SCR_1_PLANE, PAL_TEXT, 18, 0, level, 1);
    PrintString(SCR_1_PLANE, PAL_TEXT, 0, 18, "Q*");
    PrintDecimal(SCR_1_PLANE, PAL_TEXT, 2, 18, lives, 1);
}

static void draw_score(void) {
    PrintDecimal(SCR_1_PLANE, PAL_TEXT, 2, 0, score, 5);
}

/* Draw a 16x16 sprite as 4 unchained hardware sprites.
 * Hides the whole group if any part goes off the visible area. */
static void draw16(u8 base, u16 tile, s16 x, s16 y, u8 pal) {
    u8 px, py;
    if (x < 0 || y < 0 || x > 144 || y > 150) {
        UnsetSprite(base);
        UnsetSprite((u8)(base+1));
        UnsetSprite((u8)(base+2));
        UnsetSprite((u8)(base+3));
        return;
    }
    px = (u8)x;
    py = (u8)y;
    SetSprite(base,          tile,   0, px,        py,        pal);
    SetSprite((u8)(base+1), tile+1, 0, (u8)(px+8), py,        pal);
    SetSprite((u8)(base+2), tile+2, 0, px,        (u8)(py+8), pal);
    SetSprite((u8)(base+3), tile+3, 0, (u8)(px+8), (u8)(py+8), pal);
}

static void clear_sprites(void) {
    u8 i;
    for (i = 0; i < 64; i++) UnsetSprite(i);
}

static void draw_qbert(void) {
    u16 tile;
    s16 dy;
    if (q_face) tile = T_QR;
    else tile = T_QL;
    dy = q_py;
    if (q_state == Q_HOP) dy = q_py - (s16)hop_arc[q_hf];
    draw16(0, tile, q_px, dy, SPAL_Q);
}

static void draw_enemies(void) {
    u8 i, base;
    u16 tile;
    u8 pal;
    s16 dy;
    for (i = 0; i < MAX_ENEMIES; i++) {
        base = (u8)(4 + (i << 2));
        if (e_type[i] == E_NONE) {
            UnsetSprite(base);
            UnsetSprite((u8)(base+1));
            UnsetSprite((u8)(base+2));
            UnsetSprite((u8)(base+3));
        } else {
            if (e_type[i] == E_COILY) { tile = T_COILY; pal = SPAL_COILY; }
            else if (e_type[i] == E_EGG) { tile = T_BALL; pal = SPAL_COILY; }
            else { tile = T_BALL; pal = SPAL_BALL; }
            dy = e_py[i];
            if (e_hop[i] > 0 && e_fall[i] == 0) {
                dy = e_py[i] - (s16)hop_arc[(u8)(HOP_FRAMES - e_hop[i])];
            }
            draw16(base, tile, e_px[i], dy, pal);
        }
    }
}

/* ================= Screens ================= */

static void draw_title(void) {
    FillScreen(SCR_1_PLANE, ' ', PAL_TEXT);
    SysSetSystemFont();
    install_tiles();
    setup_palettes();
    PrintString(SCR_1_PLANE, PAL_TEXT, 5, 2, "@!#?@!");
    PrintString(SCR_1_PLANE, PAL_TEXT, 5, 4, "QBERT  NEO");
    PrintString(SCR_1_PLANE, PAL_TEXT, 1, 7, "STUDIO SO NOT KANSAI");
    PrintString(SCR_1_PLANE, PAL_TEXT, 2, 10, "UP=NE     LEFT=NW");
    PrintString(SCR_1_PLANE, PAL_TEXT, 2, 11, "RIGHT=SE  DOWN=SW");
    PrintString(SCR_1_PLANE, PAL_TEXT, 2, 13, "LIGHT EVERY CUBE!");
    PrintString(SCR_1_PLANE, PAL_TEXT, 3, 16, "PRESS A TO HOP");
    PrintDecimal(SCR_1_PLANE, PAL_TEXT, 7, 18, hi_score, 5);
    PrintString(SCR_1_PLANE, PAL_TEXT, 4, 18, "HI:");
}

static void draw_game_over(void) {
    PrintString(SCR_1_PLANE, PAL_TEXT, 5, 9, "GAME  OVER");
    PrintString(SCR_1_PLANE, PAL_TEXT, 3, 11, "PRESS A: RETRY");
}

/* ================= Game logic ================= */

static void reset_positions(void) {
    u8 i;
    q_state = Q_IDLE;
    q_r = 0; q_c = 0;
    q_px = (s16)cube_x(0, 0);
    q_py = (s16)cube_y(0) - 8;
    q_face = 1;
    q_hf = 0;
    death_timer = 0;
    death_fell = 0;
    for (i = 0; i < MAX_ENEMIES; i++) {
        e_type[i] = E_NONE;
        e_hop[i] = 0;
        e_fall[i] = 0;
    }
    coily_alive = 0;
    spawn_timer = 90;
}

static void start_level(void) {
    u8 r, c;
    for (r = 0; r < 7; r++) {
        for (c = 0; c < 7; c++) cube_lit[r][c] = 0;
    }
    lit_count = 0;
    FillScreen(SCR_1_PLANE, ' ', PAL_TEXT);
    SysSetSystemFont();
    install_tiles();
    setup_palettes();
    draw_pyramid();
    draw_hud();
    reset_positions();
}

static void game_start(void) {
    score = 0;
    lives = 3;
    level = 1;
    start_level();
}

static void add_score(u16 pts) {
    score = score + pts;
    if (score > hi_score) hi_score = score;
    draw_score();
}

/* Enemy hop speed shrinks with level */
static u8 enemy_delay(u8 base) {
    u16 d;
    u16 sub;
    d = base;
    sub = (u16)(level << 1);
    if (sub >= d - 16) return 16;
    return (u8)(d - sub);
}

static void spawn_enemy(void) {
    u8 i, slot, found;
    found = 0; slot = 0;
    for (i = 0; i < MAX_ENEMIES; i++) {
        if (e_type[i] == E_NONE) { slot = i; found = 1; break; }
    }
    if (!found) return;
    /* One Coily line at a time; otherwise red balls */
    if (!coily_alive && cheap_rand(3) == 0) {
        e_type[slot] = E_EGG;
        coily_alive = 1;
    } else {
        e_type[slot] = E_BALL;
    }
    e_r[slot] = 1;
    e_c[slot] = cheap_rand(2);
    e_px[slot] = (s16)cube_x(e_r[slot], e_c[slot]);
    e_py[slot] = (s16)cube_y(e_r[slot]) - 8;
    e_hop[slot] = 0;
    e_fall[slot] = 0;
    e_wait[slot] = enemy_delay(50);
}

static void kill_qbert(u8 fell) {
    u8 tx, ty;
    death_fell = fell;
    if (fell) {
        /* sound already played at hop-off */
        death_timer = 50;
    } else {
        PlaySound(SND_DEATH);
        q_state = Q_IDLE;
        tx = (u8)(cube_x(q_r, q_c) >> 3);
        ty = (u8)(cube_y(q_r) >> 3);
        if (ty >= 2) ty = (u8)(ty - 2);
        PrintString(SCR_1_PLANE, PAL_TEXT, tx, ty, "@!#?");
        death_timer = 70;
    }
}

static void after_death(void) {
    if (lives > 0) lives = lives - 1;
    if (lives == 0) {
        clear_sprites();
        draw_game_over();
        state = STATE_OVER;
        skip = 15;
        return;
    }
    /* Redraw pyramid + HUD to erase the swear balloon */
    draw_pyramid();
    draw_hud();
    reset_positions();
}

static void level_clear(void) {
    u8 i;
    PlaySound(SND_CLEAR);
    add_score(250);
    /* Flash the pyramid by toggling the lit palette */
    for (i = 0; i < 6; i++) {
        if (i & 1) {
            SetPalette(SCR_1_PLANE, PAL_CUBE_ON, 0, RGB(15,13,2), RGB(10,10,13), RGB(5,5,8));
        } else {
            SetPalette(SCR_1_PLANE, PAL_CUBE_ON, 0, RGB(15,15,15), RGB(13,13,15), RGB(8,8,10));
        }
        Sleep(6);
    }
    if (level < 9) level = level + 1;
    clear_sprites();
    start_level();
}

static void check_collisions(void) {
    u8 i;
    if (q_state != Q_IDLE) return;
    if (death_timer > 0) return;
    for (i = 0; i < MAX_ENEMIES; i++) {
        if (e_type[i] == E_NONE) continue;
        if (e_hop[i] > 0) continue;
        if (e_fall[i]) continue;
        if (e_r[i] == q_r && e_c[i] == q_c) {
            kill_qbert(0);
            return;
        }
    }
}

/* ---- Q*Bert movement ---- */

static void q_begin_hop(s8 nr, s8 nc) {
    q_nr = nr; q_nc = nc;
    q_valid = on_board(nr, nc);
    q_hf = 0;
    q_state = Q_HOP;
    /* horizontal: +-1/frame (8px), vertical: +-2/frame (16px) */
    if (nr > (s8)q_r) {
        if (nc > (s8)q_c) q_sx = 1;
        else q_sx = -1;
        q_sy = 2;
    } else {
        if (nc < (s8)q_c) q_sx = -1;
        else q_sx = 1;
        q_sy = -2;
    }
    if (q_sx > 0) q_face = 1;
    else q_face = 0;
    PlaySound(SND_HOP);
}

static void q_input(void) {
    u8 u, d, l, r;
    if (q_state != Q_IDLE) return;
    if (death_timer > 0) return;
    u = pad_cur & J_UP;
    d = pad_cur & J_DOWN;
    l = pad_cur & J_LEFT;
    r = pad_cur & J_RIGHT;
    if (u && l)      q_begin_hop((s8)q_r - 1, (s8)q_c - 1);
    else if (u && r) q_begin_hop((s8)q_r - 1, (s8)q_c);
    else if (d && l) q_begin_hop((s8)q_r + 1, (s8)q_c);
    else if (d && r) q_begin_hop((s8)q_r + 1, (s8)q_c + 1);
    else if (u)      q_begin_hop((s8)q_r - 1, (s8)q_c);
    else if (l)      q_begin_hop((s8)q_r - 1, (s8)q_c - 1);
    else if (d)      q_begin_hop((s8)q_r + 1, (s8)q_c);
    else if (r)      q_begin_hop((s8)q_r + 1, (s8)q_c + 1);
}

static void q_update(void) {
    if (q_state == Q_HOP) {
        q_px = q_px + (s16)q_sx;
        q_py = q_py + (s16)q_sy;
        q_hf = q_hf + 1;
        if (q_hf >= HOP_FRAMES) {
            if (q_valid) {
                q_r = (u8)q_nr; q_c = (u8)q_nc;
                q_px = (s16)cube_x(q_r, q_c);
                q_py = (s16)cube_y(q_r) - 8;
                q_state = Q_IDLE;
                if (!cube_lit[q_r][q_c]) {
                    cube_lit[q_r][q_c] = 1;
                    lit_count = lit_count + 1;
                    draw_cube_top(q_r, q_c);
                    add_score(25);
                    PlaySound(SND_LIT);
                    if (lit_count >= 28) {
                        level_clear();
                        return;
                    }
                }
                check_collisions();
            } else {
                q_state = Q_FALL;
                PlaySound(SND_FALL);
            }
        }
    } else if (q_state == Q_FALL) {
        q_py = q_py + 5;
        if (q_py > 160) {
            kill_qbert(1);
            q_state = Q_IDLE;
        }
    }
}

/* ---- Enemy movement ---- */

static void e_begin_hop(u8 i, s8 nr, s8 nc) {
    e_nr[i] = nr; e_nc[i] = nc;
    e_hop[i] = HOP_FRAMES;
    if (nr > (s8)e_r[i]) {
        if (nc > (s8)e_c[i]) e_sx[i] = 1;
        else e_sx[i] = -1;
        e_sy[i] = 2;
    } else {
        if (nc < (s8)e_c[i]) e_sx[i] = -1;
        else e_sx[i] = 1;
        e_sy[i] = -2;
    }
    PlaySound(SND_EHOP);
}

static void coily_choose(u8 i) {
    s8 r, c, nr, nc;
    r = (s8)e_r[i];
    c = (s8)e_c[i];
    if ((s8)q_r < r) {
        /* chase upward */
        nr = r - 1;
        if ((s8)q_c >= c) nc = c;
        else nc = c - 1;
        if (!on_board(nr, nc)) {
            if (nc == c) nc = (s8)(c - 1);
            else nc = c;
        }
    } else if ((s8)q_r > r) {
        nr = r + 1;
        if ((s8)q_c > c) nc = c + 1;
        else nc = c;
    } else {
        /* same row: zigzag toward the column */
        if (r < 6) {
            nr = r + 1;
            if ((s8)q_c > c) nc = c + 1;
            else nc = c;
        } else {
            nr = r - 1;
            if ((s8)q_c > c) nc = c;
            else nc = c - 1;
            if (!on_board(nr, nc)) {
                if (nc == c) nc = (s8)(c - 1);
                else nc = c;
            }
        }
    }
    e_begin_hop(i, nr, nc);
}

static void e_update(void) {
    u8 i;
    s8 nr, nc;
    for (i = 0; i < MAX_ENEMIES; i++) {
        if (e_type[i] == E_NONE) continue;

        if (e_fall[i]) {
            e_py[i] = e_py[i] + 5;
            if (e_py[i] > 160) {
                if (e_type[i] == E_EGG) coily_alive = 0;
                e_type[i] = E_NONE;
            }
            continue;
        }

        if (e_hop[i] > 0) {
            e_px[i] = e_px[i] + (s16)e_sx[i];
            e_py[i] = e_py[i] + (s16)e_sy[i];
            e_hop[i] = e_hop[i] - 1;
            if (e_hop[i] == 0) {
                if (on_board(e_nr[i], e_nc[i])) {
                    e_r[i] = (u8)e_nr[i];
                    e_c[i] = (u8)e_nc[i];
                    e_px[i] = (s16)cube_x(e_r[i], e_c[i]);
                    e_py[i] = (s16)cube_y(e_r[i]) - 8;
                    if (e_type[i] == E_COILY) e_wait[i] = enemy_delay(42);
                    else e_wait[i] = enemy_delay(50);
                    /* Egg hatches on the bottom row */
                    if (e_type[i] == E_EGG && e_r[i] == 6) {
                        e_type[i] = E_COILY;
                        e_wait[i] = 60;
                    }
                    check_collisions();
                } else {
                    e_fall[i] = 1;
                }
            }
            continue;
        }

        /* grounded: countdown to next hop */
        if (e_wait[i] > 0) {
            e_wait[i] = e_wait[i] - 1;
            continue;
        }

        if (e_type[i] == E_COILY) {
            coily_choose(i);
        } else {
            /* balls hop down-left or down-right; off the bottom = gone */
            nr = (s8)e_r[i] + 1;
            if (cheap_rand(2)) nc = (s8)e_c[i] + 1;
            else nc = (s8)e_c[i];
            e_begin_hop(i, nr, nc);
        }
    }
}

static void game_update(void) {
    /* death freeze */
    if (death_timer > 0) {
        death_timer = death_timer - 1;
        if (death_timer == 0) {
            after_death();
            if (state != STATE_GAME) return;
        }
        draw_qbert();
        draw_enemies();
        return;
    }

    /* spawns */
    if (spawn_timer > 0) spawn_timer = spawn_timer - 1;
    if (spawn_timer == 0) {
        spawn_enemy();
        spawn_timer = (u8)(enemy_delay(140) + cheap_rand(60));
    }

    q_input();
    q_update();
    if (state != STATE_GAME) return;
    if (death_timer == 0) e_update();

    draw_qbert();
    draw_enemies();
}

/* ================= Konami code ================= */

static void check_konami(void) {
    u8 expected;
    if (pad_press == 0) return;
    expected = konami_seq[konami_pos];
    if (pad_press & expected) {
        konami_pos = konami_pos + 1;
        if (konami_pos >= 10) {
            retro_mode = retro_mode ^ 1;
            konami_pos = 0;
            PlaySound(SND_CLEAR);
            draw_title();
        }
    } else {
        konami_pos = 0;
    }
}

/* ================= Entry ================= */

void main(void) {
    u8 paused;
    InitNGPC();
    SysSetSystemFont();
    install_tiles();
    setup_palettes();

    InstallSoundDriver();
    WaitVsync();
    WaitVsync();
    InstallSounds(game_sounds, 6);

    retro_mode = 0;
    konami_pos = 0;
    hi_score = 0;
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
        } else if (state == STATE_GAME) {
            if (pad_press & J_OPTION) {
                paused = paused ^ 1;
                if (paused) PrintString(SCR_1_PLANE, PAL_TEXT, 7, 9, "PAUSED");
                else { PrintString(SCR_1_PLANE, PAL_TEXT, 7, 9, "      "); draw_pyramid(); }
            }
            if (!paused) game_update();
        } else if (state == STATE_OVER) {
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
