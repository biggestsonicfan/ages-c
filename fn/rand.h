/* rand: i960 0x6DC8, PS2 0x1CBA00 (EXT 64;
 * decomp/FV/FN/ext64_rand_001cba00.c).
 *
 * Mixes the four timers at 0xF00000 into the seed at 0x500098;
 * g0 = 16 bits of the new seed. */
#pragma once

static uint32_t afn_rand(int entry)
{
    uint32_t seed = ages_ld32(0x500098);
    seed += ages_ld32(0xF00000) << 4;
    seed += ages_ld32(0xF00004) << 8;
    seed += ages_ld32(0xF00008) << 12;
    seed += ages_ld32(0xF0000C) << 16;
    ages_st32(0x500098, seed);
    AGES_G(0) = (seed >> 4) & 0xFFFFu;
    if (entry) ages_i960_ret();
    return 0;
}
