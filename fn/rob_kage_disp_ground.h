/* rob_kage_disp_ground: i960 0x7860C, PS2 0x1CC840 (helper of
 * rob_kabe_kage_disp).
 *
 * Where fighter g7's shadow meets the walls, a blob on the floor: for units
 * 1-15 below 0.24 high, model 0xAB6 at the unit's floor point, turned
 * toward and stretched to the wall-space point rob_kage_disp_kabe kept at
 * 0x50F600, each in its own linked display-list block (list 0x50184C when
 * the stage's flags have bit 6, else 0x50185C). Nothing when neither wall
 * casts a shadow. g1 is left as the stage's flags then 0, as the board. */
#pragma once

#include "rob_kage_disp_test.h"

static void rkg_unit(uint32_t u) {
    uint32_t p[3], q[3];
    ages_ldn(p, AGES_G(7) + 0x1F4u + u * 12u, 3);
    ages_ldn(q, 0x50F600u + u * 16u, 3);
    ages_cop_w(0x00800101u);
    ages_cop_w(0x03000606u);
    ages_cop_w(p[0]);
    ages_cop_w(0);
    ages_cop_w(p[2]);
    ages_cop_w(0x17802F2Fu);
    ages_cop_w(p[2]);
    ages_cop_w(q[2]);
    ages_cop_w(q[0]);
    ages_cop_w(p[0]);
    uint32_t a = ages_cop_r();
    ages_cop_w(0x15802B2Bu);
    ages_cop_w(p[0]);
    ages_cop_w(q[0]);
    ages_cop_w(p[2]);
    ages_cop_w(q[2]);
    float d = ages_cop_rf() * ages_u2f(0x3F933333u);
    ages_cop_w(0x04800909u);
    ages_cop_w(a + 0x8000u);
    ages_cop_w(0x03800707u);
    ages_cop_w(0x3ECCCCCDu);
    ages_cop_w(0);
    ages_cop_w(ages_f2u(d));
    AGES_G(1) = ages_ld32(0x6442BA8u + ages_ld8(0x500064u) * 12u);
    rkt_linked((AGES_G(1) & 0x40u) ? 0x50184Cu : 0x50185Cu, 0xAB6u);
    ages_cop_w(0x01000202u);
}

static uint32_t afn_rob_kage_disp_ground(int entry) {
    AGES_G(0) = 2;
    afn_set_mmode(0);
    if ((ages_ld32(0x50A3F8u) & 3u) != 3u) {
        for (AGES_G(5) = 1; AGES_G(5) != 16; AGES_G(5)++) {
            uint32_t y = ages_ld32(AGES_G(7) + 0x1F4u + AGES_G(5) * 12u + 4u);
            if (!(ages_u2f(0x3E75C28Fu) <= ages_u2f(y))) rkg_unit(AGES_G(5));
        }
    }
    AGES_G(0) = 1;
    afn_set_mmode(0);
    if (entry) ages_i960_ret();
    return 0;
}
