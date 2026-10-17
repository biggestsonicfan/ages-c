/* fix_hara: i960 0x19B44, PS2 0x1DAE00.
 *
 * Once per hold (bit 2 of g7+0): take the fighter's position (g7+0x1F4..
 * 0x1FC) as the current one (g7+0x18..0x20; x and z only when bit 0 of
 * g7+0x83F is clear). Then sets bits 2-3 of g7+0 and bit 4 of g7+0x1A4. */
#pragma once

static uint32_t afn_fix_hara(int entry) {
    uint32_t g7 = AGES_G(7), f = ages_ld32(g7);
    uint32_t cc = 2u;
    if (!(f & 4u)) {
        ages_st32(g7 + 0x1C, ages_ld32(g7 + 0x1F8));
        if (!(ages_ld8(g7 + 0x83F) & 1u)) {
            ages_st32(g7 + 0x18, ages_ld32(g7 + 0x1F4));
            ages_st32(g7 + 0x20, ages_ld32(g7 + 0x1FC));
            cc = 0u;
        }
    }
    ages_st32(g7, f | 12u);
    ages_st32(g7 + 0x1A4, ages_ld32(g7 + 0x1A4) | 0x10u);
    AGES_AC = (AGES_AC & ~7u) | cc;
    if (entry) ages_i960_ret();
    return 0;
}
