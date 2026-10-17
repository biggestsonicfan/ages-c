/* send_ball_to_unit2: i960 0x1E6C, PS2 0x1EC180. As send_ball_to_unit, with
 * the bytes at 0x506680+i and 0x5066A0+i to DM 0x306C0+i and 0x307C0+i. */
#pragma once
#include "send_ball_to_unit.h"

static uint32_t afn_send_ball_to_unit2(int entry) {
    for (uint32_t i = 0; i < 32u; i++)
        colig_cop_pair(0x306C0u + i, ages_ld8(0x506680u + i),
                       0x307C0u + i, ages_ld8(0x5066A0u + i));
    AGES_AC = (AGES_AC & ~7u) | 2u;
    if (entry) ages_i960_ret();
    return 0;
}
