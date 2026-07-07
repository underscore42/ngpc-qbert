/* screen.c — QBERT NEO: palettes, level colour schemes, arcade font,
 * pyramid/HUD drawing, title, bonus intermission, score screens */
#include "screen.h"
#include "tiles.h"

/* ---- Level colour schemes, extracted from the arcade sheet ----
 * Each row: faceL, faceR, top state 0, top state 1 (mid), top state 2 (target)
 * NGPC RGB444. Level N uses scheme (N-1) mod NUM_SCHEMES.
 */
static const u16 schemes[NUM_SCHEMES][5] = {
    /* Faces deliberately darker than every top so the diamonds pop */
    /* 1: green faces, blue -> brown -> yellow */
    { RGB(0,4,0),  RGB(0,6,1),  RGB(0,5,15),  RGB(11,6,2),  RGB(13,13,0) },
    /* 2: teal faces, violet -> pink -> yellow */
    { RGB(3,7,6),  RGB(2,3,3),  RGB(6,5,15),  RGB(15,6,6),  RGB(13,13,0) },
    /* 3: brown/orange faces, blue -> green -> cream */
    { RGB(6,3,0),  RGB(9,4,1),  RGB(0,5,14),  RGB(2,12,3),  RGB(14,13,7) },
    /* 4: grey faces, blue -> mid grey -> light cyan */
    { RGB(6,6,6),  RGB(2,2,2),  RGB(2,7,13),  RGB(9,9,9),   RGB(12,13,13) },
    /* 5: slate/navy faces, blue -> magenta -> olive */
    { RGB(5,6,6),  RGB(0,0,7),  RGB(0,7,15),  RGB(11,0,8),  RGB(11,12,0) },
    /* 6: khaki/navy faces, sage -> salmon -> cyan */
    { RGB(8,8,1),  RGB(0,3,7),  RGB(5,7,6),   RGB(15,5,5),  RGB(0,11,14) },
    /* 7: khaki/red faces, navy -> blue -> purple */
    { RGB(8,8,1),  RGB(8,2,2),  RGB(0,3,10),  RGB(3,9,13),  RGB(10,0,9) },
    /* 8: teal faces, yellow -> violet -> pink */
    { RGB(3,7,6),  RGB(2,3,3),  RGB(13,13,0), RGB(6,5,15),  RGB(15,6,6) },
    /* 9: green faces, brown -> yellow -> blue */
    { RGB(0,4,0),  RGB(0,6,1),  RGB(11,6,2),  RGB(13,13,0), RGB(0,5,15) }
};

/* Konami-code Mello Yello scheme: can colours */
static const u16 mello_scheme[5] = {
    RGB(0,5,0), RGB(0,3,0), RGB(0,9,1), RGB(13,12,0), RGB(15,1,2)
};

static const u16 *cur_scheme(void) {
    u8 i;
    if (mello_mode) return mello_scheme;
    i = level;
    if (i < 1) i = 1;
    i = i - 1;
    while (i >= NUM_SCHEMES) i = i - NUM_SCHEMES;
    return schemes[i];
}

void setup_palettes(void) {
    const u16 *s;
    s = cur_scheme();
    SetBackgroundColour(RGB(0,0,0));
    SetPalette(SCR_1_PLANE, PAL_TEXT,     0, RGB(15,15,15), RGB(15,15,15), RGB(15,15,15));
    SetPalette(SCR_1_PLANE, PAL_CUBE_OFF, 0, s[2], s[0], s[1]);
    SetPalette(SCR_1_PLANE, PAL_CUBE_MID, 0, s[3], s[0], s[1]);
    SetPalette(SCR_1_PLANE, PAL_CUBE_ON,  0, s[4], s[0], s[1]);
    SetPalette(SCR_1_PLANE, PAL_AFONT,    0, RGB(13,13,0), RGB(15,1,2), RGB(0,0,0));
    SetPalette(SCR_1_PLANE, PAL_CAN,      0, RGB(13,12,0), RGB(15,15,15), RGB(0,7,0));
    SetPalette(SPRITE_PLANE, SPAL_Q,      0, RGB(15,6,0),  RGB(15,15,15), RGB(2,1,1));
    SetPalette(SPRITE_PLANE, SPAL_BALL,   0, RGB(15,1,2),  RGB(15,10,10), RGB(7,0,0));
    SetPalette(SPRITE_PLANE, SPAL_COILY,  0, RGB(11,0,11), RGB(15,15,15), RGB(5,0,5));
    SetPalette(SPRITE_PLANE, SPAL_BUBBLE, 0, RGB(15,1,2),  RGB(15,15,15), RGB(1,1,1));
    SetPalette(SPRITE_PLANE, SPAL_BIGTXT, 0, RGB(13,13,0), RGB(15,1,2),  RGB(0,0,0));
    SetPalette(SPRITE_PLANE, SPAL_DISC,   0, RGB(12,12,12),RGB(15,15,15), RGB(6,2,10));
}

/* Flash the target-colour palette during level clear */
void flash_target_palette(u8 bright) {
    const u16 *s;
    s = cur_scheme();
    if (bright) SetPalette(SCR_1_PLANE, PAL_CUBE_ON, 0, RGB(15,15,15), s[0], s[1]);
    else SetPalette(SCR_1_PLANE, PAL_CUBE_ON, 0, s[4], s[0], s[1]);
}

/* ---- Arcade big font (16x16 glyphs, 2x2 tiles each) ---- */

static u16 glyph_of(char ch) {
    /* returns tile base, or 0 for space/unknown */
    u16 g;
    if (ch >= 'A' && ch <= 'Z') g = 12 + (u16)(ch - 'A');
    else if (ch >= '1' && ch <= '9') g = (u16)(ch - '1');
    else if (ch == '0') g = 12 + ('O' - 'A');
    else if (ch == '.') g = 9;
    else if (ch == '!') g = 10;
    else if (ch == '-') g = 11;
    else return 0;
    return (u16)(T_AFONT + (g << 2));
}

void arc_print(u8 x, u8 y, const char *s) {
    u16 t;
    u8 cx;
    cx = x;
    while (*s) {
        t = glyph_of(*s);
        if (t) {
            PutTile(SCR_1_PLANE, PAL_AFONT, cx,   y,   t);
            PutTile(SCR_1_PLANE, PAL_AFONT, cx+1, y,   t+1);
            PutTile(SCR_1_PLANE, PAL_AFONT, cx,   y+1, t+2);
            PutTile(SCR_1_PLANE, PAL_AFONT, cx+1, y+1, t+3);
        } else {
            PutTile(SCR_1_PLANE, PAL_TEXT, cx,   y,   ' ');
            PutTile(SCR_1_PLANE, PAL_TEXT, cx+1, y,   ' ');
            PutTile(SCR_1_PLANE, PAL_TEXT, cx,   y+1, ' ');
            PutTile(SCR_1_PLANE, PAL_TEXT, cx+1, y+1, ' ');
        }
        cx = cx + 2;
        s++;
    }
}

/* ---- Sprite text: transparent big text over the playfield ----
 * Installs each string's glyphs into the T_SPRTXT tile window (12 glyph
 * slots) and draws them as sprites. Returns the number of sprites used. */
u8 sprite_text(u8 base, u8 x, u8 y, const char *str) {
    u16 t, dt;
    u8 n, slot, cx;
    n = 0; slot = 0; cx = x;
    while (*str) {
        t = glyph_of(*str);
        if (t && slot < 12) {
            dt = (u16)(T_SPRTXT + (slot << 2));
            InstallTileSetAt((const unsigned short (*)[8])afont[(t - T_AFONT)], 32, dt);
            SetSprite((u8)(base+n),   dt,   0, cx,        y,        SPAL_BIGTXT);
            SetSprite((u8)(base+n+1), dt+1, 0, (u8)(cx+8), y,        SPAL_BIGTXT);
            SetSprite((u8)(base+n+2), dt+2, 0, cx,        (u8)(y+8), SPAL_BIGTXT);
            SetSprite((u8)(base+n+3), dt+3, 0, (u8)(cx+8), (u8)(y+8), SPAL_BIGTXT);
            n = (u8)(n + 4);
            slot = slot + 1;
        }
        cx = (u8)(cx + 16);
        str++;
    }
    return n;
}

void hide_sprite_text(u8 base, u8 n) {
    u8 i;
    for (i = 0; i < n; i++) UnsetSprite((u8)(base + i));
}

/* Print a single big glyph (initials entry) */
void arc_char(u8 x, u8 y, char ch) {
    char buf[2];
    buf[0] = ch; buf[1] = 0;
    arc_print(x, y, buf);
}

/* ---- Pyramid ---- */

void draw_cube(u8 r, u8 c) {
    u8 tx, ty, pal;
    u16 tl, tr;
    tx = (u8)(cube_x(r, c) >> 3);
    ty = (u8)(cube_y(r) >> 3);
    if (cube_st[r][c] == 0) pal = PAL_CUBE_OFF;
    else if (cube_st[r][c] == 1) pal = PAL_CUBE_MID;
    else pal = PAL_CUBE_ON;
    if (c > 0) tl = T_CUBE + 2;
    else tl = T_CUBE;
    if (c < r) tr = T_CUBE + 3;
    else tr = T_CUBE + 1;
    PutTile(SCR_1_PLANE, pal, tx,   ty,   tl);
    PutTile(SCR_1_PLANE, pal, tx+1, ty,   tr);
    PutTile(SCR_1_PLANE, pal, tx,   ty+1, T_CUBE+4);
    PutTile(SCR_1_PLANE, pal, tx+1, ty+1, T_CUBE+5);
    /* bottom row gets the tapered underside cap */
    if (r == 6) {
        PutTile(SCR_1_PLANE, pal, tx,   ty+2, T_CUBE+6);
        PutTile(SCR_1_PLANE, pal, tx+1, ty+2, T_CUBE+7);
    }
}

void draw_cube_top(u8 r, u8 c) {
    u8 tx, ty, pal;
    u16 tl, tr;
    tx = (u8)(cube_x(r, c) >> 3);
    ty = (u8)(cube_y(r) >> 3);
    if (cube_st[r][c] == 0) pal = PAL_CUBE_OFF;
    else if (cube_st[r][c] == 1) pal = PAL_CUBE_MID;
    else pal = PAL_CUBE_ON;
    if (c > 0) tl = T_CUBE + 2;
    else tl = T_CUBE;
    if (c < r) tr = T_CUBE + 3;
    else tr = T_CUBE + 1;
    PutTile(SCR_1_PLANE, pal, tx,   ty, tl);
    PutTile(SCR_1_PLANE, pal, tx+1, ty, tr);
}

void draw_pyramid(void) {
    u8 r, c;
    for (r = 0; r < 7; r++) {
        for (c = 0; c <= r; c++) draw_cube(r, c);
    }
}

void draw_hud(void) {
    PrintString(SCR_1_PLANE, PAL_TEXT, 0, 0, "S:");
    PrintDecimal(SCR_1_PLANE, PAL_TEXT, 2, 0, score, 5);
    PrintString(SCR_1_PLANE, PAL_TEXT, 9, 0, "H:");
    PrintDecimal(SCR_1_PLANE, PAL_TEXT, 11, 0, hs_score[0], 5);
    PrintString(SCR_1_PLANE, PAL_TEXT, 17, 0, "L");
    PrintDecimal(SCR_1_PLANE, PAL_TEXT, 18, 0, level, 1);
    PrintString(SCR_1_PLANE, PAL_TEXT, 0, 18, "Q*");
    PrintDecimal(SCR_1_PLANE, PAL_TEXT, 2, 18, lives, 1);
}

void draw_score_hud(void) {
    PrintDecimal(SCR_1_PLANE, PAL_TEXT, 2, 0, score, 5);
    if (score >= hs_score[0]) PrintDecimal(SCR_1_PLANE, PAL_TEXT, 11, 0, score, 5);
}

/* ---- Screen setup helper: wipe, font, tiles, palettes ---- */
void fresh_screen(void) {
    FillScreen(SCR_1_PLANE, ' ', PAL_TEXT);
    SysSetSystemFont();
    install_tiles();
    setup_palettes();
}

/* three initials of entry i as a printable string */
static char initbuf[4];
const char *hs_init_str(u8 i) {
    initbuf[0] = (char)hs_init[i][0];
    initbuf[1] = (char)hs_init[i][1];
    initbuf[2] = (char)hs_init[i][2];
    initbuf[3] = 0;
    return initbuf;
}

/* ---- Title ---- */
void draw_title(void) {
    fresh_screen();
    arc_print(1, 2, "QBERT NEO");
    PrintString(SCR_1_PLANE, PAL_TEXT, 1, 6, "STUDIO SO NOT KANSAI");
    PrintString(SCR_1_PLANE, PAL_TEXT, 2, 9,  "UP=NE     LEFT=NW");
    PrintString(SCR_1_PLANE, PAL_TEXT, 2, 10, "RIGHT=SE  DOWN=SW");
    PrintString(SCR_1_PLANE, PAL_TEXT, 2, 12, "LIGHT EVERY CUBE!");
    PrintString(SCR_1_PLANE, PAL_TEXT, 3, 14, "A:START  OPT:SCORES");
    if (mello_mode) PrintString(SCR_1_PLANE, PAL_TEXT, 4, 16, "* MELLO MODE *");
    PrintString(SCR_1_PLANE, PAL_TEXT, 4, 18, "HI:");
    PrintDecimal(SCR_1_PLANE, PAL_TEXT, 7, 18, hs_score[0], 5);
    PrintString(SCR_1_PLANE, PAL_TEXT, 13, 18, hs_init_str(0));
}

/* ---- Mello Yello can (BG, 3x4 tiles) ---- */
void draw_can(u8 tx, u8 ty) {
    u8 x, y;
    u16 t;
    t = T_CAN;
    for (y = 0; y < 4; y++) {
        for (x = 0; x < 3; x++) {
            PutTile(SCR_1_PLANE, PAL_CAN, (u8)(tx+x), (u8)(ty+y), t);
            t = t + 1;
        }
    }
}

/* ---- Bonus intermission screen ---- */
void draw_bonus_screen(void) {
    fresh_screen();
    arc_print(5, 3, "BONUS");
    PrintDecimal(SCR_1_PLANE, PAL_TEXT, 8, 6, bonus_amt, 5);
    if (mello_mode) {
        arc_print(5, 8, "MELLO");
        arc_print(5, 11, "YELLO");
        PrintString(SCR_1_PLANE, PAL_TEXT, 4, 14, "AT 7-ELEVEN NOW");
        draw_can(2, 8);
        draw_can(15, 8);
    } else {
        PrintString(SCR_1_PLANE, PAL_TEXT, 5, 9, "LEVEL");
        PrintDecimal(SCR_1_PLANE, PAL_TEXT, 11, 9, (u16)(level+1), 1);
        PrintString(SCR_1_PLANE, PAL_TEXT, 13, 9, "NEXT");
        draw_can(9, 11);
    }
}

/* ---- High score table ---- */
void draw_scores(void) {
    u8 i, y;
    fresh_screen();
    arc_print(2, 1, "HISCORES");
    for (i = 0; i < 5; i++) {
        y = (u8)(5 + (i << 1));
        PrintDecimal(SCR_1_PLANE, PAL_TEXT, 3, y, (u16)(i+1), 1);
        PrintString(SCR_1_PLANE, PAL_TEXT, 6, y, hs_init_str(i));
        PrintDecimal(SCR_1_PLANE, PAL_TEXT, 11, y, hs_score[i], 5);
    }
    PrintString(SCR_1_PLANE, PAL_TEXT, 2, 17, "A:PLAY  OPT:TITLE");
}

/* ---- Initials entry ---- */
void draw_entry(u8 slot, const u8 letters[3], u8 cur) {
    u8 i, x;
    fresh_screen();
    arc_print(0, 2, "NEW SCORE!");
    PrintString(SCR_1_PLANE, PAL_TEXT, 4, 6, "RANK");
    PrintDecimal(SCR_1_PLANE, PAL_TEXT, 9, 6, (u16)(slot+1), 1);
    PrintDecimal(SCR_1_PLANE, PAL_TEXT, 12, 6, score, 5);
    for (i = 0; i < 3; i++) {
        x = (u8)(7 + (i << 1));
        arc_char(x, 9, (char)letters[i]);
        if (i == cur) PrintString(SCR_1_PLANE, PAL_TEXT, x, 11, "--");
        else PrintString(SCR_1_PLANE, PAL_TEXT, x, 11, "  ");
    }
    PrintString(SCR_1_PLANE, PAL_TEXT, 1, 14, "UP/DN:LETTER  A:OK");
}
