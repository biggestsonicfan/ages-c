/* send_tex_col_skin: i960 0x8F4, PS2 0x1EB740 (EXT 86;
 * decomp/FV/FN/ext86_send_tex_col_skin_001eb740.c).
 *
 * g0: fighter 0 or 1 (its work area from 0x500804 / 0x500808 into g3; 1 moves
 * the colour RAM base by 0x400). g1: the skin row. A character (+0x1B0) of 13
 * or more takes rows from 0x2400 on, one row down. Two blocks of 64 colours
 * from 0x2101000 + 0x300*g1 go through the lumi tables at 0x544000/0x544200/
 * 0x544400 into the colour RAM at 0x1813800 (R, G, B planes 0x4000 apart).
 * The board's loads are ldos; the PS2 reads unsigned. g0 ends at 64. */
#pragma once
#include "send_tex_col_go.h"

static uint32_t afn_send_tex_col_skin(int entry) {
    AGES_G(2) = 0;
    AGES_G(3) = ages_ld32(0x500804u);
    if (AGES_G(0) != 0) {
        AGES_G(0) = 0x400;
        AGES_G(3) = ages_ld32(0x500808u);
    }
    if (ages_ld8(AGES_G(3) + 0x1B0u) >= 13) {
        AGES_G(2) = 0x2400;
        AGES_G(1) -= 13;
    }
    uint32_t dst = 0x1803800u + AGES_G(0);
    for (uint32_t blk = 0; blk < 2; blk++) {
        uint32_t src = 0x2101000u + 0x300u * AGES_G(1) + 0x180u * blk + AGES_G(2);
        for (uint32_t i = 0; i < 64; i++, dst += 2)
            for (uint32_t c = 0; c < 3; c++) {
                uint32_t v = (uint32_t)ages_ld16s(src + 0x80u * c + i * 2u);
                ages_st16(dst + 0x10000u + 0x4000u * c,
                          (uint32_t)ages_ld16s(0x544000u + 0x200u * c + v * 2u));
            }
        dst += 0x180u;
    }
    AGES_G(0) = 64;
    col_set_cc(2);   /* cmpinco 1,r5 on the last block */
    if (entry) ages_i960_ret();
    return 0;
}
