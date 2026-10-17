/* coli_bu_co_init: i960 0x1A18, PS2 0x1ECAC0. 32 bytes from 0x27998 to g0. */
#pragma once

static uint32_t afn_coli_bu_co_init(int entry) {
    for (uint32_t i = 0; i < 32u; i++)
        ages_st8(AGES_G(0) + i, ages_ld8(0x27998u + i));
    AGES_AC = (AGES_AC & ~7u) | 2u;
    if (entry) ages_i960_ret();
    return 0;
}
