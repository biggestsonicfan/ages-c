/* os_set_etc: i960 0x63910, PS2 0x1FCB80 (helper of osage_dsp, record
 * type 3).
 *
 * Copies the record's two words to g8: the second is 0.8 while the fighter
 * is low (g13 flag 6), the first 1 while the chain is frozen (flag 4). */
#pragma once

static uint32_t afn_os_set_etc(int entry) {
    uint32_t g8 = AGES_G(8), g9 = AGES_G(9), g13 = AGES_G(13);
    ages_st32(g8, ages_ld32(g9));
    uint32_t v = ages_ld32(g9 + 4u);
    if (ages_ld32(g13) & 0x40u) v = 0x3F4CCCCDu;
    ages_st32(g8 + 4u, v);
    uint32_t f = ages_ld32(g13) & 0x10u;
    if (f) ages_st32(g8, 1);
    AGES_AC = (AGES_AC & ~7u) | (f ? 2u : 0u);
    if (entry) ages_i960_ret();
    return 0;
}
