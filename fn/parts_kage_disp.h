/* parts_kage_disp: i960 0x609B0, PS2 0x1F4AC0 (helper of
 * rob_kage_disp_inside).
 *
 * The 8 loose parts' shadows (parts_kage_set_flag's list), in GEO mode 2
 * under the kage matrix: each live part is moved and turned (COP 0x06,
 * 0x09, 0x08, 0x0A), then COP 0x41 and the kage argument (1 inside, 2
 * outside: bit 8 of +0x24), and drawn. A part with bit 19 is drawn as its
 * own display-list block, linked on 0x501844 (rkt_linked). */
#pragma once

#include "rob_kage_disp_test.h"

static void pkd_part(uint32_t p) {
    if (ages_ld16(p + 0x22u) == 0) return;
    AGES_G(5) = (ages_ld32(p + 0x24u) & 0x100u) ? 1u : 2u;
    AGES_G(1) = ages_ld32(p + 0x24u) & 1u;
    uint32_t v[3];
    ages_ldn(v, p, 3);
    ages_cop_w(0x00800101u);
    ages_cop_w(0x03000606u);
    ages_cop_w(v[0]);
    ages_cop_w(v[1]);
    ages_cop_w(v[2]);
    ages_cop_w(0x04800909u);
    ages_cop_w((uint32_t)ages_ld16s(p + 0x1Eu));
    ages_cop_w(0x04000808u);
    ages_cop_w((uint32_t)ages_ld16s(p + 0x1Cu));
    ages_cop_w(0x05000A0Au);
    ages_cop_w((uint32_t)ages_ld16s(p + 0x20u));
    rkt_sync();
    ages_cop_w(0x20804141u);
    ages_cop_w(0x23004646u);
    ages_cop_w(AGES_G(5));
    uint32_t model = ages_ld32(p + 0x44u);
    if (ages_ld32(p + 0x24u) & (1u << 19)) {
        rkt_linked(0x501844u, model);
    } else {
        AGES_G(1) = 0;
        AGES_G(0) = model;
        afn_set_obj(0);
    }
    ages_cop_w(0x01000202u);
}

static uint32_t afn_parts_kage_disp(int entry) {
    AGES_G(0) = 2;
    afn_set_mmode(0);
    rkt_kage_matrix();
    uint32_t p = ages_ld32(0x500844u) + 0x88u;
    for (int i = 0; i < 8; i++, p += 0x48u) pkd_part(p);
    ages_cop_w(0x01000202u);
    AGES_G(0) = 1;
    afn_set_mmode(0);
    if (entry) ages_i960_ret();
    return 0;
}
