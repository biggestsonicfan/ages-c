/* calc_rob_angle_cont: i960 0x2AAA8, PS2 0x1E1E80 (EXT 5e). The fighter's
 * base and chest matrices (COP 0x62, 0x67), the waist/chest/head parts
 * (calc_unit_1) and the four limbs by two-bone IK (0x6B), then adj_taikaku
 * and g7+0x1A8 to +0xC4C. Unlike the PS2: the limb records are the fixed
 * table 0x2C194 (20 bytes each), one 0x13 sync before each IK and none
 * before the matrices, and adj_taikaku instead of the reach scale. */
#pragma once

static void m1_cra_w3(uint32_t a, uint32_t b, uint32_t c)
{
    ages_cop_w(a);
    ages_cop_w(b);
    ages_cop_w(c);
}

static uint32_t m1_cra_p2(void)
{
    return (ages_ld8(AGES_G(7) + 4) & 1u) ? 0x100u : 0u;
}

/* One limb: sync, 0x6B with its record, angles, point, lengths, the two
 * slots and the side; steps g0/g1/g3..g6; the reply dropped. */
static void m1_cra_ik(uint32_t lo, uint32_t hi, uint32_t side)
{
    uint32_t g6 = AGES_G(6), g5 = AGES_G(5), g3 = AGES_G(3), g4 = AGES_G(4);
    m1_cra_w3(0x09801313, 0x09801313, 0x09801313);
    (void)ages_cop_r();
    ages_cop_w(0x35806B6B);
    m1_cra_w3(ages_ld32(g6), ages_ld32(g6 + 4), ages_ld32(g6 + 8));
    m1_cra_w3((uint32_t)ages_ld16s(g5 + 4), (uint32_t)ages_ld16s(g5 + 2),
              (uint32_t)ages_ld16s(g5));
    m1_cra_w3((uint32_t)ages_ld16s(g3 + 2), (uint32_t)ages_ld16s(g3),
              (uint32_t)ages_ld16s(g3 + 4));
    m1_cra_w3(ages_ld32(g4), ages_ld32(g4 + 4), ages_ld32(g4 + 8));
    ages_cop_w(ages_ld32(g6 + 0x10));
    ages_cop_w(ages_ld32(g6 + 0xC));
    uint32_t p2 = m1_cra_p2();
    m1_cra_w3(lo + p2, hi + p2, side);
    AGES_G(6) += 20;
    AGES_G(5) += 6;
    AGES_G(4) += 12;
    AGES_G(3) += 6;
    AGES_G(1) += 1;
    AGES_G(0) += 14;
    (void)ages_cop_r();
}

static void m1_cra_base(uint32_t rob)
{
    ages_cop_w(0x31006262);
    m1_cra_w3(ages_ld32(rob + 0x80), ages_ld32(rob + 0x84), ages_ld32(rob + 0x88));
    m1_cra_w3((uint32_t)ages_ld16s(rob + 0x144), (uint32_t)ages_ld16s(rob + 0x142),
              (uint32_t)ages_ld16s(rob + 0x140));
    m1_cra_w3((uint32_t)ages_ld16s(rob + 0xC02), (uint32_t)ages_ld16s(rob + 0xC00),
              (uint32_t)ages_ld16s(rob + 0xC04));
    ages_cop_w(0x33806767);
    ages_cop_w(0x3A00u + m1_cra_p2());
}

static uint32_t afn_calc_rob_angle_cont(int entry)
{
    uint32_t rob = AGES_G(7);
    m1_cra_base(rob);
    AGES_G(6) = 0x2C194;
    AGES_G(5) = rob + 0xBA8;
    AGES_G(4) = rob + 0xB00;
    AGES_G(1) = 1;
    AGES_G(0) = rob + 0x146;
    AGES_G(3) = rob + 0xC06;
    ages_cop_w(0x00800101);
    afn_calc_unit_1(0);
    ages_cop_w(0x33806767);
    ages_cop_w(0x3A0Cu + m1_cra_p2());
    ages_cop_w(0x00800101);
    afn_calc_unit_1(0);
    ages_cop_w(0x01000202);
    ages_cop_w(0x00800101);
    m1_cra_ik(0x3A30, 0x3A24, 0);
    ages_cop_w(0x01000202);
    m1_cra_ik(0x3A54, 0x3A48, 0);
    ages_cop_w(0x01000202);
    afn_calc_unit_1(0);
    ages_cop_w(0x00800101);
    m1_cra_ik(0x3A84, 0x3A78, 1);
    ages_cop_w(0x01000202);
    m1_cra_ik(0x3AA8, 0x3A9C, 1);
    ages_cop_w(0x01000202);
    afn_adj_taikaku(0);
    ages_st16(rob + 0xC4C, ages_ld16(rob + 0x1A8));
    if (entry) ages_i960_ret();
    return 0;
}
