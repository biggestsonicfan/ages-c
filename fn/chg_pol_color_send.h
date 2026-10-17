/* chg_pol_color_send: i960 0x4060, PS2 0x1EB440 (EXT 87;
 * decomp/FV/FN/ext87_chg_pol_color_send_001eb440.c).
 *
 * chg_pol_color_req's queue (0x546000 busy, 0x546004 the next of 28 blocks,
 * colours from 0x546008, 0x120 bytes a block): copies blocks of 48 colours
 * into the colour RAM at 0x1810000 + 0x200*block (R, G, B planes 0x4000
 * apart), and clears 0x546000 after the last. Between blocks the board reads
 * timer 2 (ldl 0xF00008: both words from the one register; MMIO does not
 * burst, m2-hle2's mem_burst_step) and stops for this frame once
 * ((0xFFFFF - count) - 18) / 25 is over 0x62C. The PS2 drops the timer and
 * copies every block in one go. */
#pragma once
#include "send_tex_col_go.h"

static uint32_t afn_chg_pol_color_send(int entry) {
    uint32_t busy = ages_ld32(0x546000u);
    col_set_cc(col_cmpo(0, busy));
    if (busy != 0) {
        uint32_t blk = ages_ld32(0x546004u);
        uint32_t src = 0x546008u + 0x120u * blk;
        uint32_t dst = 0x1800000u + 0x200u * blk;
        for (;;) {
            for (uint32_t n = 0; n < 0x1B0u; n += 9, dst += 2)
                for (uint32_t c = 0; c < 3; c++, src += 2)
                    ages_st16(dst + 0x10000u + 0x4000u * c, (uint32_t)ages_ld16s(src));
            dst += 0x1A0u;
            uint32_t last = blk == 27;
            col_set_cc(col_cmpo(27, blk));
            ages_st32(0x546004u, ++blk);
            if (last) {
                ages_st32(0x546000u, 0);
                break;
            }
            (void)ages_ld32(0xF00008u);   /* ldl: the second word too */
            uint32_t t = (0xFFFFFu - (ages_ld32(0xF00008u) & 0xFFFFFu) - 18u) / 25u;
            col_set_cc(col_cmpo(t, 0x62C));
            if (t > 0x62Cu) break;
        }
    }
    if (entry) ages_i960_ret();
    return 0;
}
