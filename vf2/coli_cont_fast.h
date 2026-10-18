/* VF2 coli_cont_fast: i960 0x23524 to its `ret` at 0x23648 (PS2
 * sysCopPatchColiContFast 0x196AC0, EXT 4e; decomp/VF2/FN/ext4e_*). The
 * fight collision of the frame, with its callees: area_coli (0x2396C) for
 * each fighter, decide_coli_kind (0x233D0), then decide_dir (0x2364C) alone,
 * or calc_attack_flag (0x238A4) for each, TGP 0x3B (the narrow phase: contact
 * g13+0x144/0x148, g4, g5; g5 averaged over four frames with g13+0xEC..0xF4),
 * coli_flag_conv (0x238F8) and coli_recalc_pos (0x23694), which the board
 * reaches by `bal` and which works in this frame's registers.
 *
 * The TGP is at g11 + g12 (vf2_tgp_w/r, vf2/set_obj.h); the float maths is
 * the i960's (FV's colig_ helpers, fn/decide_dir.h). The C stops on the
 * `ret`, every local as the i960 leaves it there. */
#pragma once
#include "set_obj.h"

#define VF2_CC_KIND_TBL  0x000232C4u   /* 6-word rows: 0xB4, 0xB8, 0xC0, 0xC4, 0xBC, 0x88 */
#define VF2_CC_BALL_MAP  0x00023284u   /* 30 bytes: ball -> unit */
#define VF2_CC_DIRS      0x00023BBCu   /* 16-byte rows; their count at 0x23BB8 */

static inline uint32_t vf2_cc_bit(uint32_t w, uint32_t n) { return (w >> (n & 31u)) & 1u; }

/* 0x23878: g3's ball bits as unit bits, through the table at 0x2007B76. */
static void vf2_cc_ball_units(void)
{
    uint32_t r6 = 0;
    for (uint32_t r4 = 0; r4 < 30; r4++)
        if (vf2_cc_bit(AGES_G(3), r4)) r6 |= 1u << (ages_ld32(0x02007B76u + r4 * 4u) & 31u);
    AGES_G(3) = r6;
}

/* area_coli's ball positions: 30 balls from the fighter's table into
 * g7+0xD00, each with its radius from 0x20078A8. */
static void vf2_cc_area_balls(uint32_t g7)
{
    uint32_t r8 = ages_ld32(0x00023944u + ages_ld8(g7 + 4u) * 4u);
    for (uint32_t r9 = 0; r9 < 30; r9++) {
        uint32_t r11 = ages_ld8(0x0002394Cu + r9), q[4];
        ages_ldn(q, r8 + r9 * 12u, 3);
        q[3] = ages_ld32(0x020078A8u + 0xCu + r11 * 16u);
        ages_stn(g7 + 0xD00u + r11 * 16u, q, 4);
    }
}

/* area_coli's limits at g13+0xFC..0x108, its bit words and counters cleared. */
static void vf2_cc_area_limits(uint32_t g7, uint32_t g13)
{
    uint32_t r15 = 0x3D4CCCCDu;                        /* 0.05f */
    uint32_t r12 = ages_ld32(0x0050A010u), r11 = ages_ld32(g7 + 0x1Cu);
    uint32_t r14 = colig_addr(0, r15);
    r12 = colig_addr(r12, r15);
    r11 = colig_addr(r11, r15);
    ages_st32(g13 + 0xFCu, r14);
    ages_st32(g13 + 0x100u, 0xBDCCCCCDu);              /* -0.1f */
    ages_st32(g13 + 0x104u, r12);
    ages_st32(g13 + 0x108u, r11);
    for (uint32_t o = 0x10Cu; o < 0x13Cu; o += 4u) ages_st32(g13 + o, 0);
}

/* Bit r3 of the word at g13+0x10 past the limit at g13+lim, unless
 * s1 > the limit (cmpr; bg). */
static void vf2_cc_mark_le(uint32_t g13, uint32_t lim, uint32_t s1, uint32_t r3)
{
    uint32_t w = g13 + lim + 0x10u;
    if (colig_cmpr(s1, ages_ld32(g13 + lim)) != 1u)
        ages_st32(w, ages_ld32(w) | 1u << r3);
}

/* area_coli's walls: ball (x r4, z r6) against each direction row. */
static void vf2_cc_area_dirs(uint32_t g13, uint32_t r3, uint32_t r4, uint32_t r6)
{
    uint32_t r11 = ages_ld32(0x00023BB8u);
    do {
        r11--;
        uint32_t d[3];
        ages_ldn(d, VF2_CC_DIRS + r11 * 16u, 3);
        uint32_t r8 = colig_mulr(r4, d[0]), r9 = colig_mulr(r6, d[1]);
        uint32_t r10 = ages_ld32(0x0050A00Cu) ^ 0x80000000u;
        r10 = colig_addr(r10, r8);
        r10 = colig_addr(r10, r9);
        if ((int32_t)r10 >= 0) {
            AGES_G(4) |= 1u << r3;
            uint32_t a = g13 + 0x11Cu + r11 * 4u;
            ages_st32(a, ages_ld32(a) + 1u);
        } else if (vf2_cc_bit(ages_ld32(g13 + 0x110u), r3)) {
            uint32_t a = g13 + 0x12Cu + r11 * 4u;
            if (colig_cmpr(r10, ages_ld32(a)) != 1u) ages_st32(a, r10);
        }
    } while ((int32_t)r11 > 0);
}

/* area_coli's push: the direction with the most balls out, times its depth. */
static void vf2_cc_area_push(uint32_t g7, uint32_t g13)
{
    uint32_t r11 = ages_ld32(0x00023BB8u), r6 = r11, r3 = 0;
    do {
        r11--;
        uint32_t r15 = ages_ld32(g13 + 0x11Cu + r11 * 4u);
        if (!((int32_t)r3 >= (int32_t)r15)) { r3 = r15; r6 = r11; }
    } while ((int32_t)r11 > 0);
    uint32_t r7 = ages_ld32(g13 + 0x12Cu + r6 * 4u);
    uint32_t r4 = ages_ld32(VF2_CC_DIRS + r6 * 16u), r5 = ages_ld32(VF2_CC_DIRS + r6 * 16u + 4u);
    ages_st32(g7 + 0x644u, colig_mulr(r7, r4));
    ages_st32(g7 + 0x64Cu, colig_mulr(r7, r5));
}

/* area_coli (0x2396C): fighter g7's balls against the ring and its walls. */
static void vf2_cc_area_coli(void)
{
    uint32_t g7 = AGES_G(7), g13 = AGES_G(13);
    vf2_cc_area_balls(g7);
    vf2_cc_area_limits(g7, g13);
    AGES_G(4) = 0;
    for (uint32_t r3 = 0; r3 < 30; r3++) {
        uint32_t q[4];
        ages_ldn(q, g7 + 0xD00u + r3 * 16u, 4);
        uint32_t r7 = colig_subr(q[3], q[1]), r5 = colig_addr(q[3], q[1]);
        vf2_cc_mark_le(g13, 0xFCu, r7, r3);
        vf2_cc_mark_le(g13, 0x100u, r5, r3);
        vf2_cc_mark_le(g13, 0x104u, r7, r3);
        if (!vf2_cc_bit(ages_ld32(g7), 2) && !vf2_cc_bit(ages_ld32(g7 + 0x1A4u), 23))
            vf2_cc_mark_le(g13, 0x108u, r7, r3);
        vf2_cc_area_dirs(g13, r3, q[0], q[2]);
    }
    AGES_G(3) = ages_ld32(g13 + 0x114u) & AGES_G(4);
    vf2_cc_ball_units();
    ages_st16(g7 + 0x624u, AGES_G(3));
    AGES_G(4) = ~AGES_G(4);
    AGES_G(3) = ages_ld32(g13 + 0x10Cu) & AGES_G(4);
    vf2_cc_ball_units();
    ages_st16(g7 + 0x614u, AGES_G(3));
    AGES_G(3) = ages_ld32(g13 + 0x118u);
    vf2_cc_ball_units();
    ages_st16(g7 + 0x618u, AGES_G(3));
    vf2_cc_area_push(g7, g13);
}

/* decide_coli_kind's g6 bits (0x23398, 0x233FC, 0x23434). */
static void vf2_cc_kind_bits(uint32_t g7, uint32_t g8, uint32_t r7, uint32_t r8)
{
    uint32_t r9 = r7 | r8, r10 = r7 & r8, r11 = r7 ^ r8;
    uint32_t r3 = ages_ld16(g7 + 0x1A8u), r4 = ages_ld16(g8 + 0x1A8u);
    AGES_G(6) = 0;
    if (r3 == 0x242u || r4 == 0x242u || r3 == 0x241u || r4 == 0x241u || r3 == 0x6Cu || r4 == 0x6Cu)
        AGES_G(6) |= 2u;
    else if (vf2_cc_bit(r9, 18))
        AGES_G(6) |= 1u;
    else if (vf2_cc_bit(r11, 8)) {
        if (ages_ld8(g7 + 0x820u) != 0x29u && ages_ld8(g8 + 0x820u) != 0x29u) AGES_G(6) |= 2u;
    } else if (vf2_cc_bit(r10, 8))
        AGES_G(6) |= 4u;
    if (vf2_cc_bit(r9, 14)) AGES_G(6) |= 8u;
}

/* 0x23450 / 0x2347C: a throw by `a` on `b` (bits 16, 15, 22, b+0x804 12/4). */
static bool vf2_cc_throw(uint32_t ra, uint32_t rb, uint32_t b)
{
    if (vf2_cc_bit(ra, 15) || !vf2_cc_bit(rb, 22)) return false;
    return vf2_cc_bit(ages_ld32(b + 0x804u), 12) || vf2_cc_bit(ages_ld32(b + 0x804u), 4);
}

/* decide_coli_kind's row: 0x23440..0x234EC. */
static uint32_t vf2_cc_kind_row(uint32_t g7, uint32_t g8, uint32_t r7, uint32_t r8)
{
    uint32_t g6 = AGES_G(6);
    if (g6 & 1u || g6 & 4u) return VF2_CC_KIND_TBL;
    if (vf2_cc_bit(r7, 16)) {
        if (vf2_cc_throw(r7, r8, g8)) return 0x0002333Cu;
    } else if (vf2_cc_bit(r8, 16)) {
        if (vf2_cc_throw(r8, r7, g7)) return 0x00023354u;
    } else {
        if (!(g6 & 2u)) return VF2_CC_KIND_TBL;
        return vf2_cc_bit(r7, 8) ? 0x0002330Cu : 0x00023324u;
    }
    if (!vf2_cc_bit(r7, 16)) return 0x000232F4u;
    if (!vf2_cc_bit(r8, 16)) return 0x000232DCu;
    return VF2_CC_KIND_TBL;
}

/* decide_coli_kind (0x233D0): g6, and the kind's row into g13. */
static void vf2_cc_decide_coli_kind(void)
{
    uint32_t g7 = AGES_G(7) = ages_ld32(0x00500804u);
    uint32_t g8 = AGES_G(8) = ages_ld32(0x00500808u);
    uint32_t r7 = ages_ld32(g7 + 0x1A4u), r8 = ages_ld32(g8 + 0x1A4u);
    vf2_cc_kind_bits(g7, g8, r7, r8);
    uint32_t r12 = vf2_cc_kind_row(g7, g8, r7, r8), g13 = AGES_G(13);
    static const uint8_t to[6] = { 0xB4, 0xB8, 0xC0, 0xC4, 0xBC, 0x88 };
    for (int i = 0; i < 6; i++) ages_st32(g13 + to[i], ages_ld32(r12 + 4u * (uint32_t)i));
}

/* decide_dir (0x2364C): TGP 0x30, the unit x/z vector g7 -> g8, to g13+0xC8. */
static void vf2_cc_decide_dir(void)
{
    uint32_t g7 = AGES_G(7), g8 = AGES_G(8), g13 = AGES_G(13);
    uint32_t r12 = colig_subr(ages_ld32(g7 + 0x1F4u), ages_ld32(g8 + 0x1F4u));
    uint32_t r14 = colig_subr(ages_ld32(g7 + 0x1FCu), ages_ld32(g8 + 0x1FCu));
    vf2_tgp_w(0x18003030u);
    vf2_tgp_w(r12);
    vf2_tgp_w(0);
    vf2_tgp_w(r14);
    for (uint32_t o = 0xC8u; o <= 0xD0u; o += 4u) {
        uint32_t v = vf2_tgp_r();
        ages_st32(g13 + o, v);
    }
}

/* calc_attack_flag (0x238A4): g3 = fighter g7's attacking balls. */
static void vf2_cc_calc_attack_flag(void)
{
    uint32_t g7 = AGES_G(7);
    AGES_G(3) = 0;
    if (!vf2_cc_bit(ages_ld32(g7 + 0x1A4u), 8)) return;
    if (ages_ld16(g7 + 0x1AAu) < ages_ld16(g7 + 0x808u)) return;
    uint32_t r3 = ages_ld8(g7 + 0x820u);
    if (r3 == 0x29u) return;
    r3 = ages_ld32(0x02007ACAu + r3 * 4u);
    for (uint32_t r6 = 0; r6 < 30; r6++)
        if (vf2_cc_bit(r3, ages_ld8(VF2_CC_BALL_MAP + r6))) AGES_G(3) |= 1u << r6;
}

/* coli_flag_conv (0x238F8): the TGP's ball-pair bits (0x91F880) as unit
 * bits at g13+0x40. */
static void vf2_cc_coli_flag_conv(void)
{
    uint32_t r4 = AGES_G(13) + 0x40u;
    for (uint32_t r6 = 0; r6 < 30; r6++) {
        uint32_t r7 = ages_ld8(VF2_CC_BALL_MAP + r6), r10 = ages_ld32(0x0091F880u + r6 * 4u);
        for (uint32_t r8 = 0; r8 < 30; r8++) {
            if (!vf2_cc_bit(r10, r8)) continue;
            uint32_t a = r4 + ages_ld8(VF2_CC_BALL_MAP + r8) * 4u;
            ages_st32(a, ages_ld32(a) | 1u << (r7 & 31u));
        }
    }
}

/* coli_recalc_pos, 0x236CC / 0x23704: a thrown fighter's (rob) step,
 * capped at 0.2f in rob+0x650; every other move zero. */
static void vf2_cc_thrown_step(uint32_t *r, uint32_t rob, int slot)
{
    r[12] = ages_ld32(rob + 0x650u);
    r[15] = colig_addr(AGES_G(5), r[12]);
    r[14] = 0x3E4CCCCDu;                               /* 0.2f */
    if (colig_cmpr(r[14], r[15]) != 1u) {
        r[15] = r[14];
        AGES_G(5) = colig_subr(r[12], r[14]);
    }
    ages_st32(rob + 0x650u, r[15]);
    for (int i = 8; i <= 14; i++) if (i != 11) r[i] = 0;
    r[slot] = AGES_G(5);
}

/* coli_recalc_pos, 0x23738: the push along g13+0xC8 (r4, r6 hold the
 * pair's x/z, r8/r10 g8's, r11 an attack word). */
static void vf2_cc_push(uint32_t *r)
{
    uint32_t g13 = AGES_G(13);
    r[4] = colig_subr(r[4], r[8]);
    r[6] = colig_subr(r[6], r[10]);
    r[8] = ages_ld32(g13 + 0xC8u);
    r[10] = ages_ld32(g13 + 0xD0u);
    r[15] = 0x3E99999Au;                               /* 0.3f */
    r[9] = colig_mulr(r[8], r[15]);
    r[11] = colig_mulr(r[11], r[15]);
    r[4] = colig_subr(r[9], r[4]);
    r[6] = colig_subr(r[11], r[6]);
    r[4] = colig_mulr(r[4], r[8]);
    r[6] = colig_mulr(r[6], r[10]);
    r[4] = colig_addr(r[4], r[6]);
    if (r[4] & 0x80000000u) {
        r[4] ^= 0x80000000u;
        AGES_G(4) = colig_addr(AGES_G(4), r[4]);
    }
    r[12] = ages_ld32(g13 + 0xC8u);
    r[13] = ages_ld32(g13 + 0xCCu);
    r[14] = ages_ld32(g13 + 0xD0u);
    r[3] = colig_mulr(AGES_G(4), ages_ld32(g13 + 0xC0u) ^ 0x80000000u);
    for (int i = 0; i < 3; i++) r[8 + i] = colig_mulr(r[3], r[12 + i]);
    r[3] = colig_mulr(AGES_G(4), ages_ld32(g13 + 0xC4u));
    for (int i = 0; i < 3; i++) r[12 + i] = colig_mulr(r[3], r[12 + i]);
}

/* coli_recalc_pos's moves: r8..r10 g7's, r12..r14 g8's, less the walls'
 * push (rob+0x644/0x64C), to the TGP (0x3D) and onto the positions. */
static void vf2_cc_apply(uint32_t *r, uint32_t g7, uint32_t g8)
{
    uint32_t g13 = AGES_G(13);
    for (int i = 0; i < 3; i++) ages_st32(g13 + 0xD4u + 4u * (uint32_t)i, r[8 + i]);
    for (int i = 0; i < 3; i++) ages_st32(g13 + 0xE0u + 4u * (uint32_t)i, r[12 + i]);
    r[8]  = colig_subr(r[3] = ages_ld32(g7 + 0x644u), r[8]);
    r[10] = colig_subr(r[3] = ages_ld32(g7 + 0x64Cu), r[10]);
    r[12] = colig_subr(r[3] = ages_ld32(g8 + 0x644u), r[12]);
    r[14] = colig_subr(r[3] = ages_ld32(g8 + 0x64Cu), r[14]);
    r[15] = 0x1E803D3Du;
    vf2_tgp_w(r[15]);
    static const int send[6] = { 8, 9, 10, 12, 13, 14 };
    for (int i = 0; i < 6; i++) vf2_tgp_w(r[send[i]]);
    for (int k = 0; k < 2; k++) {
        uint32_t rob = k ? g8 : g7;
        r[4] = colig_addr(ages_ld32(rob + 0x18u), r[k ? 12 : 8]);
        r[6] = colig_addr(ages_ld32(rob + 0x20u), r[k ? 14 : 10]);
        ages_st32(rob + 0x18u, r[4]);
        ages_st32(rob + 0x20u, r[6]);
    }
}

/* coli_recalc_pos's decay: rob+0x650 less 0.03f, not below 0. CC is the
 * last `bbc 0x1f`'s. */
static void vf2_cc_decay(uint32_t *r, uint32_t rob)
{
    r[14] = 0x3CF5C28Fu;
    r[15] = colig_subr(r[14], ages_ld32(rob + 0x650u));
    uint32_t neg = r[15] >> 31;
    if (neg) r[15] = 0;
    ages_st32(rob + 0x650u, r[15]);
    AGES_AC = (AGES_AC & ~7u) | (neg ? 2u : 0u);
}

/* coli_recalc_pos (0x23694, by `bal`): in this frame's registers r. */
static void vf2_cc_recalc_pos(uint32_t *r)
{
    uint32_t g7 = AGES_G(7) = ages_ld32(0x00500804u);
    uint32_t g8 = AGES_G(8) = ages_ld32(0x00500808u);
    if (AGES_G(6) & 8u) vf2_cc_decide_dir();
    r[3] = ages_ld32(AGES_G(13) + 0x148u);
    if (r[3] == 0) {
        vf2_cc_decide_dir();
        for (int i = 8; i <= 14; i++) if (i != 11) r[i] = 0;
    } else if ((r[15] = ages_ld32(AGES_G(13) + 0x88u)) == 5u) {
        vf2_cc_thrown_step(r, g7, 9);
    } else if (r[15] == 4u) {
        vf2_cc_thrown_step(r, g8, 13);
    } else {
        vf2_cc_push(r);
    }
    vf2_cc_apply(r, g7, g8);
    vf2_cc_decay(r, g7);
    vf2_cc_decay(r, g8);
    AGES_G(14) = 0x00023648u;
}

/* 0x235AC..0x23644: the narrow phase, coli_flag_conv and coli_recalc_pos. */
static void vf2_cc_narrow(uint32_t *r)
{
    uint32_t g13 = AGES_G(13);
    AGES_G(7) = ages_ld32(0x00500804u);
    vf2_cc_calc_attack_flag();
    r[11] = AGES_G(3);
    AGES_G(7) = ages_ld32(0x00500808u);
    vf2_cc_calc_attack_flag();
    r[12] = AGES_G(3);
    r[3] = ages_ld32(g13 + 0x88u);
    r[13] = ages_ld32(g13 + 0xB4u);
    r[14] = ages_ld32(g13 + 0xB8u);
    r[15] = 0x1D803B3Bu;
    vf2_tgp_w(r[15]);
    vf2_tgp_w(r[3]);
    vf2_tgp_w(r[11]);
    vf2_tgp_w(r[12]);
    vf2_tgp_w(r[13]);
    vf2_tgp_w(r[14]);
    r[12] = vf2_tgp_r();
    r[13] = vf2_tgp_r();
    AGES_G(4) = vf2_tgp_r();
    AGES_G(5) = vf2_tgp_r();
    ages_st32(g13 + 0x144u, r[12]);
    ages_st32(g13 + 0x148u, r[13]);
    r[15] = ages_ld32(g13 + 0xF4u);
    r[14] = ages_ld32(g13 + 0xF0u);
    r[15] = colig_addr(r[14], r[15]);
    ages_st32(g13 + 0xF4u, r[14]);
    r[14] = ages_ld32(g13 + 0xECu);
    r[15] = colig_addr(r[14], r[15]);
    ages_st32(g13 + 0xF0u, r[14]);
    r[15] = colig_addr(AGES_G(5), r[15]);
    ages_st32(g13 + 0xECu, AGES_G(5));
    r[14] = 0x40800000u;                               /* 4.0f */
    AGES_G(5) = colig_divr(r[14], r[15]);
    vf2_cc_coli_flag_conv();
    vf2_cc_recalc_pos(r);
}

static uint32_t afn_vf2_coli_cont_fast(int entry)
{
    (void)entry;
    uint32_t *r = &AGES_R(0), g13 = AGES_G(13);
    ages_ldn(&r[4], AGES_G(7) + 0x1F4u, 3);
    ages_ldn(&r[8], AGES_G(8) + 0x1F4u, 3);
    r[12] = colig_subr(r[4], r[8]);
    r[14] = colig_subr(r[6], r[10]);
    r[15] = 0x1F003E3Eu;
    vf2_tgp_w(r[15]);
    vf2_tgp_w(r[12]);
    vf2_tgp_w(r[14]);
    for (int i = 4; i <= 6; i++) vf2_tgp_w(r[i]);
    for (uint32_t o = 0x40u; o < 0x80u; o += 4u) ages_st32(g13 + o, 0);
    r[13] = 0;
    r[14] = g13 + 0x80u;
    r[15] = 0x1D003A3Au;
    vf2_tgp_w(r[15]);
    AGES_G(7) = ages_ld32(0x00500804u);
    AGES_G(9) = 0x01000550u;
    vf2_cc_area_coli();
    AGES_G(7) = ages_ld32(0x00500808u);
    vf2_cc_area_coli();
    vf2_cc_decide_coli_kind();
    if (AGES_G(6) & 1u) {
        vf2_cc_decide_dir();
        AGES_AC = (AGES_AC & ~7u) | 2u;                /* bbc 0,g6 not taken */
    } else {
        vf2_cc_narrow(r);
    }
    return (0x23648u - 0x23524u) * 2u;                 /* the `ret` (EXT 4e: 0x49) */
}
