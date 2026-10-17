/* calc_attack_flag: i960 0x286F4, PS2 0x1DCC00. g7's attack ball mask: when
 * g7 attacks (bit 8 of +0x1A4) and its frame +0x1AA has reached +0x808, bit i
 * is set for each of the 32 balls whose part (byte i at [+0x1D1C]) is in the
 * attack kind's part mask (word at 0x640E72E + 4 * kind, kind = +0x820; kind
 * 41 has none). The mask goes to g3 and +0x77C, the kind's word to +0xAC8. */
#pragma once

static uint32_t afn_calc_attack_flag(int entry) {
    uint32_t g7 = AGES_G(7), r3 = 0, g3 = 0, cc = 0;
    if (ages_ld32(g7 + 0x1A4u) & 0x100u) {
        uint32_t a = (uint32_t)ages_ld16s(g7 + 0x1AAu), b = (uint32_t)ages_ld16s(g7 + 0x808u);
        cc = a < b ? 4u : a == b ? 2u : 1u;
        if (a >= b) {
            r3 = ages_ld8(g7 + 0x820u);
            cc = r3 == 41u ? 2u : r3 < 41u ? 4u : 1u;    /* cmpobe r3, 41 */
            if (r3 != 41u) {
                r3 = ages_ld32(0x640E72Eu + r3 * 4u);
                uint32_t parts = ages_ld32(g7 + 0x1D1Cu);
                for (uint32_t i = 0; i < 32u; i++)
                    if ((r3 >> (ages_ld8(parts + i) & 31u)) & 1u) g3 |= 1u << i;
                cc = 2u;
            }
        }
    }
    ages_st32(g7 + 0xAC8u, r3);
    ages_st32(g7 + 0x77Cu, g3);
    AGES_G(3) = g3;
    AGES_AC = (AGES_AC & ~7u) | cc;
    if (entry) ages_i960_ret();
    return 0;
}
