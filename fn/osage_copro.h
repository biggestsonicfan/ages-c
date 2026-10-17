/* osage_copro: i960 0x6397C, PS2 0x1FE0C0 (helper of osage_dsp).
 *
 * Runs the COP's sway solver (0x25004A4A) over the chain stream at
 * g13+0x4C, then walks the records (g9 from g13+0x44, g8 from g13+0x48,
 * steps from the table at 0x62734), reading one COP word per record; type
 * 5 records (segments) draw through os_set_osage_after, types 1-4 go to a
 * bare ret on the board. */
#pragma once

#include "os_set_osage_after.h"

static uint32_t afn_osage_copro(int entry) {
    uint32_t g13 = AGES_G(13);
    ages_cop_w(0x25004A4Au);
    ages_cop_w(ages_ld32(g13 + 0x4Cu));
    AGES_G(9) = ages_ld32(g13 + 0x44u);
    AGES_G(8) = ages_ld32(g13 + 0x48u);
    for (;;) {
        (void)ages_cop_r();
        uint32_t type = ages_ld32(AGES_G(9));
        AGES_G(9) += 4;
        AGES_G(8) += 4;
        if (type == 0) break;
        if (type == 5) afn_os_set_osage_after(0);
        uint32_t step[2];
        ages_ldn(step, 0x62734u + type * 8u, 2);
        AGES_G(9) += step[0];
        AGES_G(8) += step[1];
    }
    AGES_AC = (AGES_AC & ~7u) | 2u;
    if (entry) ages_i960_ret();
    return 0;
}
