/* set_motion: i960 0x1B5B0, PS2 0x1D4D00 (EXT 5f).
 *
 * Start motion g0 (low 13 bits; 0 stops): settle the flags at g7+0x1A4,
 * drop an option (delete_option), read the motion header (set_mot_dat), set
 * the start angles (calc_rob_angle_int) and the blend (smooth_int), then the
 * hit-stop counters at g7+0x1222/0x1246. As the board: the 0x1246 branch
 * clears 0x1224 too, not 0x1248. */
#pragma once
#include "set_mot_dat.h"
#include "set_option.h"
#include "smooth_int.h"

/* The flags before the motion is read; returns the old g7+0x1A4 (r7). */
static uint32_t smo_flags(uint32_t g7, uint32_t g0) {
    uint32_t r7 = ages_ld32(g7 + 0x1A4);
    ages_st32(g7, ages_ld32(g7) | (1u << 11));
    if (g0 & (1u << 14)) { r7 &= ~0x200060u; ages_st8(g7 + 0xBE4, 1); }
    if (g0 & (1u << 13)) { r7 &= ~0x200020u; ages_st8(g7 + 0xBE4, 1); }
    if (r7 & 0x40u)
        ages_st16(g7 + 0x26, (uint32_t)(ages_ld16s(g7 + 0x812) + ages_ld16s(g7 + 0x26)));
    if (r7 & 0x20u) {
        ages_st32(g7 + 0x18, ages_ld32(g7 + 0x1F4));
        ages_st32(g7 + 0x20, ages_ld32(g7 + 0x1FC));
        ages_st32(g7 + 0x80, 0);
        ages_st32(g7 + 0x88, 0);
    }
    if (r7 & (1u << 21)) ages_st32(g7, ages_ld32(g7) ^ 0x40u);
    if (g0 & (1u << 15)) ages_st32(g7, ages_ld32(g7) ^ 0x40u);
    return r7;
}

/* delete_option with the i960's save of g0..g14 around the call. */
static void smo_delete_option(void) {
    uint32_t g[15];
    for (int i = 0; i < 15; i++) g[i] = AGES_G(i);
    opt_delete_option();
    for (int i = 0; i < 15; i++) AGES_G(i) = g[i];
}

/* One hit-stop counter (0x1B7D8 / 0x1B824); returns the cc. */
static uint32_t smo_stop(uint32_t g7, uint32_t r8, uint32_t cnt, uint32_t dst, int second) {
    if (ages_ld32(g7) & 0x10u) { ages_st16(g7 + 0x1224, 0); return 2u; }
    if (!(r8 & 0x100u)) return 2u;
    uint32_t n = ages_ld16(g7 + cnt);
    ages_st16(g7 + cnt, 0);
    if (n == 0) { ages_st16(g7 + 0x1224, 0); return 2u; }
    ages_st16(g7 + dst, n);
    if (!second) ages_st32(g7 + 0x1228, ages_ld32(g7 + 0x122C));
    return 4u;
}

static uint32_t afn_set_motion(int entry) {
    uint32_t g7 = AGES_G(7), g0 = AGES_G(0);
    ages_st32(g7 + 0x5CC, 0);
    ages_st8(g7 + 0x60C, 0);
    ages_st32(g7, ages_ld32(g7) & ~(1u << 9));
    if (g0 == 0) {
        ages_st32(g7 + 0x1A4, ages_ld32(g7 + 0x1A4) & 0x14068u);
        ages_st16(g7 + 0x1A8, 0);
        AGES_AC = (AGES_AC & ~7u) | 2u;
        if (entry) ages_i960_ret();
        return 0;
    }
    uint32_t r7 = smo_flags(g7, g0);
    AGES_G(0) = g0 & 0x1FFFu;
    ages_st16(g7 + 0x1A8, AGES_G(0));
    ages_st16(g7 + 0x1AA, 1);
    if (ages_ld32(g7) & (1u << 27)) smo_delete_option();
    afn_set_mot_dat(0);
    uint32_t r8 = ages_ld32(g7 + 0x1A4), r4;
    if ((g0 & 0x8000u) && ((r4 = ages_ld32(g7 + 0x804)) & (1u << 21))) {
        ages_st32(g7 + 0x804, r4 | (1u << 21));
        r8 ^= 1u << 21;
    }
    if (r8 & 0x100u) ages_st16(g7 + 0x6E2, ages_ld16(g7 + 0x6E2) + 1u);
    if (r7 & 0x20u) r8 |= 0x80u;
    ages_st32(g7 + 0x1A4, r8);
    afn_calc_rob_angle_int(0);
    afn_smooth_int(0);
    smo_stop(g7, r8, 0x1222, 0x1224, 0);
    uint32_t cc = smo_stop(g7, r8, 0x1246, 0x1248, 1);
    AGES_AC = (AGES_AC & ~7u) | cc;
    if (entry) ages_i960_ret();
    return 0;
}
