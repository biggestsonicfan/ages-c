/* finish_wall_calc_pos: i960 0x7B694, PS2 0x1FA4C0. Which wall the finishing
 * blow drove the loser into. Of the finish position (finish_coli_pos_get):
 * |x| > |z| sets bit 1 (x < 0) or 3 of 0x574004 and puts z (negated for 3)
 * in 0x574000; otherwise bit 2 (z < 0) or 0, with x (negated for 2). */
#pragma once
#include "finish_coli_pos_get.h"
#include "decide_dir.h"            /* colig_cmpr */

static uint32_t afn_finish_wall_calc_pos(int entry) {
    uint32_t x, z, bit, v;
    colig_finish_pos(&x, &z);
    if (colig_cmpr(x & 0x7FFFFFFFu, z & 0x7FFFFFFFu) == 1u) {
        if (x & 0x80000000u) { bit = 2u; v = z; }
        else                 { bit = 8u; v = z ^ 0x80000000u; }
    } else {
        if (z & 0x80000000u) { bit = 4u; v = x ^ 0x80000000u; }
        else                 { bit = 1u; v = x; }
    }
    ages_st32(0x574004u, ages_ld32(0x574004u) | bit);
    ages_st32(0x574000u, v);
    if (entry) ages_i960_ret();
    return 0;
}
