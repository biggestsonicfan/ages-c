/* ucb_adr_init: i960 0x1A84, PS2 0x1ECE80. g0: a table of 16 words, g1: the
 * unit records it points at (8 bytes each). Words 1-15 get g1, g1+8, ...;
 * g0 and g1 are left past them (g0 + 4, g1 + 0x78).
 * The board's first store is `st 0, 0x0[g0*4]` (absolute g0*4, a slip for
 * (g0)); the PS2 drops it. It is written here, as the i960 does. */
#pragma once

static uint32_t afn_ucb_adr_init(int entry) {
    ages_st32(AGES_G(0) * 4u, 0);
    AGES_G(0) += 4u;
    for (uint32_t i = 0; i < 15u; i++) {
        ages_st32(AGES_G(0) + i * 4u, AGES_G(1));
        AGES_G(1) += 8u;
    }
    AGES_AC = (AGES_AC & ~7u) | 2u;
    if (entry) ages_i960_ret();
    return 0;
}
