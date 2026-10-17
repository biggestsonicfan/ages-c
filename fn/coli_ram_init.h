/* coli_ram_init: i960 0x27FE0, PS2 0x1DCA80. Clear the fight collision's
 * per-frame work: g13+0x170..0x1AF, g13+0x98..0xA8, and of both fighters
 * (0x500804 / 0x500808) the halfwords +0x770, +0x772 and the pair +0xA68. */
#pragma once

static uint32_t afn_coli_ram_init(int entry) {
    uint32_t g13 = AGES_G(13);
    for (uint32_t o = 0x170u; o < 0x1B0u; o += 4u) ages_st32(g13 + o, 0);
    for (uint32_t k = 0; k < 2u; k++) {
        uint32_t rob = ages_ld32(0x500804u + k * 4u);
        ages_st16(rob + 0x770u, 0);
        ages_st16(rob + 0x772u, 0);
        ages_st32(rob + 0xA68u, 0);
        ages_st32(rob + 0xA6Cu, 0);
    }
    for (uint32_t o = 0x98u; o <= 0xA8u; o += 4u) ages_st32(g13 + o, 0);
    if (entry) ages_i960_ret();
    return 0;
}
