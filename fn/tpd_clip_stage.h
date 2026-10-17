/* tpd_clip_stage: i960 0x334DC, PS2 0x1EDD40 (a helper of tex_move_poly).
 *
 * One texture-point record (g6, 0x24 bytes) against the screen: COP 0x29
 * moves its point (+8, g9 for y, +0x10) by the camera matrix; behind the eye
 * (z negative) it stops. Otherwise x and x +- the radius (+0x14), scaled by
 * the focal length (0x501084) over z, are counted outside +-248 and the count
 * goes to COP 0x17. g0 bit 0 is set unless all three are out on one side.
 * The PS2's floats are the i960's real ops here (tpd_*r: m2-hle2's, rounded
 * to single, divr by zero 0). The tpd_* real helpers serve set_tpd_data too. */
#pragma once
#include "send_tex_col_go.h"

static uint32_t tpd_r(double d) { return i960_double_to_single(d); }
static double tpd_d(uint32_t u) { return i960_single_to_double(u); }
static uint32_t tpd_mulr(uint32_t a, uint32_t b) {
    double x = tpd_d(a), y = tpd_d(b);
    return tpd_r(i960_nan_result(y * x, x, y));
}
/* divr s1,s2: s2 / s1 */
static uint32_t tpd_divr(uint32_t s1, uint32_t s2) {
    double a = tpd_d(s1), b = tpd_d(s2);
    return a != 0.0 ? tpd_r(i960_nan_result(b / a, b, a)) : tpd_r(0.0);
}
static uint32_t tpd_addr(uint32_t a, uint32_t b) {
    double x = tpd_d(a), y = tpd_d(b);
    return tpd_r(i960_nan_result(x + y, x, y));
}
/* subr s1,s2: s2 - s1 */
static uint32_t tpd_subr(uint32_t s1, uint32_t s2) {
    double a = tpd_d(s1), b = tpd_d(s2);
    return tpd_r(i960_nan_result(b - a, b, a));
}
/* remr s1,s2: fmod(s2, s1) */
static uint32_t tpd_remr(uint32_t s1, uint32_t s2) {
    double a = tpd_d(s1), b = tpd_d(s2);
    return tpd_r(i960_nan_result(fmod(b, a), b, a));
}
/* cmpr a,b: 4 less, 2 equal, 1 greater, 0 unordered */
static uint32_t tpd_cmpr(uint32_t a, uint32_t b) {
    double x = tpd_d(a), y = tpd_d(b);
    return x < y ? 4u : x == y ? 2u : x > y ? 1u : 0u;
}

/* The camera matrix on the COP's stack (push, 0x09 with the view tilt), then
 * COP 0x29 on (x, y, z): the moved point's three words into p. */
static void tpd_cop_point(uint32_t x, uint32_t y, uint32_t z, uint32_t *p) {
    ages_cop_w(0x800101u);
    ages_cop_w(0x4800909u);
    ages_cop_w(0u - (uint32_t)ages_ld16s(0x50A022u));
    ages_cop_w(0x14802929u);
    ages_cop_w(x);
    ages_cop_w(y);
    ages_cop_w(z);
    p[0] = ages_cop_r();
    p[1] = ages_cop_r();
    p[2] = ages_cop_r();
}

/* x against +-248.0 (cmpr; bl / bg): 1 if outside, NaN too. */
static uint32_t tpd_out(uint32_t x) {
    uint32_t cc = tpd_cmpr(x, 0x43780000u);
    if (cc & 4u) {
        cc = tpd_cmpr(x, 0xC3780000u);
        col_set_cc(cc);
        return (cc & 1u) ? 0u : 1u;
    }
    col_set_cc(cc);
    return 1u;
}

static uint32_t afn_tpd_clip_stage(int entry) {
    uint32_t p[3];
    tpd_cop_point(ages_ld32(AGES_G(6) + 8u), AGES_G(9), ages_ld32(AGES_G(6) + 0x10u), p);
    ages_cop_w(0x1000202u);
    col_set_cc((p[2] & 0x80000000u) ? 2u : 0u);
    if (!(p[2] & 0x80000000u)) {
        uint32_t f = ages_ld32(0x501084u);
        uint32_t c = tpd_divr(p[2], tpd_mulr(f, p[0]));
        uint32_t r = tpd_divr(p[2], tpd_mulr(f, ages_ld32(AGES_G(6) + 0x14u)));
        uint32_t v[3] = { c, tpd_addr(r, c), tpd_subr(r, c) };
        uint32_t n = tpd_out(v[0]) + tpd_out(v[1]) + tpd_out(v[2]);
        ages_cop_w(0xB801717u);
        ages_cop_w(n);
        (void)ages_cop_r();
        col_set_cc(col_cmpo(3, n));
        uint32_t side = (v[0] ^ v[1]) | (v[1] ^ v[2]) | (v[0] ^ v[2]);
        if (n == 3) col_set_cc((side & 0x80000000u) ? 2u : 0u);
        if (n != 3 || (side & 0x80000000u)) AGES_G(0) |= 1u;
    }
    if (entry) ages_i960_ret();
    return 0;
}
