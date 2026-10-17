/* rob_kage_set_matrix: i960 0x5FA7C (inlined in the PS2's
 * rob_kage_disp_inside).
 *
 * COP 0x39 with the light's two angles, the signed halfwords at 0x5019AC
 * and 0x5019AE (calc_ext_light_vec's). */
#pragma once

static uint32_t afn_rob_kage_set_matrix(int entry) {
    uint32_t a = (uint32_t)ages_ld16s(0x5019ACu), b = (uint32_t)ages_ld16s(0x5019AEu);
    ages_cop_w(0x39807373u);
    ages_cop_w(a);
    ages_cop_w(b);
    if (entry) ages_i960_ret();
    return 0;
}
