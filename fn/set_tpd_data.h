/* set_tpd_data: i960 0x332CC, PS2 0x1ED4C0 (a helper of tex_move_poly).
 *
 * Queue texture-point record g6 (number g5) for send_tpd: bit g5 of
 * 0x50E000, its point count (+6) onto 0x50E008, the record into 0x50E100[n]
 * and its texture scroll into 0x50E300[n] (n: 0x50E004, counted on). The
 * scroll: the record's two points (+8/+0x10, +0x18/+0x20) moved by the camera
 * matrix (COP 0x29); COP 0x2F from the player (0x500814 +0x18) to the first
 * and from the second to the first; their integer difference to a real
 * (COP 0x23), *646 *8 mod 2048 (the i960's real ops), to an integer (COP
 * 0x18), the low half. */
#pragma once
#include "tpd_clip_stage.h"

/* COP 0x2F on (z, z', x', x): one word back. */
static uint32_t tpd_cop_2f(uint32_t z, uint32_t z2, uint32_t x2, uint32_t x) {
    ages_cop_w(0x1000202u);
    ages_cop_w(0x17802F2Fu);
    ages_cop_w(z);
    ages_cop_w(z2);
    ages_cop_w(x2);
    ages_cop_w(x);
    return ages_cop_r();
}

/* The scroll from the two distances (subi; COP 0x23; mulr mulr remr addr;
 * COP 0x18). The CC is bbc 31's on the remainder. */
static uint32_t tpd_scroll(uint32_t d1, uint32_t d2) {
    ages_cop_w(0x11802323u);
    ages_cop_w(d2 - d1);
    uint32_t r = ages_cop_r();
    r = tpd_mulr(0x44218000u, r);   /* 646.0 */
    r = tpd_mulr(0x41000000u, r);   /* 8.0 */
    r = tpd_remr(0x45000000u, r);   /* 2048.0 */
    col_set_cc((r & 0x80000000u) ? 2u : 0u);
    if (r & 0x80000000u) r = tpd_addr(0x45000000u, r);
    ages_cop_w(0x0C001818u);
    ages_cop_w(r);
    return ages_cop_r() & 0xFFFFu;
}

static uint32_t afn_set_tpd_data(int entry) {
    ages_st32(0x50E000u, ages_ld32(0x50E000u) | (1u << (AGES_G(5) & 31u)));
    uint32_t n = ages_ld32(0x50E004u);
    uint32_t pts = (uint32_t)ages_ld16s(AGES_G(6) + 6u);
    ages_st32(0x50E008u, pts + ages_ld32(0x50E008u));
    ages_st32(0x50E100u + n * 4u, AGES_G(6));
    uint32_t pl[3], p[3], q[3];
    ages_ldn(pl, ages_ld32(0x500814u) + 0x18u, 3);
    tpd_cop_point(ages_ld32(AGES_G(6) + 8u), 0, ages_ld32(AGES_G(6) + 0x10u), p);
    uint32_t d1 = tpd_cop_2f(pl[2], p[2], p[0], pl[0]);
    tpd_cop_point(ages_ld32(AGES_G(6) + 0x18u), 0, ages_ld32(AGES_G(6) + 0x20u), q);
    uint32_t d2 = tpd_cop_2f(q[2], p[2], p[0], q[0]);
    uint32_t s = tpd_scroll(d1, d2);
    ages_st32(0x50E300u + ages_ld32(0x50E004u) * 4u, s);
    ages_st32(0x50E004u, ages_ld32(0x50E004u) + 1u);
    if (entry) ages_i960_ret();
    return 0;
}
