/* os_set_tsukene: i960 0x63298, PS2 0x1FC780 (helper of osage_dsp, record
 * type 4).
 *
 * A chain's anchor (tsukene) point g9+8 by the record's kind (g9+0): 0 in
 * unit g9+4's space then the kage matrix 1; 1 last anchor again; 2 in the
 * kage matrix 1 then unit g9+4 relative, matrix slot 2; 3 the kage matrix 2.
 * The point goes to g0-g2, g13+0xFC (new ones), g8 and g13+0x108. Kinds
 * above 3 jump through whatever follows the board's table; not done. */
#pragma once

static void ost_xform(void) {
    ages_cop_w(0x14802929u);
    ages_cop_w(AGES_G(0));
    ages_cop_w(AGES_G(1));
    ages_cop_w(AGES_G(2));
    for (int i = 0; i < 3; i++) AGES_G(i) = ages_cop_r();
}

static void ost_kage(uint32_t n) {
    ages_cop_w(0x22004444u);
    ages_cop_w(n);
}

static void ost_unit(uint32_t cmd, uint32_t unit) {
    ages_cop_w(cmd);
    ages_cop_w(ages_ld8(AGES_G(7) + 4u));
    ages_cop_w(unit * 12u);
}

static uint32_t afn_os_set_tsukene(int entry) {
    uint32_t g9 = AGES_G(9), g13 = AGES_G(13);
    uint32_t kind = ages_ld32(g9);
    if (kind == 1) {
        ages_ldn(&AGES_G(0), g13 + 0xFCu, 3);
    } else {
        uint32_t unit = ages_ld32(g9 + 4u);
        ages_cop_w(0x00800101u);
        if (kind == 0) ost_unit(0x1B003636u, unit);
        ages_ldn(&AGES_G(0), g9 + 8u, 3);
        if (kind == 0) {
            ost_xform();
            ost_kage(1);
        } else if (kind == 2) {
            ost_kage(1);
            ost_unit(0x1B803737u, unit);
        } else {
            ost_kage(2);
        }
        ost_xform();
        if (kind == 2) {
            ages_cop_w(0x21804343u);
            ages_cop_w(2);
        }
        ages_cop_w(0x01000202u);
        ages_stn(g13 + 0xFCu, &AGES_G(0), 3);
    }
    ages_stn(AGES_G(8), &AGES_G(0), 3);
    ages_stn(g13 + 0x108u, &AGES_G(0), 3);
    if (entry) ages_i960_ret();
    return 0;
}
