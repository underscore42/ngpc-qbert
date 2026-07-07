/* game.c — QBERT NEO: gameplay
 * Multi-state cubes with per-level rules, chasing Coily, bonus intermission.
 */
#include "game.h"
#include "screen.h"
#include "tiles.h"

/* ---- Shared state ---- */
u8  state, skip;
u8  pad_cur, pad_prev, pad_press;
u8  rand_seed;
u8  mello_mode;

u8  cube_st[7][7];
u8  lit_count;
u16 score;
u8  lives;
u8  level;
u16 bonus_amt;
u8  bonus_timer;

u8  q_state;
u8  q_r, q_c;
s16 q_px, q_py;
u8  q_pose;

u8  e_type[MAX_ENEMIES];

/* ---- Local state ---- */
static s8  q_sx, q_sy;
static u8  q_hf;
static s8  q_nr, q_nc;
static u8  q_valid;
static u8  q_rest;            /* landing lockout vs held input   */
static u8  death_timer;
static u8  bubble_on;

static u8  e_r[MAX_ENEMIES], e_c[MAX_ENEMIES];
static s16 e_px[MAX_ENEMIES], e_py[MAX_ENEMIES];
static s8  e_sx[MAX_ENEMIES], e_sy[MAX_ENEMIES];
static u8  e_hop[MAX_ENEMIES];
static u8  e_wait[MAX_ENEMIES];
static u8  e_fall[MAX_ENEMIES];
static s8  e_nr[MAX_ENEMIES], e_nc[MAX_ENEMIES];
static u8  coily_alive;
static u8  spawn_timer;
static s16 bq_x;              /* bonus-screen Q*Bert x */
static u8  bq_phase;

/* Flying discs: index 0 = left of pyramid, 1 = right */
static u8 disc_on[2];
static u8 disc_gr[2];         /* grid row the disc floats beside      */
static u8 disc_ride;          /* 0/1: which disc Q is riding, +1; 0=none */
static s8 lure_r, lure_c;     /* cell Q jumped toward, for Coily bait */
static u8 hop_alt;            /* rotates hop sound variants           */

/* Per-level rules (index level-1, capped at 9):
 * adv2: cubes need two hops (via mid state); revert: what a target cube
 * does when hopped again: 0=nothing, 1=back to start, 2=back one step */
static const u8 lvl_adv2[NUM_SCHEMES]   = { 0, 1, 0, 1, 0, 1, 0, 1, 1 };
static const u8 lvl_revert[NUM_SCHEMES] = { 0, 0, 1, 2, 1, 1, 1, 2, 1 };

static const u8 hop_arc[HOP_FRAMES] = { 4, 7, 9, 10, 10, 9, 7, 4 };

/* ================= Utilities ================= */

u8 cheap_rand(u8 max) {
    u16 v;
    rand_seed = (u8)(rand_seed * 13 + 7);
    v = rand_seed;
    return (u8)(v % max);
}

u16 cube_x(u8 r, u8 c) {
    u16 x;
    x = 72;
    x = x - ((u16)r << 3);
    x = x + ((u16)c << 4);
    return x;
}

u16 cube_y(u8 r) {
    return (u16)(16 + ((u16)r << 4));
}

static u8 on_board(s8 r, s8 c) {
    if (r < 0) return 0;
    if (r > 6) return 0;
    if (c < 0) return 0;
    if (c > r) return 0;
    return 1;
}

static u8 rule_idx(void) {
    u8 i;
    i = level;
    if (i < 1) i = 1;
    if (i > NUM_SCHEMES) i = NUM_SCHEMES;
    return (u8)(i - 1);
}

/* ================= Sprites ================= */

void clear_sprites(void) {
    u8 i;
    for (i = 0; i < 64; i++) UnsetSprite(i);
}

static void hide_group(u8 base, u8 n) {
    u8 i;
    for (i = 0; i < n; i++) UnsetSprite((u8)(base + i));
}

/* 16x16 = 2x2 sprites */
static void draw16(u8 base, u16 tile, s16 x, s16 y, u8 pal) {
    u8 px, py;
    if (x < 0 || y < 0 || x > 144 || y > 148) { hide_group(base, 4); return; }
    px = (u8)x; py = (u8)y;
    SetSprite(base,          tile,   0, px,        py,        pal);
    SetSprite((u8)(base+1), tile+1, 0, (u8)(px+8), py,        pal);
    SetSprite((u8)(base+2), tile+2, 0, px,        (u8)(py+8), pal);
    SetSprite((u8)(base+3), tile+3, 0, (u8)(px+8), (u8)(py+8), pal);
}

/* 16x24 = 2x3 sprites */
static void draw2x3(u8 base, u16 tile, s16 x, s16 y, u8 pal) {
    u8 px, py, rr, s;
    u16 t;
    if (x < 0 || y < 0 || x > 144 || y > 140) { hide_group(base, 6); return; }
    px = (u8)x; py = (u8)y;
    s = base; t = tile;
    for (rr = 0; rr < 3; rr++) {
        SetSprite(s,          t,   0, px,        (u8)(py + (rr<<3)), pal);
        SetSprite((u8)(s+1), t+1, 0, (u8)(px+8), (u8)(py + (rr<<3)), pal);
        s = s + 2; t = t + 2;
    }
}

/* 32x16 = 4x2 sprites (swear bubble) */
static void draw_bubble_at(s16 x, s16 y) {
    u8 px, py, cc, s;
    u16 t;
    if (x < 0) x = 0;
    if (x > 128) x = 128;
    if (y < 0) y = 0;
    px = (u8)x; py = (u8)y;
    s = SPR_BUBBLE; t = T_BUBBLE;
    for (cc = 0; cc < 4; cc++) {
        SetSprite(s,          t,   0, (u8)(px + (cc<<3)), py,        SPAL_BUBBLE);
        SetSprite((u8)(s+4), t+4, 0, (u8)(px + (cc<<3)), (u8)(py+8), SPAL_BUBBLE);
        s = s + 1; t = t + 1;
    }
}

static void hide_bubble(void) { hide_group(SPR_BUBBLE, 8); }

static u16 disc_x(u8 i) {
    if (i == 0) return (u16)(56 - ((u16)disc_gr[0] << 3));
    return (u16)(88 + ((u16)disc_gr[1] << 3));
}

static u16 disc_y(u8 i) {
    return cube_y(disc_gr[i]);
}

static void draw_disc16x8(u8 base, s16 x, s16 y) {
    u8 px, py;
    if (x < 0 || y < 0 || x > 144 || y > 148) { hide_group(base, 2); return; }
    px = (u8)x; py = (u8)y;
    SetSprite(base,          T_DISC,   0, px,        py, SPAL_DISC);
    SetSprite((u8)(base+1), T_DISC+1, 0, (u8)(px+8), py, SPAL_DISC);
}

static void draw_discs(void) {
    u8 i, base;
    for (i = 0; i < 2; i++) {
        base = (u8)(SPR_DISC + (i << 1));
        if (disc_ride == (u8)(i + 1)) {
            /* ridden disc travels under Q*Bert */
            draw_disc16x8(base, q_px, q_py + 16);
        } else if (disc_on[i]) {
            draw_disc16x8(base, (s16)disc_x(i), (s16)disc_y(i));
        } else {
            hide_group(base, 2);
        }
    }
}

static void draw_qbert(void) {
    u16 tile;
    s16 dy;
    if (q_pose == POSE_DL) tile = T_QB_DL;
    else if (q_pose == POSE_DR) tile = T_QB_DR;
    else if (q_pose == POSE_UL) tile = T_QB_UL;
    else tile = T_QB_UR;
    dy = q_py;
    if (q_state == Q_HOP) dy = q_py - (s16)hop_arc[q_hf];
    draw16(SPR_Q, tile, q_px, dy, SPAL_Q);
}

static void draw_enemies(void) {
    u8 i, base;
    s16 dy;
    for (i = 0; i < MAX_ENEMIES; i++) {
        base = (u8)(SPR_ENEMY + i * 6);
        if (e_type[i] == E_NONE) {
            hide_group(base, 6);
        } else {
            dy = e_py[i];
            if (e_hop[i] > 0 && e_fall[i] == 0) {
                dy = e_py[i] - (s16)hop_arc[(u8)(HOP_FRAMES - e_hop[i])];
            }
            if (e_type[i] == E_COILY) {
                draw2x3(base, T_COILY, e_px[i], dy, SPAL_COILY);
            } else {
                hide_group((u8)(base+4), 2);
                if (e_type[i] == E_EGG) draw16(base, T_BALL, e_px[i], dy, SPAL_COILY);
                else draw16(base, T_BALL, e_px[i], dy, SPAL_BALL);
            }
        }
    }
}

/* ================= Setup ================= */

static void snap_q(void) {
    q_px = (s16)cube_x(q_r, q_c);
    q_py = (s16)cube_y(q_r) - 8;
}

static void reset_positions(void) {
    u8 i;
    q_state = Q_IDLE;
    q_r = 0; q_c = 0;
    snap_q();
    q_pose = POSE_DL;
    q_hf = 0;
    q_rest = 0;
    death_timer = 0;
    bubble_on = 0;
    hide_bubble();
    for (i = 0; i < MAX_ENEMIES; i++) {
        e_type[i] = E_NONE;
        e_hop[i] = 0;
        e_fall[i] = 0;
    }
    coily_alive = 0;
    spawn_timer = 100;
    disc_ride = 0;
}

void start_level(void) {
    u8 r, c;
    for (r = 0; r < 7; r++) {
        for (c = 0; c < 7; c++) cube_st[r][c] = 0;
    }
    lit_count = 0;
    disc_on[0] = 1;
    disc_on[1] = 1;
    disc_gr[0] = (u8)(1 + cheap_rand(4));
    disc_gr[1] = (u8)(1 + cheap_rand(4));
    fresh_screen();
    draw_pyramid();
    draw_hud();
    reset_positions();
}

void game_start(void) {
    score = 0;
    lives = 3;
    level = 1;
    start_level();
}

static void add_score(u16 pts) {
    if (score > (u16)(65000 - pts)) score = 65000;
    else score = score + pts;
    draw_score_hud();
}

static u8 enemy_delay(u8 base) {
    u16 d, sub;
    d = base;
    sub = (u16)(level << 1);
    if (sub >= d - 16) return 16;
    return (u8)(d - sub);
}

/* ================= Death / bonus ================= */

static void kill_qbert(u8 fell) {
    if (fell) {
        death_timer = 50;
    } else {
        PlaySound(SND_DEATH);
        q_state = Q_IDLE;
        bubble_on = 1;
        draw_bubble_at(q_px - 8, q_py - 18);
        death_timer = 70;
    }
}

static void after_death(void) {
    if (lives > 0) lives = lives - 1;
    hide_bubble();
    bubble_on = 0;
    if (lives == 0) {
        clear_sprites();
        sprite_text(0, 16, 60, "GAME OVER");
        PrintString(SCR_1_PLANE, PAL_TEXT, 3, 17, "PRESS A BUTTON");
        state = STATE_OVER;
        skip = 20;
        return;
    }
    draw_pyramid();
    draw_hud();
    reset_positions();
}

static void begin_bonus(void) {
    u8 i;
    PlaySound(SND_CLEAR);
    /* flash the pyramid */
    for (i = 0; i < 6; i++) {
        flash_target_palette((u8)(i & 1));
        Sleep(6);
    }
    flash_target_palette(0);
    bonus_amt = (u16)(500 + ((u16)level << 8));
    clear_sprites();
    draw_bonus_screen();
    bonus_timer = 240;
    bq_x = -16;
    bq_phase = 0;
    q_pose = POSE_DR;
    state = STATE_BONUS;
    skip = 5;
}

/* Called each frame in STATE_BONUS */
void bonus_update(void) {
    s16 dy;
    bonus_timer = bonus_timer - 1;
    /* Q*Bert hops across the bottom of the screen */
    bq_x = bq_x + 1;
    bq_phase = (u8)((bq_phase + 1) & 15);
    dy = 122;
    if (bq_phase < 8) dy = 122 - (s16)hop_arc[bq_phase];
    draw16(SPR_Q, T_QB_DR, bq_x, dy, SPAL_Q);
    if (bq_phase == 0) PlaySound(SND_HOP);
    if (bonus_timer == 0 || (pad_press & J_A)) {
        add_score(bonus_amt);
        if (level < 99) level = level + 1;
        clear_sprites();
        start_level();
        state = STATE_GAME;
        skip = 5;
    }
}

/* ================= Cube logic ================= */

static void land_on_cube(void) {
    u8 st, ri, adv2, rev;
    ri = rule_idx();
    adv2 = lvl_adv2[ri];
    rev = lvl_revert[ri];
    st = cube_st[q_r][q_c];
    if (st < 2) {
        if (adv2 && st == 0) st = 1;
        else st = 2;
        cube_st[q_r][q_c] = st;
        draw_cube_top(q_r, q_c);
        add_score(25);
        if (st == 2) {
            lit_count = lit_count + 1;
            PlaySound(SND_LIT);
            if (lit_count >= 28) {
                begin_bonus();
                return;
            }
        } else {
            PlaySound(SND_HOP);
        }
    } else {
        /* already at target */
        if (rev == 1) {
            cube_st[q_r][q_c] = 0;
            lit_count = lit_count - 1;
            draw_cube_top(q_r, q_c);
        } else if (rev == 2) {
            cube_st[q_r][q_c] = 1;
            lit_count = lit_count - 1;
            draw_cube_top(q_r, q_c);
        }
    }
}

/* ================= Collisions ================= */

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

/* ================= Q*Bert ================= */

static void q_begin_hop(s8 nr, s8 nc) {
    q_nr = nr; q_nc = nc;
    q_valid = on_board(nr, nc);
    q_hf = 0;
    q_state = Q_HOP;
    if (nr > (s8)q_r) {
        if (nc > (s8)q_c) { q_sx = 1; q_pose = POSE_DR; }
        else { q_sx = -1; q_pose = POSE_DL; }
        q_sy = 2;
    } else {
        if (nc < (s8)q_c) { q_sx = -1; q_pose = POSE_UL; }
        else { q_sx = 1; q_pose = POSE_UR; }
        q_sy = -2;
    }
    hop_alt = hop_alt + 1;
    if (hop_alt >= 3) hop_alt = 0;
    if (hop_alt == 0) PlaySound(SND_HOP);
    else if (hop_alt == 1) PlaySound(SND_HOP2);
    else PlaySound(SND_HOP3);
}

static void q_input(void) {
    u8 u, d, l, r, fresh;
    if (q_state != Q_IDLE) return;
    if (death_timer > 0) return;
    fresh = pad_press & (J_UP | J_DOWN | J_LEFT | J_RIGHT);
    if (q_rest > 0 && !fresh) return;
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
    if (q_rest > 0) q_rest = q_rest - 1;
    if (q_state == Q_HOP) {
        q_px = q_px + (s16)q_sx;
        q_py = q_py + (s16)q_sy;
        q_hf = q_hf + 1;
        if (q_hf >= HOP_FRAMES) {
            if (q_valid) {
                q_r = (u8)q_nr; q_c = (u8)q_nc;
                snap_q();
                q_state = Q_IDLE;
                q_rest = 6;
                land_on_cube();
                if (state != STATE_GAME) return;
                check_collisions();
            } else {
                /* off the pyramid: is there a disc at that spot? */
                u8 di, caught;
                caught = 0;
                for (di = 0; di < 2; di++) {
                    if (!disc_on[di]) continue;
                    if (q_nr != (s8)disc_gr[di]) continue;
                    if (di == 0 && q_nc == -1) caught = 1;
                    if (di == 1 && q_nc == (s8)(disc_gr[1] + 1)) caught = 1;
                    if (caught) {
                        disc_ride = (u8)(di + 1);
                        lure_r = q_nr;
                        lure_c = q_nc;
                        q_px = (s16)disc_x(di);
                        q_py = (s16)disc_y(di) - 16;
                        q_state = Q_RIDE;
                        PlaySound(SND_DISC);
                        break;
                    }
                }
                if (!caught) {
                    q_state = Q_FALL;
                    PlaySound(SND_FALL);
                }
            }
        }
    } else if (q_state == Q_RIDE) {
        /* glide up beside the pyramid to above the summit */
        if (q_px < 72) q_px = q_px + 1;
        else if (q_px > 72) q_px = q_px - 1;
        if (q_py > -4) q_py = q_py - 1;
        if (q_px == 72 && q_py <= -4) {
            u8 di;
            di = (u8)(disc_ride - 1);
            disc_on[di] = 0;
            disc_ride = 0;
            q_r = 0; q_c = 0;
            snap_q();
            q_state = Q_IDLE;
            q_rest = 6;
            land_on_cube();
            if (state != STATE_GAME) return;
            check_collisions();
        }
    } else if (q_state == Q_FALL) {
        q_py = q_py + 5;
        if (q_py > 160) {
            kill_qbert(1);
            q_state = Q_IDLE;
        }
    }
}

/* ================= Enemies ================= */

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

static void snap_enemy(u8 i) {
    e_px[i] = (s16)cube_x(e_r[i], e_c[i]);
    if (e_type[i] == E_COILY) e_py[i] = (s16)cube_y(e_r[i]) - 16;
    else e_py[i] = (s16)cube_y(e_r[i]) - 8;
}

static void spawn_enemy(void) {
    u8 i, slot, found;
    found = 0; slot = 0;
    for (i = 0; i < MAX_ENEMIES; i++) {
        if (e_type[i] == E_NONE) { slot = i; found = 1; break; }
    }
    if (!found) return;
    if (!coily_alive && cheap_rand(3) == 0) {
        e_type[slot] = E_EGG;
        coily_alive = 1;
    } else {
        e_type[slot] = E_BALL;
    }
    e_r[slot] = 1;
    e_c[slot] = cheap_rand(2);
    e_hop[slot] = 0;
    e_fall[slot] = 0;
    snap_enemy(slot);
    e_wait[slot] = enemy_delay(50);
}

static void coily_choose(u8 i) {
    s8 r, c, nr, nc, tr, tc;
    r = (s8)e_r[i];
    c = (s8)e_c[i];
    /* when Q*Bert is riding a disc, Coily blindly chases the bait cell */
    if (q_state == Q_RIDE) {
        tr = lure_r;
        tc = lure_c;
        if (tr < r) {
            nr = r - 1;
            if (tc >= c) nc = c;
            else nc = c - 1;
        } else if (tr > r) {
            nr = r + 1;
            if (tc > c) nc = c + 1;
            else nc = c;
        } else {
            nr = r - 1;
            if (tc >= c) nc = c;
            else nc = c - 1;
        }
        /* no board clamp: over the edge he goes */
        e_begin_hop(i, nr, nc);
        return;
    }
    if ((s8)q_r < r) {
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
                if (e_type[i] == E_COILY) {
                    add_score(500);
                    coily_alive = 0;
                }
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
                    if (e_type[i] == E_EGG && e_r[i] == 6) {
                        e_type[i] = E_COILY;
                        e_wait[i] = 60;
                    } else if (e_type[i] == E_COILY) {
                        e_wait[i] = enemy_delay(42);
                    } else {
                        e_wait[i] = enemy_delay(50);
                    }
                    snap_enemy(i);
                    check_collisions();
                } else {
                    e_fall[i] = 1;
                    if (e_type[i] == E_COILY) PlaySound(SND_LURE);
                }
            }
            continue;
        }

        if (e_wait[i] > 0) {
            e_wait[i] = e_wait[i] - 1;
            continue;
        }

        if (e_type[i] == E_COILY) {
            coily_choose(i);
        } else {
            nr = (s8)e_r[i] + 1;
            if (cheap_rand(2)) nc = (s8)e_c[i] + 1;
            else nc = (s8)e_c[i];
            e_begin_hop(i, nr, nc);
        }
    }
}

/* ================= Frame update ================= */

void game_update(void) {
    if (death_timer > 0) {
        death_timer = death_timer - 1;
        if (death_timer == 0) {
            after_death();
            if (state != STATE_GAME) return;
        }
        draw_qbert();
        draw_enemies();
        draw_discs();
        return;
    }

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
    draw_discs();
}
