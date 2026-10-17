/* set_kamae_ram: i960 0x29EEC, PS2 0x1E1580, EXT 82. Fighter g7's guard
 * (kamae) poses: five get_kamae_value calls, motion numbers from the
 * halfwords at [g7+0x1A0] + 0, 2, 0x10, 0x18, 0x54, into [g7+0xBD8] +
 * 0x1E0, 0x2D0, 0x3C0, 0x4B0, 0x5A0. */
#pragma once

static uint32_t afn_set_kamae_ram(int entry) {
    static const uint16_t mot[5] = { 0x0, 0x2, 0x10, 0x18, 0x54 };
    uint32_t tbl = ages_ld32(AGES_G(7) + 0x1A0u), dst = ages_ld32(AGES_G(7) + 0xBD8u);
    for (int k = 0; k < 5; k++) {
        AGES_G(0) = (uint32_t)ages_ld16s(tbl + mot[k]);
        AGES_G(3) = dst + 0x1E0u + 0xF0u * (uint32_t)k;
        afn_get_kamae_value(0);
    }
    if (entry) ages_i960_ret();
    return 0;
}
