/* rob_kage_set_flag_inout: i960 0x60178, PS2 0x1F4180 (helper of
 * rob_kage_disp_inside).
 *
 * The shadow volume for rob_kage_set_flag_sub: g3/g4 = the float at
 * 0x50A00C less 0.3 / plus 0.2, g5/g6 from 0x501990/0x501994. On an extend
 * stage (0x5FA6C) both move up 0.2 and g5/g6 come from the light's angles
 * (COP 0x21, 0x22, 0x24, 0x25) for fighter g7 alone; else for both
 * fighters, g7 = 0x500804's then 0x500808's (g7 is left at the second).
 * The same as the PS2. */
#pragma once

#include "rob_kage_set_flag_sub.h"

static void rki_light(void) {
    const float d = ages_u2f(0x3E4CCCCDu);   /* 0.2 */
    AGES_G(3) = ages_f2u(ages_u2f(AGES_G(3)) + d);
    AGES_G(4) = ages_f2u(ages_u2f(AGES_G(4)) + d);
    ages_cop_w(0x10802121u);
    ages_cop_w((uint32_t)ages_ld16s(0x5019ACu));
    float c = ages_cop_rf();
    ages_cop_w(0x11002222u);
    ages_cop_w((uint32_t)ages_ld16s(0x5019ACu));
    float s = ages_cop_rf();
    float t = s * (1.0f / c);
    uint32_t a = (uint32_t)ages_ld16s(0x5019AEu) + 0x8000u;
    ages_cop_w(0x12002424u);
    ages_cop_w(a);
    ages_cop_wf(t);
    AGES_G(5) = ages_cop_r() ^ 0x80000000u;
    ages_cop_w(0x12802525u);
    ages_cop_w(a);
    ages_cop_wf(t);
    AGES_G(6) = ages_cop_r();
}

static uint32_t afn_rob_kage_set_flag_inout(int entry) {
    float h = ages_ldf(0x50A00Cu);
    AGES_G(3) = ages_f2u(h - ages_u2f(0x3E99999Au));
    AGES_G(4) = ages_f2u(h + ages_u2f(0x3E4CCCCDu));
    AGES_G(5) = ages_ld32(0x501990u);
    AGES_G(6) = ages_ld32(0x501994u);
    if (ages_ld8(0x5FA6Cu + ages_ld8(0x500064u)) != 0) {
        rki_light();
    } else {
        AGES_G(7) = ages_ld32(0x500804u);
        afn_rob_kage_set_flag_sub(0);
        AGES_G(7) = ages_ld32(0x500808u);
    }
    afn_rob_kage_set_flag_sub(0);
    if (entry) ages_i960_ret();
    return 0;
}
