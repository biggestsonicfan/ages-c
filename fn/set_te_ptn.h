/* set_te_ptn: i960 0x1AC80, PS2 0x1D2280.
 *
 * Hand pattern for rob_disp: g1 = the hand table at 0x6AC(g7) (+0x180 for
 * any unit but 5), indexed by 0x6FD(g7) for the hand unit (8 when bit 6 of
 * the flags is set, else 5) or 0x6F7(g7) for the other. As the PS2. */
#pragma once
#include "set_ashi_ptn.h"

static uint32_t afn_set_te_ptn(int entry)
{
    uint32_t tab = ages_ld32(AGES_G(7) + 0x6AC);
    uint32_t hand = dsp_bbc(ages_ld32(AGES_G(7)), 6) ? 5 : 8;
    if (dsp_cmpi(5, AGES_G(0)) != 2) tab += 0x180;
    uint32_t i = ages_ld8(AGES_G(7) + 0x6FD);
    if (dsp_cmpi(hand, AGES_G(0)) != 2) i = ages_ld8(AGES_G(7) + 0x6F7);
    AGES_G(1) = ages_ld32(tab + (i & 31u) * 4u);
    if (entry) ages_i960_ret();
    return 0;
}
