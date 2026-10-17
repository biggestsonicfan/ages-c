/* get_end_value: i960 0x2B4CC, PS2 0x1E8500. Fills the 60 end values of the
 * motion (g7+0xBD8) at g5 from its key stream (each keyed channel's last
 * key), converts the first 36 to halfwords (cvtri, stis), and mirrors them
 * if g7 bit 6. As the PS2. Leaves g1 = 0, g2..g4 advanced, g5 at its start. */
#pragma once

static uint32_t afn_get_end_value(int entry)
{
    uint32_t mot = ages_ld32(AGES_G(7) + 0xBD8u);
    AGES_G(4) = mot + 0x78Cu;
    AGES_G(3) = ages_ld32(mot + 0x784u);
    AGES_G(2) = ages_ld32(mot + 0x780u);
    for (AGES_G(1) = 60; AGES_G(1) != 0; AGES_G(1)--) {
        uint32_t kind = ages_ld8(AGES_G(4));
        if (kind == 4) {
            ages_st32(AGES_G(5), ages_ld32(AGES_G(2)));
            AGES_G(2) += 4;
        } else if (kind > 4) {
            uint32_t stride = kind == 5 ? 4u : 12u;
            AGES_G(2) += ages_ld8(AGES_G(3)) * stride;
            AGES_G(3)++;
            ages_st32(AGES_G(5), ages_ld32(AGES_G(2) - stride));
        } else if (kind == 3) {
            ages_st32(AGES_G(5), 0);
        } else {
            AGES_G(2) += 4;
        }
        AGES_G(4)++;
        AGES_G(5) += 4;
    }
    AGES_G(5) -= 0xF0;
    for (int i = 0; i < 36; i++) {
        ages_st16(AGES_G(5), gems_cvtri(ages_ld32(AGES_G(5))));
        AGES_G(5) += 4;
    }
    AGES_G(5) -= 0x90;
    int mirror = (ages_ld32(AGES_G(7)) & 0x40u) != 0;
    AGES_AC = (AGES_AC & ~7u) | (mirror ? 2u : 0u);
    if (mirror)
        afn_set_mirror(0);
    if (entry) ages_i960_ret();
    return 0;
}
