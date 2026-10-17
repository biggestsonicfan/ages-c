/* calc_up_down_damage: i960 0x2735C, PS2 0x1E1240. Keep the last upper and
 * lower damage totals (g7+0x1F74/78 to +0x1F7C/80), then total the 16 part
 * damages at g7+0x1F00 (halfwords): parts 0-8 upper, 9-15 lower. A total at
 * or over its limit (g7+0xAF0, +0xAF4; unsigned) sets bit 0 / 1 of g7+0x7F4,
 * unless bit 2 / 3 is already set. */
#pragma once

/* The CC of an unsigned compare a, b. */
static uint32_t colig_cc_u(uint32_t a, uint32_t b) {
    return a < b ? 4u : a == b ? 2u : 1u;
}

/* bbs bit, then cmpobl total, limit: set bit + set_bit when not over. */
static uint32_t colig_damage_flag(uint32_t total, uint32_t lim_off, uint32_t bit, uint32_t set_bit) {
    uint32_t a = AGES_G(7) + 0x7F4u;
    if (ages_ld32(a) & (1u << bit)) return 2u;
    uint32_t lim = ages_ld32(AGES_G(7) + lim_off);
    if (total >= lim) ages_st32(a, ages_ld32(a) | (1u << set_bit));
    return colig_cc_u(total, lim);
}

static uint32_t afn_calc_up_down_damage(int entry) {
    uint32_t g7 = AGES_G(7), up = 0, down = 0;
    ages_st32(g7 + 0x1F7Cu, ages_ld32(g7 + 0x1F74u));
    ages_st32(g7 + 0x1F80u, ages_ld32(g7 + 0x1F78u));
    for (uint32_t i = 0; i < 16u; i++) {
        uint32_t d = (uint32_t)ages_ld16s(g7 + 0x1F00u + i * 2u);
        if (i >= 9u) down += d; else up += d;
    }
    ages_st32(g7 + 0x1F74u, up);
    ages_st32(g7 + 0x1F78u, down);
    colig_damage_flag(up, 0xAF0u, 2, 0);
    uint32_t cc = colig_damage_flag(down, 0xAF4u, 3, 1);
    AGES_AC = (AGES_AC & ~7u) | cc;
    if (entry) ages_i960_ret();
    return 0;
}
