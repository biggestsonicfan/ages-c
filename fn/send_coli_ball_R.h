/* send_coli_ball_R: i960 0x1B90, PS2 0x1EBB40, EXT 63. Both fighters' ball
 * radii (0x50A19C, 0x50A224) to the COP's DM with Fn_write_ram: ball
 * order[i] (bytes at 0x292AC) to slots 0x00-0x1F, 0 to slot 0x20, order[31]
 * .. order[0] to slots 0x21-0x40, then the words at 0x50A21C/0x50A220 (and
 * 0x50A2A4/0x50A2A8) to slots 0xF0/0xF1, without the handshake. Fighter 0's
 * slots are at DM 0x30600, fighter 1's at 0x30700. */
#pragma once
#include "send_ball_to_unit.h"

static void colig_send_r1(uint32_t slot, uint32_t i) {
    uint32_t b = ages_ld8(0x292ACu + i);
    colig_cop_pair(0x30600u + slot, ages_ld32(0x50A19Cu + b * 4u),
                   0x30700u + slot, ages_ld32(0x50A224u + b * 4u));
}

static void colig_write_ram(uint32_t dm, uint32_t v) {
    ages_cop_w(0x24804949u);
    ages_cop_w(dm);
    ages_cop_w(v);
}

static uint32_t afn_send_coli_ball_R(int entry) {
    for (uint32_t i = 0; i < 32u; i++)
        colig_send_r1(i, i);
    colig_cop_pair(0x30620u, 0, 0x30720u, 0);
    for (uint32_t k = 0; k < 32u; k++)
        colig_send_r1(0x21u + k, 31u - k);
    colig_write_ram(0x306F0u, ages_ld32(0x50A21Cu));
    colig_write_ram(0x307F0u, ages_ld32(0x50A2A4u));
    colig_write_ram(0x306F1u, ages_ld32(0x50A220u));
    colig_write_ram(0x307F1u, ages_ld32(0x50A2A8u));
    AGES_AC = (AGES_AC & ~7u) | 1u;   /* cmpible 0, -1: greater */
    if (entry) ages_i960_ret();
    return 0;
}
