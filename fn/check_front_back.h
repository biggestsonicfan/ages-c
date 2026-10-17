/* check_front_back: i960 0x1CA20 (a label after play_motion), PS2 0x1DB500.
 *
 * g0: a motion's direction bits. Bit 8 or 9 of g0 flips it front for back:
 * xor 0x140 / 0x280, or 0x180 / 0x240 when bit 1 of g7+0 is set. */
#pragma once

static uint32_t afn_check_front_back(int entry) {
    uint32_t g0 = AGES_G(0), back = (ages_ld32(AGES_G(7)) & 2u) != 0;
    uint32_t cc = 0u;
    if (g0 & 0x100u)      g0 ^= back ? 0x180u : 0x140u;
    else if (g0 & 0x200u) g0 ^= back ? 0x240u : 0x280u;
    else                  cc = 2u;
    AGES_G(0) = g0;
    AGES_AC = (AGES_AC & ~7u) | cc;
    if (entry) ages_i960_ret();
    return 0;
}
