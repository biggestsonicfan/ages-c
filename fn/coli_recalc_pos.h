/* coli_recalc_pos: i960 0x28128, PS2 0x1DF280. A `bal` leaf of coli_cont_cop
 * (returns with bx (g14)): turn the contact into each fighter's step, g7's
 * in s[0..2] (r8..r10), g8's in s[3..5] (r12..r14). The steps are zero, or
 * g5 up a slope (g13+0x88 == 5 / 4, g5 onto +0x678 capped at 0.15), or the
 * push-out along g13+0xD4..DC split by g13+0xCC / 0xD0 and scaled by g4
 * (capped at 0.3); g6 bit 0 pools the robs' edge pushes +0x644 / +0x64C
 * instead. Then walls drop an axis, an edge hands the push to the other
 * fighter, the rob limits +0x650..+0x65C clamp it, the edge is taken off, and
 * the steps go to +0xA68, COP 0x3D and 0x72 (reply to +0xA29) and +0x18/+0x20;
 * +0x678 decays by 0.03. It reads the caller's r4, r6, r8, r10: coli_cont_cop's
 * x/z of its entry g7 and g8, passed to colig_recalc_pos. */
#pragma once
#include "decide_dir.h"            /* colig_ FPU */

/* Up a slope: g5 onto rob+0x678, capped at 0.15. */
static uint32_t colig_crp_slope(uint32_t rob) {
    uint32_t r12 = ages_ld32(rob + 0x678u), r15 = colig_addr(AGES_G(5), r12);
    if (colig_cmpr(0x3E19999Au, r15) != 1u) {
        r15 = 0x3E19999Au;
        AGES_G(5) = colig_subr(r12, 0x3E19999Au);
    }
    ages_st32(rob + 0x678u, r15);
    return AGES_G(5);
}

/* The share of the push the pair keeps (+0x67C of both). */
static uint32_t colig_crp_share(uint32_t g7, uint32_t g8) {
    uint32_t r7 = ages_ld32(g7 + 0x804u), r11 = ages_ld32(g7 + 0x1A4u);
    if ((r7 & r11) & 0x100u) return 0x3E99999Au;
    uint32_t r3 = ages_ld32(g8 + 0x804u), r12 = ages_ld32(g8 + 0x1A4u);
    if ((r3 & r12) & 0x100u) return 0x3E99999Au;
    AGES_G(0) = 0x2100u;
    if (((r7 | r3) & 0x2100u) == 0x2100u) return 0x3F333333u;
    if ((((r7 >> 6) & r11) | ((r3 >> 6) & r12)) & 4u) return 0x3F266666u;
    return 0x3E99999Au;
}

static void colig_crp_push(uint32_t g7, uint32_t g8, uint32_t r4, uint32_t r6, uint32_t *s) {
    uint32_t g13 = AGES_G(13);
    uint32_t r8 = ages_ld32(g13 + 0xD4u), r10 = ages_ld32(g13 + 0xDCu);
    uint32_t r15 = colig_crp_share(g7, g8);
    ages_st32(g7 + 0x67Cu, r15);
    ages_st32(g8 + 0x67Cu, r15);
    r4 = colig_subr(colig_mulr(r8, r15), r4);
    r6 = colig_subr(colig_mulr(r10, r15), r6);
    r4 = colig_addr(colig_mulr(r4, r8), colig_mulr(r6, r10));
    if (r4 & 0x80000000u) AGES_G(4) = colig_addr(AGES_G(4), r4 ^ 0x80000000u);
    if (!(AGES_G(4) & 0x80000000u) && !(colig_cmpr(0x3E99999Au, AGES_G(4)) & 3u))
        AGES_G(4) = 0x3E99999Au;
    uint32_t n[3] = { ages_ld32(g13 + 0xD4u), ages_ld32(g13 + 0xD8u), ages_ld32(g13 + 0xDCu) };
    uint32_t a = colig_mulr(AGES_G(4), ages_ld32(g13 + 0xCCu) ^ 0x80000000u);
    uint32_t b = colig_mulr(AGES_G(4), ages_ld32(g13 + 0xD0u));
    for (int i = 0; i < 3; i++) {
        s[i] = colig_mulr(a, n[i]);
        s[3 + i] = colig_mulr(b, n[i]);
    }
}

static void colig_crp_store(const uint32_t *s) {
    for (uint32_t i = 0; i < 6u; i++) ages_st32(AGES_G(13) + 0xE0u + i * 4u, s[i]);
}

/* g6 bit 0: no step; when both robs hold (bit 19 or 18 of +0x1A4) they share
 * their edge pushes. */
static void colig_crp_pool(uint32_t g7, uint32_t g8, uint32_t *s) {
    for (int i = 0; i < 6; i++) s[i] = 0;
    colig_crp_store(s);
    uint32_t a = ages_ld32(g7 + 0x1A4u), b = ages_ld32(g8 + 0x1A4u);
    if (!((a | a << 1) & 0x80000u) || !((b | b << 1) & 0x80000u)) return;
    for (uint32_t o = 0x644u; o <= 0x64Cu; o += 8u) {
        uint32_t v = colig_addr(ages_ld32(g7 + o), ages_ld32(g8 + o));
        ages_st32(g7 + o, v);
        ages_st32(g8 + o, v);
    }
}

/* The steps before the walls; 1 when g6 bit 0 pooled them. */
static void colig_crp_steps(uint32_t g7, uint32_t g8, uint32_t r4, uint32_t r6, uint32_t *s) {
    if (!((ages_ld32(g7 + 0x1A4u) | ages_ld32(g8 + 0x1A4u)) & 0x20000000u)) {
        if (AGES_G(6) & 1u) { colig_crp_pool(g7, g8, s); return; }
        if (AGES_G(6) & 8u) afn_decide_dir(0);
    }
    for (int i = 0; i < 6; i++) s[i] = 0;
    uint32_t kind = ages_ld32(AGES_G(13) + 0x88u);
    if (ages_ld32(AGES_G(13) + 0x164u) == 0)
        afn_decide_dir(0);
    else if (kind == 5u)
        s[1] = colig_crp_slope(g7);
    else if (kind == 4u)
        s[4] = colig_crp_slope(g8);
    else
        colig_crp_push(g7, g8, r4, r6, s);
    colig_crp_store(s);
}

/* A wall (g13+0x23C / 0x23E) holds one axis of the rob's step: z when bits
 * 14 and 13 of its heading +0x26 differ, else x. */
static void colig_crp_wall(uint32_t rob, uint32_t off, uint32_t *s) {
    if (ages_ld16s(AGES_G(13) + off) == 0) return;
    uint32_t h = (uint32_t)ages_ld16s(rob + 0x26u);
    s[((h ^ h << 1) & 0x4000u) ? 2 : 0] = 0;
}

/* 1.0, or 0.9 unless the other's body unit g13+0x1E8 is not beyond 0x1D8. */
static uint32_t colig_crp_factor(uint32_t other) {
    if (ages_ld32(other + 0x1A4u) & 0x100u) return 0x3F800000u;
    uint32_t v = ages_ld32(AGES_G(13) + 0x1E8u + ages_ld8(other + 4u) * 4u);
    return colig_cmpr(ages_ld32(AGES_G(13) + 0x1D8u), v) == 4u ? 0x3F666666u : 0;
}

/* The fighter that is further out (+0x680) hands its edge push on. Indices:
 * from (the step axis the other takes it on), to, the two zeroed. */
static uint32_t colig_crp_edges(uint32_t g7, uint32_t g8, uint32_t *s) {
    uint32_t r7 = (colig_cmpr(ages_ld32(g7 + 0x680u), ages_ld32(g8 + 0x680u)) & 3u) ? 0 : 1u;
    static const uint8_t t[4][6] = {
        { 0, 3, 0, 2, 0, 0 }, { 2, 5, 2, 0, 0, 0 }, { 5, 0, 3, 5, 1, 0 }, { 5, 2, 5, 3, 1, 0 },
    };
    uint32_t r3 = 0;
    for (uint32_t k = 0; k < 4u; k++) {
        uint32_t rob = k < 2u ? g7 : g8, other = k < 2u ? g8 : g7;
        if (ages_ld32(rob + 0x644u + (k & 1u) * 8u) == 0 || r7 != t[k][4]) continue;
        uint32_t f = colig_mulr(s[t[k][0]], colig_crp_factor(other));
        s[t[k][1]] = colig_subr(f, s[t[k][1]]);
        s[t[k][2]] = 0;
        s[t[k][3]] = 0;
        r3 |= 1u << k;
    }
    return r3;
}

static uint32_t colig_crp_clamp(uint32_t v, uint32_t lim) {
    if (v & 0x80000000u) {
        uint32_t l = ages_ld32(lim + 4u) ^ 0x80000000u;
        return (colig_cmpr(l, v) & 6u) ? v : l;
    }
    uint32_t l = ages_ld32(lim);
    return (colig_cmpr(l, v) & 3u) ? v : l;
}

static void colig_crp_send(uint32_t rob, uint32_t x, uint32_t z) {
    ages_cop_w(0x39007272u);
    ages_cop_w(ages_ld8(rob + 4u));
    ages_cop_w(x);
    ages_cop_w(z);
    ages_st8(rob + 0xA29u, ages_cop_r());
}

static uint32_t colig_crp_decay(uint32_t rob) {
    uint32_t v = colig_subr(0x3CF5C28Fu, ages_ld32(rob + 0x678u));   /* 0.03f */
    uint32_t cc = (v & 0x80000000u) ? 2u : 0;
    ages_st32(rob + 0x678u, cc ? 0 : v);
    return cc;
}

static void colig_crp_apply(uint32_t g7, uint32_t g8, uint32_t *s) {
    static const uint8_t ix[4] = { 0, 2, 3, 5 };
    for (uint32_t k = 0; k < 4u; k++) {
        uint32_t rob = k < 2u ? g7 : g8, o = 0x644u + (k & 1u) * 8u;
        s[ix[k]] = colig_subr(ages_ld32(rob + o), s[ix[k]]);
    }
    for (uint32_t k = 0; k < 4u; k++)
        ages_st32((k < 2u ? g7 : g8) + 0xA68u + (k & 1u) * 4u, s[ix[k]]);
    ages_cop_w(0x1E803D3Du);
    for (int i = 0; i < 6; i++) ages_cop_w(s[i]);
    colig_crp_send(g7, s[0], s[2]);
    colig_crp_send(g8, s[3], s[5]);
    for (uint32_t k = 0; k < 4u; k++) {
        uint32_t rob = k < 2u ? g7 : g8, o = 0x18u + (k & 1u) * 8u;
        ages_st32(rob + o, colig_addr(ages_ld32(rob + o), s[ix[k]]));
    }
}

static uint32_t colig_recalc_pos(uint32_t r4, uint32_t r6) {
    uint32_t g7 = ages_ld32(0x500804u), g8 = ages_ld32(0x500808u), s[6];
    AGES_G(7) = g7;
    AGES_G(8) = g8;
    colig_crp_steps(g7, g8, r4, r6, s);
    colig_crp_wall(g7, 0x23Cu, s);
    colig_crp_wall(g8, 0x23Eu, s + 3);
    uint32_t r3 = colig_crp_edges(g7, g8, s);
    colig_crp_store(s);
    static const uint8_t ix[4] = { 0, 2, 3, 5 };
    for (uint32_t k = 0; k < 4u; k++)
        if (!(r3 & (1u << k)))
            s[ix[k]] = colig_crp_clamp(s[ix[k]], (k < 2u ? g7 : g8) + 0x650u + (k & 1u) * 8u);
    colig_crp_apply(g7, g8, s);
    colig_crp_decay(g7);
    return colig_crp_decay(g8);
}

static uint32_t afn_coli_recalc_pos(int entry) {
    uint32_t cc = colig_recalc_pos(colig_subr(AGES_R(4), AGES_R(8)), colig_subr(AGES_R(6), AGES_R(10)));
    AGES_AC = (AGES_AC & ~7u) | cc;
    if (entry) ages_branch(AGES_G(14));
    return 0;
}
