/* kuc_sukebo_check: i960 0x31DEC (called by koshi_unit_change).
 *
 * While the timer at 0x50A42C runs, measure from the point koshi_unit_change
 * saved at 0x50A420 to the fighter's 0x1F4 (COP get_3d_r); set 0xC8C(g7) to 1
 * when the distance is within 0.4 under the timer's value. */
#pragma once
#include "set_ashi_ptn.h"

static uint32_t afn_kuc_sukebo_check(int entry)
{
    uint32_t t = ages_ld32(0x50A42C);
    if (dsp_cmpo(0, t) != 2) {
        uint32_t d = dsp_get_3d_r(0x50A420, AGES_G(7) + 0x1F4);
        if (dsp_cmpr(d, t) != 1) {
            d = dsp_addr(0x3ECCCCCDu, d);
            if (dsp_cmpr(d, t) != 4) ages_st32(AGES_G(7) + 0xC8C, 1);
        }
    }
    if (entry) ages_i960_ret();
    return 0;
}
