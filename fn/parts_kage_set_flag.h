/* parts_kage_set_flag: i960 0x608B8, PS2 0x1F5080 (helper of
 * rob_kage_disp_inside).
 *
 * For the 8 loose parts at 0x500844's +0x88 (0x48 bytes each, live when
 * the halfword at +0x22 is set): bit 8 of the word at +0x24 = the part's
 * shadow is inside the volume. Its position is pushed along the light (g5,
 * g6 from 0x501990/0x501994) by its height; the reach is COP 0x2D over COP
 * 0x1A(2.0) on stages with a light (0x60B98), else the larger of |x|, |z|.
 * Bit 8 is set when y >= 0 and the reach is not past the float at 0x50A00C
 * - 0.5, cleared when not below + 0.4. The compares are the board's
 * integer compares of the floats' bits (cmpibg / cmpibl). Leaves g1 past
 * the parts, g3-g6 as set. The same as the PS2. */
#pragma once

static int32_t pks_reach(float x, float z) {
    if (ages_ld8(0x60B98u + ages_ld8(0x500064u)) != 0) {
        ages_cop_w(0x16802D2Du);
        ages_cop_wf(x);
        ages_cop_wf(z);
        float r = ages_cop_rf();
        ages_cop_w(0x0D001A1Au);
        ages_cop_w(0x40000000u);
        return (int32_t)ages_f2u(r / ages_cop_rf());
    }
    int32_t ax = (int32_t)(ages_f2u(x) & 0x7FFFFFFFu);
    int32_t az = (int32_t)(ages_f2u(z) & 0x7FFFFFFFu);
    return az < ax ? ax : az;
}

static void pks_part(uint32_t p) {
    if (ages_ld16(p + 0x22u) == 0) return;
    uint32_t v[3];
    ages_ldn(v, p, 3);
    float y = ages_u2f(v[1]);
    float x = ages_u2f(v[0]) + ages_u2f(AGES_G(5)) * y;
    float z = ages_u2f(v[2]) + ages_u2f(AGES_G(6)) * y;
    int32_t r = pks_reach(x, z);
    if (!((int32_t)v[1] < 0) && !(r > (int32_t)AGES_G(3)))
        ages_st32(p + 0x24u, ages_ld32(p + 0x24u) | 0x100u);
    if (!(r < (int32_t)AGES_G(4)))
        ages_st32(p + 0x24u, ages_ld32(p + 0x24u) & ~0x100u);
}

static uint32_t afn_parts_kage_set_flag(int entry) {
    AGES_G(5) = ages_ld32(0x501990u);
    AGES_G(6) = ages_ld32(0x501994u);
    float h = ages_ldf(0x50A00Cu);
    AGES_G(3) = ages_f2u(h - 0.5f);
    AGES_G(4) = ages_f2u(h + ages_u2f(0x3ECCCCCDu));
    AGES_G(1) = ages_ld32(0x500844u) + 0x88u;
    for (int i = 0; i < 8; i++, AGES_G(1) += 0x48u) pks_part(AGES_G(1));
    AGES_AC = (AGES_AC & ~7u) | 2u;
    if (entry) ages_i960_ret();
    return 0;
}
