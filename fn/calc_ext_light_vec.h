/* calc_ext_light_vec: i960 0x60554, PS2 0x1F4600 (helper of
 * rob_kage_disp_inside).
 *
 * The shadow direction from extend light g0 to fighter g7: the yaw (COP
 * 0x27 of dz, dx, negated, + 0x8000) to 0x5019AE and the pitch (COP 0x27 of
 * scale + 1 - distance, clamped at 0, and the light's y) to 0x5019AC.
 * g0 = 1. The same as the PS2. */
#pragma once

#include "calc_near_light.h"

static uint32_t afn_calc_ext_light_vec(int entry) {
    float x, z;
    uint32_t pos[3];
    float dist = cnl_light(AGES_G(0), &x, &z, pos);
    float y = ages_ldf(0x50027Cu);
    float dx = ages_u2f(pos[0]) - x;
    float dz = ages_u2f(pos[2]) - z;
    ages_cop_w(0x13802727u);
    ages_cop_wf(dz);
    ages_cop_wf(dx);
    uint32_t yaw = 0u - ages_cop_r() + 0x8000u;
    float h = (ages_ldf(0x50A00Cu) - ages_u2f(0xBF800000u)) - dist;
    int neg = (ages_f2u(h) & 0x80000000u) != 0;   /* bbc 31 */
    if (neg) h = 0.0f;
    ages_cop_w(0x13802727u);
    ages_cop_wf(h);
    ages_cop_wf(y);
    ages_st16(0x5019ACu, ages_cop_r());
    ages_st16(0x5019AEu, yaw);
    AGES_G(0) = 1;
    AGES_AC = (AGES_AC & ~7u) | (neg ? 2u : 0u);
    if (entry) ages_i960_ret();
    return 0;
}
