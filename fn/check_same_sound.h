/* check_same_sound: i960 0x41FA4, PS2 0x1F3940. Note sound g0 in the
 * frame's list (words at 0x504078, count byte 0x50406A, up to 4 entries)
 * unless the list is full or g0 is in slots 0..count already (slot count is
 * compared before it is written, as the board does). */
#pragma once
#include "calc_up_down_damage.h"   /* colig_cc_u */

static uint32_t afn_check_same_sound(int entry) {
    uint32_t n = ages_ld8(0x50406Au), cc = colig_cc_u(4u, n);
    if (n < 4u) {
        uint32_t i = n + 1u;
        for (;;) {
            if (ages_ld32(0x504074u + i * 4u) == AGES_G(0)) { cc = 2u; goto out; }
            cc = colig_cc_u(1u, i);
            if (i-- <= 1u) break;
        }
        ages_st32(0x504078u + n * 4u, AGES_G(0));
        ages_st8(0x50406Au, (uint32_t)ages_ld8s(0x50406Au) + 1u);
    }
out:
    AGES_AC = (AGES_AC & ~7u) | cc;
    if (entry) ages_i960_ret();
    return 0;
}
