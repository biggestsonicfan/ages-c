/* os_set_osage_after: i960 0x639E8, PS2 0x1FE280 (helper of osage_copro,
 * record type 5).
 *
 * Draws a sway segment through set_obj_fifo (model g9+0x18; g2 = g13 flag 5,
 * or 1 for model 0; g3 the FIFO at 0x50E400): if the FIFO moved, the
 * record's word g9+0x1C follows and 0x50E400 steps. g13+0xF8 takes the word
 * at g8; for the first two frames (g13+6) the three words at g8+0xC clear.
 * FV has none of STF's character 4 blanking. */
#pragma once

static uint32_t afn_os_set_osage_after(int entry) {
    AGES_G(3) = ages_ld32(0x50E400u);
    uint32_t w = ages_ld32(AGES_G(13));
    AGES_G(0) = ages_ld32(AGES_G(9) + 0x18u);
    AGES_G(2) = (w >> 5) & 1u;
    AGES_G(1) = ages_ld8(AGES_G(7) + 4u);
    AGES_G(2) |= AGES_G(0) == 0;
    uint32_t fifo = AGES_G(3);
    afn_set_obj_fifo(0);
    if (AGES_G(3) != fifo) {
        ages_st32(AGES_G(3), ages_ld32(AGES_G(9) + 0x1Cu));
        AGES_G(3) += 4;
        ages_st32(0x50E400u, AGES_G(3));
    }
    ages_st32(AGES_G(13) + 0xF8u, ages_ld32(AGES_G(8)));
    uint32_t n = ages_ld16(AGES_G(13) + 6u);
    if (n < 2) {
        static const uint32_t zero[3] = { 0, 0, 0 };
        ages_stn(AGES_G(8) + 0xCu, zero, 3);
    }
    AGES_AC = (AGES_AC & ~7u) | (n < 2 ? 1u : n == 2 ? 2u : 4u);
    if (entry) ages_i960_ret();
    return 0;
}
