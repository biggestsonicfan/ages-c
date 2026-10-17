/* rob_kage_disp_kabe: i960 0x77AF8, PS2 0x1CDDC0 (helper of
 * rob_kabe_kage_disp).
 *
 * Fighter g7's shadow on one wall, under COP matrix g5 (6: x wall, 4: z
 * wall): the 15 units of the wall table (0x6442C68 by the byte at
 * g7+0x1B0, 0x1C bytes each: unit, four models, the item's model, the low
 * model) not yet in g6, whose wall-space position (kept at 0x50F600) is
 * below the unit's height limit and within the wall; drawn ones go into g3,
 * units within the wall into g6 (unless g6 bit 31). Then the held item's
 * shadow (g6 bit 30). The model choice follows the board (0xBB8 / 0xBB9
 * for units 15 / 12 unless g7+0x1A4 bit 14 or 4). */
#pragma once

#include "rob_kage_disp_test.h"

/* The unit's model: 0x10 normally; units' variants by the fighter's state. */
static uint32_t rkk_model(uint32_t ent, uint32_t u, uint32_t y) {
    uint32_t rob = AGES_G(7);
    if (u == 9 && !(ages_u2f(y) > ages_u2f(0x3E4CCCCDu)))
        return ages_ld32(ent + 0x18u);
    uint32_t st = ages_ld32(rob + 0x1A4u);
    if (!(st & 0x4000u) && !(st & 0x10u)) {
        if (u == 15) return 0xBB8u;
        if (u == 12) return 0xBB9u;
    }
    if ((ages_ld16(rob + 0x1F48u) >> (u & 31u)) & 1u)
        return ages_ld32(ent + 0x10u);
    if (!(ages_ld32(rob) & 0x08000000u)) return ages_ld32(ent + 4u);
    if (!(ages_ld32(rob) & 0x40u)) return ages_ld32(ent + 8u);
    return ages_ld32(ent + 0xCu);
}

static void rkk_set_obj(uint32_t model) {
    AGES_G(1) = 0;
    AGES_G(0) = model;
    afn_set_obj(0);
}

/* Is unit u at p (x, y, z) inside the wall? Sets its g6 bit if so. */
static int rkk_inside(uint32_t u, const uint32_t *p) {
    if (AGES_G(6) & 0x80000000u) return 1;
    float w = ages_ldf(0x50A400u) + ages_u2f(0x3DCCCCCDu);
    if (w < ages_u2f(p[0] & 0x7FFFFFFFu)) return 0;
    if (w < ages_u2f(p[2] & 0x7FFFFFFFu)) return 0;
    AGES_G(6) |= 1u << (u & 31u);
    return 1;
}

/* One unit, up to and including its 0x1000202. */
static void rkk_unit(uint32_t ent, uint32_t u) {
    uint32_t rob = AGES_G(7);
    rkt_sync();
    ages_cop_w(0x00800101u);
    ages_cop_w(0x1B803737u);
    ages_cop_w(ages_ld8(rob + 4u));
    ages_cop_w(u * 12u);
    ages_cop_w(0x23004646u);
    ages_cop_w(AGES_G(5));
    ages_cop_w(0x07800F0Fu);
    uint32_t p[3];
    p[0] = ages_cop_r();
    p[1] = ages_cop_r();
    p[2] = ages_cop_r();
    float top = ages_ldf(0x50027Cu) + ages_ldf(0x6443EDCu + u * 4u);
    if (!(ages_u2f(p[1]) > top) && rkk_inside(u, p)) {
        ages_stn(0x50F600u + u * 16u, p, 3);
        uint32_t side = (AGES_G(6) & 0x80000000u) ? 4u : 8u;
        if (!(ages_ld32(0x50A3F8u) & side)) {
            AGES_G(3) |= 1u << (u & 31u);
            ages_cop_w(0x23004646u);
            ages_cop_w(3);
            rkk_set_obj(rkk_model(ent, u, p[1]));
        }
    }
    ages_cop_w(0x01000202u);
}

/* 0x77D04-0x77E30: the held item. */
static void rkk_item(void) {
    uint32_t rob = AGES_G(7);
    rkt_sync();
    ages_cop_w(0x00800101u);
    uint32_t m = rob + 0xF34u + ages_ld32(rob + 0xF00u) * 0x34u;
    rkt_sync();
    ages_cop_w(0x05800B0Bu);
    for (uint32_t i = 0; i < 12; i++) ages_cop_w(ages_ld32(m + i * 4u));
    rkt_sync();
    ages_cop_w(0x23004646u);
    ages_cop_w(AGES_G(5));
    ages_cop_w(0x07800F0Fu);
    uint32_t x = ages_cop_r() & 0x7FFFFFFFu;
    (void)ages_cop_r();
    uint32_t z = ages_cop_r() & 0x7FFFFFFFu;
    float w = ages_ldf(0x50A400u);
    if (!(w < ages_u2f(x)) && !(w < ages_u2f(z))) {
        AGES_G(6) |= 0x40000000u;
        ages_cop_w(0x23004646u);
        ages_cop_w(3);
        uint32_t n = ages_ld16(rob + 0x788u);
        if ((AGES_G(3) >> (n & 31u)) & 1u)
            rkk_set_obj(ages_ld32(AGES_G(2) + n * 0x1Cu + 0x14u));
    }
    ages_cop_w(0x01000202u);
}

static uint32_t afn_rob_kage_disp_kabe(int entry) {
    AGES_G(0) = 2;
    afn_set_mmode(0);
    ages_cop_w(0x00800101u);
    ages_cop_w(0x22004444u);
    ages_cop_w(AGES_G(5));
    AGES_G(5) += 1;
    uint32_t ent = ages_ld32(0x6442C68u + ages_ld8(AGES_G(7) + 0x1B0u) * 4u);
    AGES_G(2) = ent;
    AGES_G(3) = 0;
    for (int i = 0; i < 15; i++, ent += 0x1Cu) {
        uint32_t u = ages_ld32(ent);
        if (!((AGES_G(6) >> (u & 31u)) & 1u)) rkk_unit(ent, u);
    }
    uint32_t rob = AGES_G(7);
    if (ages_ld16(rob + 0x78Au) != 0 && !(ages_ld32(rob) & 0x08000000u)
        && !(AGES_G(6) & 0x40000000u))
        rkk_item();
    ages_cop_w(0x01000202u);
    AGES_G(0) = 1;
    afn_set_mmode(0);
    if (entry) ages_i960_ret();
    return 0;
}
