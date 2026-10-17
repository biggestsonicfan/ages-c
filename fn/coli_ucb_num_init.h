/* coli_ucb_num_init: i960 0x19F8, PS2 0x1EC980. 16 bytes from 0x640E914 to g0. */
#pragma once

static uint32_t afn_coli_ucb_num_init(int entry) {
    for (uint32_t i = 0; i < 16u; i++)
        ages_st8(AGES_G(0) + i, ages_ld8(0x640E914u + i));
    AGES_AC = (AGES_AC & ~7u) | 2u;
    if (entry) ages_i960_ret();
    return 0;
}
