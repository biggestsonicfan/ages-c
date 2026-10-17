/* clr_pattern_s: i960 0x66DC, PS2 0x1CB7C0 (EXT 8a;
 * decomp/FV/FN/ext8a_clr_pattern_s_001cb7c0.c).
 *
 * Fill g1 rows of g0 tile halfwords from g9 (0x80 bytes a row) with 0x20.
 * g9 ends past the last row, g1 counted down by the i960's cmpdeco loop, and
 * the condition code is its last compare's. */
#pragma once

static uint32_t afn_clr_pattern_s(int entry)
{
    uint32_t more;
    do {
        uint32_t p = AGES_G(9), n = AGES_G(0), go;
        do {
            ages_st16(p, 0x20);
            p += 2;
            go = 1u < n;
            n--;
        } while (go);
        AGES_G(9) += 0x80;
        uint32_t g1 = AGES_G(1);
        AGES_AC = (AGES_AC & ~7u) | (1u < g1 ? 4u : 1u == g1 ? 2u : 1u);
        more = 1u < g1;
        AGES_G(1) = g1 - 1u;
    } while (more);
    if (entry) ages_i960_ret();
    return 0;
}
