/* rob_kage_set_flag_sub: i960 0x6027C, PS2 0x1F4440 (helper of
 * rob_kage_set_flag_inout).
 *
 * Fighter g7's shadow unit masks for light g0: COP 0x3A (fighter, the
 * stage's byte at 0x60B98, g5, g6, g3, g4) gives two words, stored at
 * g7+0xAA8 and g7+0xAB8 [g0]; zeros when status bit 7 is clear, nothing
 * on the frames whose parity is not the fighter's. The same as the PS2. */
#pragma once

static uint32_t afn_rob_kage_set_flag_sub(int entry) {
    uint32_t rob = AGES_G(7), in = 0, out = 0;
    AGES_AC &= ~7u;                          /* bbc 7 / bbs 0 not taken */
    if (ages_ld32(rob) & 0x80u) {
        uint32_t who = ages_ld8(rob + 4u);
        if ((who ^ ages_ld32(0x500020u)) & 1u) {
            AGES_AC = (AGES_AC & ~7u) | 2u;
            if (entry) ages_i960_ret();
            return 0;
        }
        uint32_t k = ages_ld8(0x60B98u + ages_ld8(0x500064u));
        ages_cop_w(0x3A807575u);
        ages_cop_w(who);
        ages_cop_w(k);
        ages_cop_w(AGES_G(5));
        ages_cop_w(AGES_G(6));
        ages_cop_w(AGES_G(3));
        ages_cop_w(AGES_G(4));
        in = ages_cop_r();
        out = ages_cop_r();
    }
    ages_st32(rob + 0xAA8u + AGES_G(0) * 4u, in);
    ages_st32(rob + 0xAB8u + AGES_G(0) * 4u, out);
    if (entry) ages_i960_ret();
    return 0;
}
