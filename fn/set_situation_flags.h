/* set_situation_flags: i960 0x63944, PS2 0x1FC1C0 (helper of osage_dsp).
 *
 * g13 flag 1: set when the word at g13+0xF8 is negative, unless the
 * fighter's g7+0x1B1 is 3 with g13 flag 9 (then clear). As the PS2. */
#pragma once

static uint32_t afn_set_situation_flags(int entry) {
    uint32_t g13 = AGES_G(13);
    int set = 0;
    if (ages_ld8(AGES_G(7) + 0x1B1u) != 3u || !(ages_ld32(g13) & 0x200u)) {
        int32_t v = (int32_t)ages_ld32(g13 + 0xF8u);
        set = v < 0;
        AGES_AC = (AGES_AC & ~7u) | (set ? 1u : v == 0 ? 2u : 4u);
    } else {
        AGES_AC = (AGES_AC & ~7u) | 2u;
    }
    uint32_t w = ages_ld32(g13);
    ages_st32(g13, set ? (w | 2u) : (w & ~2u));
    if (entry) ages_i960_ret();
    return 0;
}
