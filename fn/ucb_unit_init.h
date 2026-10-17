/* ucb_unit_init: i960 0x1A60, PS2 0x1ECD40. 15 longs from the table that
 * word 1 of 0x640E85C points at, to g0. */
#pragma once

static uint32_t afn_ucb_unit_init(int entry) {
    uint32_t src = ages_ld32(0x640E85Cu + 4u), l[2];
    for (uint32_t i = 0; i < 15u; i++) {
        ages_ldn(l, src + i * 8u, 2);
        ages_stn(AGES_G(0) + i * 8u, l, 2);
    }
    AGES_AC = (AGES_AC & ~7u) | 2u;
    if (entry) ages_i960_ret();
    return 0;
}
