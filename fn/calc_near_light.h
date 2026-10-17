/* calc_near_light: i960 0x604E0, PS2 0x1F4880 (helper of
 * rob_kage_disp_inside).
 *
 * Of the four extend lights (x, z pairs at 0x60620, scaled by the float at
 * 0x50A00C, y from 0x50027C), g0 = the one COP 0x2B puts nearest the
 * fighter's position at g7+0x1F4 (the last of equals; a NaN distance wins).
 * The same as the PS2. */
#pragma once

/* The light's scaled x, z and its distance from g7's position (COP 0x2B). */
static float cnl_light(uint32_t i, float *x, float *z, uint32_t pos[3]) {
    float s = ages_ldf(0x50A00Cu);
    *x = ages_ldf(0x60620u + i * 8u) * s;
    *z = ages_ldf(0x60624u + i * 8u) * s;
    ages_ldn(pos, AGES_G(7) + 0x1F4u, 3);
    ages_cop_w(0x15802B2Bu);
    ages_cop_wf(*x);
    ages_cop_w(pos[0]);
    ages_cop_wf(*z);
    ages_cop_w(pos[2]);
    return ages_cop_rf();
}

static uint32_t afn_calc_near_light(int entry) {
    float best = ages_u2f(0x461C4000u);   /* 10000.0 */
    uint32_t near = 0, pos[3];
    for (uint32_t i = 0; i < 4; i++) {
        float x, z, d = cnl_light(i, &x, &z, pos);
        if (!(d > best)) { near = i; best = d; }
    }
    AGES_G(0) = near;
    AGES_AC = (AGES_AC & ~7u) | 2u;
    if (entry) ages_i960_ret();
    return 0;
}
