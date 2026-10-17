/* area_coli: i960 0x28750, PS2 0x1DCEC0. The arena edge for fighter g7 (g8
 * the other): COP 0x70 (x/z, area, floor, side) fills g13+0x118..0x124 and
 * g7's +0x670, +0x724, +0x660, +0xA58, +0x650 and +0x614..; from those the
 * edge push +0x644 / +0x64C, which a finish wall (0x574004) or the mode can
 * cancel; COP 0x2B gives +0x680; the wall it touches is classed into
 * g13+0x1F0.. and +0xAE2; and a fighter pushed over the ring edge is rung
 * out (skill score, timers, sound 0xAE116B through check_same_sound and
 * sound_request_u). The PS2 calls its own sound driver; this queues the
 * board's request. */
#pragma once
#include "decide_dir.h"            /* colig_ FPU */
#include "finish_wall_calc_pos.h"
#include "check_same_sound.h"
#include "sound_request_u.h"

/* The edge query, COP 0x70, and its replies. */
static void colig_ac_query(uint32_t g7, uint32_t g8) {
    uint32_t g13 = AGES_G(13);
    for (int i = 0; i < 3; i++) ages_cop_w(0x09801313u);
    (void)ages_cop_r();
    ages_cop_w(0x38007070u);
    ages_cop_w(ages_ld32(g7 + 0x1Cu));
    ages_cop_w(ages_ld32(0x50A010u));
    ages_cop_w(ages_ld32(0x50A00Cu));
    uint32_t floor = colig_subr(0x3E428F5Cu, ages_ld32(0x50027Cu));     /* 0.19f */
    if (!(ages_ld8(g8 + 0x7D6u) & 1u) && !(ages_ld32(g7 + 0x720u) & 0x200u))
        floor = colig_addr(0x41200000u, floor);                         /* 10.0f */
    ages_cop_w(floor);
    ages_cop_w(ages_ld8(g7 + 4u) | (ages_ld8(g8 + 0x7D6u) ? 0x80000000u : 0));
    for (uint32_t o = 0x118u; o <= 0x124u; o += 4u) ages_st32(g13 + o, ages_cop_r());
    ages_st32(g7 + 0x670u, ages_cop_r());
    ages_st32(g7 + 0x724u, ages_cop_r());
    static const uint16_t q[3] = { 0x660u, 0xA58u, 0x650u };
    for (int k = 0; k < 3; k++)
        for (uint32_t i = 0; i < 4u; i++) ages_st32(g7 + q[k] + i * 4u, ages_cop_r());
    static const uint16_t o4[4] = { 0x614u, 0x674u, 0x624u, 0x618u };
    for (int i = 0; i < 4; i++) ages_st32(g7 + o4[i], ages_cop_r());
}

static void colig_ac_negate(uint32_t a) {
    ages_st32(a, colig_mulr(0xBF800000u, ages_ld32(a)));
}

/* The edge push +0x644 / +0x64C from the replies. */
static void colig_ac_push(uint32_t g7, uint32_t g8) {
    if (ages_ld8(g7 + 0xA29u) == 0) ages_st32(g7 + 0x624u, 0);
    if ((ages_ld8(g8 + 0x7D6u) & 1u) && ages_ld8(g7 + 0xA29u) >= 16u) {
        for (uint32_t i = 0; i < 4u; i++)
            ages_st32(g7 + 0x660u + i * 4u, ages_ld32(g7 + 0xA58u + i * 4u));
        colig_ac_negate(g7 + 0x660u);
        colig_ac_negate(g7 + 0x668u);
    } else {
        colig_ac_negate(g7 + 0x664u);
        colig_ac_negate(g7 + 0x66Cu);
    }
    ages_st32(g7 + 0x644u, colig_addr(ages_ld32(g7 + 0x660u), ages_ld32(g7 + 0x664u)));
    ages_st32(g7 + 0x64Cu, colig_addr(ages_ld32(g7 + 0x668u), ages_ld32(g7 + 0x66Cu)));
}

static void colig_ac_clear_push(uint32_t g7) {
    ages_st32(g7 + 0x644u, 0);
    ages_st32(g7 + 0x64Cu, 0);
}

static void colig_ac_set574(uint32_t bit) {
    ages_st32(0x574004u, ages_ld32(0x574004u) | (1u << (bit & 31u)));
}

/* g7 in a plain hit-reaction state (flag 4 without 16, not motion 0x31E). */
static int colig_ac_reacting(uint32_t g7) {
    uint32_t f = ages_ld32(g7 + 0x1A4u);
    return (f & 0x10u) && !(f & 0x10000u) && ages_ld16s(g7 + 0x1A8u) != 0x31E;
}

/* Blown into the wall hard enough (COP 0x27 of |speed|, over 4096): the
 * finish wall is set up, once. Otherwise the speed on that axis turns. */
static void colig_ac_wall_hit(uint32_t g7, int on_x) {
    uint32_t r4 = ages_ld32(g7 + 0x2Cu), r5 = ages_ld32(g7 + 0x34u);
    ages_cop_w(0x13802727u);
    ages_cop_w((on_x ? r5 : r4) & 0x7FFFFFFFu);
    ages_cop_w((on_x ? r4 : r5) & 0x7FFFFFFFu);
    if (ages_cop_r() <= 0x1000u) {
        if (on_x) ages_st32(g7 + 0x2Cu, r4 ^ 0x80000000u);
        else      ages_st32(g7 + 0x34u, r5 ^ 0x80000000u);
        return;
    }
    colig_ac_clear_push(g7);
    if (ages_ld32(0x574004u) & 0x10000000u) return;
    ages_st32(0x574004u, 0);
    colig_ac_set574(27);
    colig_ac_set574(28);
    colig_ac_set574(29);
    colig_ac_set574(ages_ld32(g7 + 4u) + 30u);
    afn_finish_wall_calc_pos(0);
    colig_ac_set574(24);
    uint32_t x, z;
    colig_finish_pos(&x, &z);
    ages_st32(0x57400Cu, x);
    ages_st32(0x574010u, z);
}

static void colig_ac_finish(uint32_t g7, uint32_t g8) {
    if (ages_ld32(0x574004u) & 0x10000000u) {
        if (!(ages_ld8(g7 + 0x7D6u) & 1u) && colig_ac_reacting(g7)) colig_ac_clear_push(g7);
        return;
    }
    if (!(ages_ld8(g8 + 0x7D6u) & 1u)) return;
    uint32_t k = ages_ld8(g8 + 0x874u);
    if (k == 1u) { colig_ac_set574(26); return; }
    if (k != 3u || !colig_ac_reacting(g7)) return;
    if (ages_ld32(g7 + 0x644u) != 0)      colig_ac_wall_hit(g7, 1);
    else if (ages_ld32(g7 + 0x64Cu) != 0) colig_ac_wall_hit(g7, 0);
}

/* The finish walls of stages that have them stop the push into them. r9 is
 * area_coli's r9 (0 in a fresh frame), which the wall class can read. */
static uint32_t colig_ac_stage_walls(uint32_t g7) {
    if (ages_ld32(0x28F80u + ages_ld8(0x500064u) * 4u) == 0) return 0;
    if (!(ages_ld8(g7 + 0x7D6u) & 1u)) return 0;
    uint32_t w = ages_ld32(0x574004u) & 15u;
    if (w == 0) return 0;
    uint32_t r7 = ages_ld32(g7 + 0x644u), r8 = ages_ld32(g7 + 0x64Cu);
    for (uint32_t r9 = 0x29000u; r9 < 0x29040u; r9 += 16u) {
        if (!((w >> (ages_ld32(r9) & 31u)) & 1u)) continue;
        uint32_t r12 = ages_ld32(r9 + 4u), r13 = ages_ld32(r9 + 8u);
        if (r12 != 0) {
            if (!((r12 ^ r7) & 0x80000000u)) r7 = 0;
        } else if (r13 != 0 && !((r13 ^ r8) & 0x80000000u)) {
            r8 = 0;
        }
    }
    ages_st32(g7 + 0x644u, r7);
    ages_st32(g7 + 0x64Cu, r8);
    return 0x29040u;
}

/* Mode 3 / 5 has no edge push; an edge push zeroes the limit on its side. */
static void colig_ac_limits(uint32_t g7) {
    if (ages_ld8(0x50002Bu) == 3u && ages_ld8(0x500031u) == 5u) colig_ac_clear_push(g7);
    uint32_t x = ages_ld32(g7 + 0x644u), z = ages_ld32(g7 + 0x64Cu);
    if (x != 0) ages_st32(g7 + 0x650u + (x >> 31) * 4u, 0);
    if (z != 0) ages_st32(g7 + 0x658u + (z >> 31) * 4u, 0);
    ages_cop_w(0x15802B2Bu);
    ages_cop_w(ages_ld32(g7 + 0x1F4u));
    ages_cop_w(0);
    ages_cop_w(ages_ld32(g7 + 0x1FCu));
    ages_cop_w(0);
    ages_st32(g7 + 0x680u, ages_cop_r());
}

/* The wall class of the larger edge push (x when on_x): its side and heading
 * octant through the ROM table at 0x290E8. Returns it with bit 3 cleared. */
static uint32_t colig_ac_class(uint32_t g7, uint32_t idx, uint32_t push, int on_x) {
    uint32_t g13 = AGES_G(13), r4 = (uint32_t)ages_ld16s(g7 + 0x26u), side;
    ages_st16(g13 + 0x1F0u + idx * 2u, r4);
    if (on_x) {
        side = ((r4 ^ push) >> 28) & 8u;
    } else {
        r4 -= 0x4000u;
        side = ((r4 ^ (push >> 16)) >> 12) & 8u;
    }
    ages_st32(g7 + 0xA08u, ages_ld32(g7 + 0xA08u) | side << 8);
    side |= ages_ld8(0x290E8u + (((r4 & 0x7000u) >> 12) | side));
    ages_st16(g13 + 0x23Cu + idx * 2u, 2u);
    ages_st16(g13 + 0x1F4u + idx * 2u, side);
    side &= ~8u;
    ages_st16(g7 + 0xAE2u, side);
    return side;
}

/* Returns the class the order byte reads: r9 (z wall) or r12 (x wall, else
 * |+0x64C|), per which limit is the smallest. *r9 / *r12 are area_coli's. */
static void colig_ac_wall(uint32_t g7, uint32_t idx, uint32_t *r9, uint32_t *r12) {
    uint32_t x = ages_ld32(g7 + 0x644u), ax = x & 0x7FFFFFFFu;
    *r12 = ages_ld32(g7 + 0x64Cu) & 0x7FFFFFFFu;
    if (ax != 0 && (*r12 == 0 || colig_cmpr(ax, *r12) != 4u)) {
        *r12 = colig_ac_class(g7, idx, x, 1);
        return;
    }
    uint32_t z = ages_ld32(g7 + 0x64Cu), az = z & 0x7FFFFFFFu;
    if (az == 0 || colig_cmpr(az, ages_ld32(g7 + 0x644u) & 0x7FFFFFFFu) == 4u) return;
    *r9 = colig_ac_class(g7, idx, z, 0);
}

/* The smallest of rob's limits +0x650..+0x65C; *at = its index. */
static uint32_t colig_ac_min_limit(uint32_t rob, uint32_t *at) {
    uint32_t r4 = ages_ld32(rob + 0x650u), r10 = 0;
    for (uint32_t i = 1; i < 4u; i++) {
        uint32_t r6 = ages_ld32(rob + 0x650u + i * 4u);
        if (colig_cmpr(r4, r6) != 4u) { r10 = i; r4 = r6; }
    }
    *at = r10;
    return r4;
}

static void colig_ac_order(uint32_t g7, uint32_t idx, uint32_t r9, uint32_t r12) {
    uint32_t g13 = AGES_G(13), r10, r11;
    colig_ac_min_limit(g7, &r10);
    if (!(r10 & 2u)) r9 = r12;
    ages_st16(g13 + 0x1F8u + idx * 2u, (r9 & 7u) | r10 << 4);
    uint32_t r7 = ages_ld8(g7 + 4u);
    uint32_t other = ages_ld32(0x500804u + ((~r7) & 1u) * 4u);
    ages_st32(g13 + 0x204u + idx * 4u, colig_ac_min_limit(other, &r10));
    if (r7 & 1u) r10 = (r10 & 2u) | (~r10 & 1u);
    uint32_t a = ages_ld32(g13 + 0x1E0u);
    r10 |= (((a + a) ^ a) >> 12) & 4u;
    r10 |= (uint32_t)ages_ld16s(0x290D8u + (a >> 13) * 2u);
    r11 = (uint32_t)ages_ld16s(0x290B8u + r10 * 2u);
    ages_st16(g13 + 0x1FCu + idx * 2u, r10);
    ages_st16(g13 + 0x200u + idx * 2u, r11);
}

/* Over the edge at a corner-free spot of the wall (|x| or |z| within 0.15 of
 * the area half-size 0x50A00C): the fighter is put just outside (+0.1). */
static int colig_ac_ring_pos(uint32_t g7) {
    uint32_t r4 = ages_ld32(g7 + 0xA4Cu), r5 = ages_ld32(g7 + 0xA50u), r6 = ages_ld32(g7 + 0xA54u);
    if (colig_cmpr(colig_subr(0x3ECCCCCDu, ages_ld32(0x50027Cu)), r5) == 1u) return 0;
    uint32_t r10 = ages_ld32(0x50A00Cu);
    uint32_t r11 = colig_subr(0x3E19999Au, r10), r12 = colig_addr(0x3E19999Au, r10);
    uint32_t o = 0x18u;
    r4 &= 0x7FFFFFFFu;
    r6 &= 0x7FFFFFFFu;
    r10 = colig_addr(0x3DCCCCCDu, r10);
    if (colig_cmpr(r4, r11) == 4u || colig_cmpr(r4, r12) == 1u) {
        if (colig_cmpr(r6, r11) == 4u || colig_cmpr(r6, r12) == 1u) return 0;
        o = 0x20u;
    }
    ages_st32(g7 + o, r10 | (ages_ld32(g7 + o) & 0x80000000u));
    return 1;
}

static void colig_ac_ring_out(uint32_t g7, uint32_t g8) {
    if (((ages_ld32(g7 + 0x644u) | ages_ld32(g7 + 0x64Cu)) & 0x7FFFFFFFu) == 0) return;
    if (!(ages_ld8(g8 + 0x7D6u) & 1u) || (ages_ld32(g7) & 8u)) return;
    uint32_t cls = (uint32_t)ages_ld16s(AGES_G(13) + 0x1F4u + ages_ld8(g7 + 4u) * 2u) & ~8u;
    if (cls > 1u) return;
    if (colig_cmpr(0xBDA3D70Au, ages_ld32(g7 + 0x30u)) & 6u) return;   /* -0.08f */
    if (!colig_ac_ring_pos(g7)) return;
    AGES_G(9) = 0x7D0u;
    afn_total_skill_adder_g8(0);
    ages_st32(g7 + 0x1Cu, ages_ld32(0x50027Cu));
    ages_st16(g7 + 0xA0Eu, 0x2AF8u);
    ages_st16(g7 + 0x852u, ages_ld32(0x28FC0u + ages_ld8(0x500064u) * 4u));
    ages_st8(g7 + 0x60Cu, 31u);
    ages_st32(g7 + 0xAE4u, ages_ld32(g7 + 0x18u));
    ages_st32(g7 + 0xAE8u, ages_ld32(g7 + 0x20u));
    colig_ac_clear_push(g7);
    AGES_G(0) = 0xAE116Bu;
    afn_check_same_sound(0);
    afn_sound_request_u(0);
}

/* The ring-out timer +0xA0E: held (flag 19) it pays the skill bonus once it
 * runs down to 1000; over 1000 the fighter stays where it went out. */
static void colig_ac_timer(uint32_t g7, uint32_t g8) {
    uint32_t t = (uint32_t)ages_ld16s(g7 + 0xA0Eu);
    if (ages_ld32(g7 + 0x1A4u) & 0x80000u) {
        if (t < 1000u) return;
        AGES_G(9) = 0xBB8u;
        afn_total_skill_adder_g8(0);
        ages_st16(g7 + 0xA0Eu, 0);
    } else if (t > 1000u) {
        ages_st32(g7 + 0x18u, ages_ld32(g7 + 0xAE4u));
        ages_st32(g7 + 0x20u, ages_ld32(g7 + 0xAE8u));
        colig_ac_clear_push(g7);
    } else {
        colig_ac_ring_out(g7, g8);
    }
}

static uint32_t afn_area_coli(int entry) {
    uint32_t g7 = AGES_G(7), g8 = AGES_G(8), r9 = 0, r12 = 0;
    colig_ac_query(g7, g8);
    colig_ac_push(g7, g8);
    if (ages_ld16s(g7 + 0xA0Eu) == 0) colig_ac_finish(g7, g8);
    if (ages_ld16s(g7 + 0xA0Eu) == 0) r9 = colig_ac_stage_walls(g7);
    colig_ac_limits(g7);
    uint32_t idx = ages_ld8(g7 + 4u);
    colig_ac_wall(g7, idx, &r9, &r12);
    colig_ac_order(g7, idx, r9, r12);
    colig_ac_timer(g7, g8);
    if (entry) ages_i960_ret();
    return 0;
}
