/* rob_ball_data_make: i960 0x24CA4, PS2 0x1E1040, EXT 58. Fighter g7's 32
 * ball quads at g7+0xD00: ball order[i] (bytes at 0x292AC) gets x, y, z from
 * the character's position table (0x2929C, 12 bytes per ball, in i order)
 * and its radius from the radius table (0x292A4, by order[i]). */
#pragma once

static uint32_t afn_rob_ball_data_make(int entry) {
    uint32_t chr = ages_ld8(AGES_G(7) + 4u);
    uint32_t pos = ages_ld32(0x2929Cu + chr * 4u);
    uint32_t rad = ages_ld32(0x292A4u + chr * 4u);
    uint32_t q[4];
    for (uint32_t i = 0; i < 32u; i++) {
        uint32_t b = ages_ld8(0x292ACu + i);
        ages_ldn(q, pos + i * 12u, 3);
        q[3] = ages_ld32(rad + b * 4u);
        ages_stn(AGES_G(7) + 0xD00u + b * 16u, q, 4);
    }
    AGES_AC = (AGES_AC & ~7u) | 2u;   /* cmpinco 31, 31 */
    if (entry) ages_i960_ret();
    return 0;
}
