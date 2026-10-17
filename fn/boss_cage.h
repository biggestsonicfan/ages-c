/* boss_cage: i960 0x22668, PS2 0x1F7E80 (helper of cage_clip_m).
 *
 * The boss stage's fence-post object list: g2 = 0x22948 when the byte at
 * g13+0x313 is set, else 0x22908 + 16 * (byte at g13+0x310) + 4 * g1.
 * The same as the PS2. */
#pragma once

static uint32_t afn_boss_cage(int entry) {
    if (ages_ld8(AGES_G(13) + 0x313u) != 0)
        AGES_G(2) = 0x22948u;
    else
        AGES_G(2) = 0x22908u + (ages_ld8(AGES_G(13) + 0x310u) << 4) + AGES_G(1) * 4u;
    if (entry) ages_i960_ret();
    return 0;
}
