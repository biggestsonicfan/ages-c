/* sound_request_u: i960 0x41EA4, PS2 0x1F3740. Queue sound command g0 for
 * the sound board. Not 0xA00001: dropped when 0x50002C & 0xC and bit 0 of
 * byte 0x3351 of [0x50016C] (sound off); a 0x9Exxxx command becomes 0x9Cxxxx.
 * Then it falls into sound_request_special (0x41F04): with 0x21 written
 * twice to the interrupt enable (0xE80004), g0 goes into the 32-entry ring
 * at 0x504020 (count 0x504001, write index 0x504003) unless it is full,
 * then 0x421 twice to 0xE80004.
 * The PS2 hands g0 to its own sound driver (FUN_0019a680); this writes the
 * board's queue, which the i960's sound interrupt drains. */
#pragma once
#include "calc_up_down_damage.h"   /* colig_cc_u */

static uint32_t colig_sound_request_special(void) {
    ages_st32(0xE80004u, 0x21u);
    ages_st32(0xE80004u, 0x21u);
    uint32_t n = ages_ld8(0x504001u), cc = colig_cc_u(n, 32u);
    if (n < 32u) {
        ages_st8(0x504001u, n + 1u);
        uint32_t w = ages_ld8(0x504003u);
        ages_st32(0x504020u + w * 4u, AGES_G(0));
        ages_st8(0x504003u, (w + 1u) & 31u);
    }
    ages_st32(0xE80004u, 0x421u);
    ages_st32(0xE80004u, 0x421u);
    return cc;
}

static uint32_t afn_sound_request_u(int entry) {
    uint32_t cc = 2u;
    if (AGES_G(0) != 0xA00001u) {
        if ((ages_ld32(0x50002Cu) & 0xCu) &&
            (ages_ld8(ages_ld32(0x50016Cu) + 0x3351u) & 1u))
            goto out;
        if ((AGES_G(0) & 0xFF0000u) == 0x9E0000u) AGES_G(0) -= 0x20000u;
    }
    cc = colig_sound_request_special();
out:
    AGES_AC = (AGES_AC & ~7u) | cc;
    if (entry) ages_i960_ret();
    return 0;
}
