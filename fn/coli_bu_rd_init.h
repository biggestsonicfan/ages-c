/* coli_bu_rd_init: i960 0x1A3C, PS2 0x1ECC00. 32 words from 0x640E7DC to g0. */
#pragma once

static uint32_t afn_coli_bu_rd_init(int entry) {
    for (uint32_t i = 0; i < 32u; i++)
        ages_st32(AGES_G(0) + i * 4u, ages_ld32(0x640E7DCu + i * 4u));
    AGES_AC = (AGES_AC & ~7u) | 2u;
    if (entry) ages_i960_ret();
    return 0;
}
