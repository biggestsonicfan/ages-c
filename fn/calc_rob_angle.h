/* calc_rob_angle: i960 0x2AA84, PS2 0x1E4780 (EXT 5c). Unless frame 1 of
 * an already-posed motion (0x1AA == 1, 0xC4E == 1, 0xC4C == 0x1A8), runs
 * get_frame_dat and falls into calc_rob_angle_cont. As the PS2. */
#pragma once

static uint32_t afn_calc_rob_angle(int entry)
{
    uint32_t rob = AGES_G(7);
    if (ages_ld16(rob + 0x1AA) == 1 && ages_ld16(rob + 0xC4E) == 1 &&
        ages_ld16(rob + 0xC4C) == ages_ld16(rob + 0x1A8)) {
        AGES_AC = (AGES_AC & ~7u) | 2u;
        if (entry) ages_i960_ret();
        return 0;
    }
    afn_get_frame_dat(0);
    return afn_calc_rob_angle_cont(entry);
}
