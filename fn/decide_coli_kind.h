/* decide_coli_kind: i960 0x27BBC, PS2 0x1DC200. Classify the contact between
 * the fighters at 0x500804 (g7) and 0x500808 (g8) from their +0x1A4 flags into
 * g6 (bit 0 both held, 1 one attacking, 2 both, 3 either flag 14, 4-7 a
 * guard/hit pair), pick the 32-byte ROM record at 0x279B8.. for that case and
 * copy it to g13+0xC0..0xD0, +0xC8, +0x88, +0x264, +0x268. A record whose
 * split (+8) is 9.9 is worked out instead by COP 0x5A from |g13+0x27C| and
 * |g13+0x280|. */
#pragma once
#include "decide_dir.h"            /* colig_cmpr */

static uint32_t colig_kind_g6(uint32_t g7, uint32_t g8, uint32_t r7, uint32_t r8) {
    uint32_t g6 = 0, r9 = r7 | r8;
    if (r9 & 0x40000u)
        g6 = 1u;
    else if ((r7 ^ r8) & 0x100u) {
        if (ages_ld8(g7 + 0x820u) != 41u && ages_ld8(g8 + 0x820u) != 41u) g6 = 2u;
    } else if (r7 & r8 & 0x100u)
        g6 = 4u;
    if (r9 & 0x4000u) g6 |= 8u;
    return g6;
}

/* A throw pair: a (flag 16 without 15) on d with flag 22; d's +0x804 bit 12
 * or 4 sets g6 bit lo and takes the pair's record, else g6 bit lo << 1. */
static int colig_kind_guard(uint32_t ra, uint32_t rd, uint32_t d, uint32_t lo) {
    if ((ra & 0x8000u) || !(rd & 0x400000u)) return 0;
    if (ages_ld32(d + 0x804u) & 0x1010u) {
        AGES_G(6) |= lo;
        return 1;
    }
    AGES_G(6) |= lo << 1;
    return 0;
}

/* The one-attacker case: rec unless the defender's part guard is under 0.1. */
static uint32_t colig_kind_hit(uint32_t def, uint32_t tbl, uint32_t rec, uint32_t alt) {
    uint32_t i = (ages_ld32(AGES_G(13) + 0x1E0u) >> 13) & 7u;
    uint32_t v = ages_ld32(def + 0x650u + ages_ld32(tbl + i * 4u));
    return colig_cmpr(0x3DCCCCCDu, v) == 4u ? rec : alt;
}

static uint32_t colig_kind_rec(uint32_t g7, uint32_t g8, uint32_t r7, uint32_t r8) {
    uint32_t g6 = AGES_G(6), rec = 0x279B8u;
    if (g6 & 5u) return rec;
    if (r7 & 0x10000u) {
        if (colig_kind_guard(r7, r8, g8, 0x40u)) return 0x27A98u;
    } else if (r8 & 0x10000u) {
        if (colig_kind_guard(r8, r7, g7, 0x10u)) return 0x27AB8u;
    } else if (g6 & 2u) {
        if (r7 & 0x100u) return colig_kind_hit(g8, 0x29118u, 0x27A18u, 0x27A58u);
        return colig_kind_hit(g7, 0x290F8u, 0x27A38u, 0x27A78u);
    } else {
        if ((r7 ^ r8) & 0x10u) return (r7 & 0x10u) ? 0x27AD8u : 0x27AF8u;
        return rec;
    }
    if (!(r7 & 0x10000u)) return (r8 & 0x8000u) ? 0x27B38u : 0x279F8u;
    if (!(r8 & 0x10000u)) return (r7 & 0x8000u) ? 0x27B18u : 0x279D8u;
    return rec;
}

static uint32_t afn_decide_coli_kind(int entry) {
    uint32_t g7 = ages_ld32(0x500804u), g8 = ages_ld32(0x500808u), g13 = AGES_G(13);
    uint32_t r7 = ages_ld32(g7 + 0x1A4u), r8 = ages_ld32(g8 + 0x1A4u);
    AGES_G(7) = g7;
    AGES_G(8) = g8;
    AGES_G(6) = colig_kind_g6(g7, g8, r7, r8);
    uint32_t rec = colig_kind_rec(g7, g8, r7, r8);
    ages_st32(g13 + 0xC0u, ages_ld32(rec));
    ages_st32(g13 + 0xC4u, ages_ld32(rec + 4u));
    uint32_t r10 = ages_ld32(rec + 8u), r11 = ages_ld32(rec + 0xCu);
    uint32_t cc = colig_cmpr(0x411E6666u, r10);       /* 9.9f */
    if (cc == 2u) {
        r10 = ages_ld32(g13 + 0x27Cu) & 0x7FFFFFFFu;
        r11 = ages_ld32(g13 + 0x280u) & 0x7FFFFFFFu;
        cc = (r10 | r11) ? 4u : 2u;
        if (r10 | r11) {
            ages_cop_w(0x2D005A5Au);
            ages_cop_w(r10);
            ages_cop_w(r11);
            r10 = ages_cop_r();
            r11 = ages_cop_r();
        }
    }
    ages_st32(g13 + 0xCCu, r10);
    ages_st32(g13 + 0xD0u, r11);
    ages_st32(g13 + 0xC8u, ages_ld32(rec + 0x10u));
    ages_st32(g13 + 0x88u, ages_ld32(rec + 0x14u));
    ages_st32(g13 + 0x264u, ages_ld32(rec + 0x18u));
    ages_st32(g13 + 0x268u, ages_ld32(rec + 0x1Cu));
    AGES_AC = (AGES_AC & ~7u) | cc;
    if (entry) ages_i960_ret();
    return 0;
}
