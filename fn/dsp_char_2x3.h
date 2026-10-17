/* dsp_char_2x3: i960 0x6B34, PS2 0x1CB4C0 (EXT 8b;
 * decomp/FV/FN/ext8b_dsp_char_2x3_001cb4c0.c).
 *
 * Put character g0 (mapped through the byte table at 0x6CE023C) of the 2x3
 * font, pattern set 13 of the table at 0x6450300, at g9 in the tilemap:
 * three rows of two tile halfwords, each the font's plus the set's base.
 * The obj_2x3_* helper is shared with dsp_num_2x3. */
#pragma once

/* Draw glyph `ch` of font 13 at g9; g9 ends one glyph (4 bytes) on. The
 * last cmpdeco leaves CC 010. */
static void obj_2x3_glyph(uint32_t ch)
{
    const uint32_t row_bytes = 0x80;
    uint32_t font = ages_ld32(0x6450300u + 13u * 4u);
    uint32_t base = ages_ld16(font);
    uint32_t src = font + 0xCu + (ch >> 3) * 0x60u + (ch & 7u) * 4u;
    for (int row = 0; row < 3; row++) {
        for (int col = 0; col < 2; col++) {
            ages_st16(AGES_G(9), base + ages_ld16(src));
            src += 2;
            AGES_G(9) += 2;
        }
        src += 28;
        AGES_G(9) += row_bytes - 4;
    }
    AGES_G(9) = AGES_G(9) - 3 * row_bytes + 4;
    AGES_AC = (AGES_AC & ~7u) | 2u;
}

static uint32_t afn_dsp_char_2x3(int entry)
{
    obj_2x3_glyph(ages_ld8(0x6CE023Cu + AGES_G(0)));
    if (entry) ages_i960_ret();
    return 0;
}
