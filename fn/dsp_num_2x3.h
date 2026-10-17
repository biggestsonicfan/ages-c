/* dsp_num_2x3: i960 0x6BD8, PS2 0x1CB1C0 (EXT 8c;
 * decomp/FV/FN/ext8c_dsp_num_2x3_001cb1c0.c).
 *
 * Draw digit g0 as glyph g0 + 40 of the 2x3 font (see dsp_char_2x3) at g9;
 * g9 ends one glyph (4 bytes) to the right. */
#pragma once
#include "dsp_char_2x3.h"

static uint32_t afn_dsp_num_2x3(int entry)
{
    obj_2x3_glyph(AGES_G(0) + 40u);
    if (entry) ages_i960_ret();
    return 0;
}
