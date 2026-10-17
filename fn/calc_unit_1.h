/* calc_unit_1: i960 0x2BBE0, PS2 0x1E3840. One part of the unit-matrix
 * chain (g1 = part; g0/g3/g4/g5/g6 its records, advanced on return): the
 * offset, the joint's 0x3F turn, the motion angles, then the look angles,
 * stored (g7+0xBDC bit) or aimed at the point g4 (COP 0x6A, 0x27, 0x2D);
 * part 1 eases the waist (g7+0xBE6), part 2 turns the head a step at a time.
 * Unlike the PS2: the step-limit flag is g7+0x7D7 and the bit test is g1
 * mod 32. */
#pragma once

static void m1_cu1_cc_cmpi(int32_t a, int32_t b)
{
    AGES_AC = (AGES_AC & ~7u) | (a < b ? 4u : a == b ? 2u : 1u);
}

/* Part 1: ease g7+0xBE6 towards the enemy's direction. */
static void m1_cu1_waist(uint32_t rob)
{
    uint32_t flags = ages_ld32(rob + 0x1A4), r[3];
    if ((flags & 0x16000u) || !(flags & 0x20Cu)) {
        ages_st16(rob + 0xBE6, 0);
        return;
    }
    ages_cop_w(0x35006A6A);
    ages_ldn(r, rob + 0xB0C, 3);
    gems_cop_wn(r, 3);
    gems_cop_rn(r, 3);
    ages_cop_w(0x13802727);
    ages_cop_w(r[0]);
    ages_cop_w(r[2]);
    int32_t mir = (ages_ld32(rob) & 0x40u) != 0;
    int32_t want = (int16_t)ages_cop_r();
    if (mir) want = -want;
    if (want < 0) want = 0;
    else if (want <= 0x2000) want >>= 1;
    else if (want <= 0x6000) want = 0x1000;
    else return;
    int32_t cur = ages_ld16s(rob + 0xBE6);
    if (mir) cur = -cur;
    int32_t d = want - cur;
    if (d > 0x100) {
        want = cur + 0x100;
        if (want > 0x1000) want = 0x1000;
    } else if (d < -0x100) {
        want = cur - 0x100;
        if (want < 0) want = 0;
    }
    if (mir) want = -want;
    ages_st16(rob + 0xBE6, (uint32_t)want);
}

/* v clamped to +-lim; sets the CC of the clamp's last compare. */
static int32_t m1_cu1_clamp(int32_t v, int32_t lim)
{
    m1_cu1_cc_cmpi(lim, v);
    if (!(lim > v))
        return lim;
    m1_cu1_cc_cmpi(-lim, v);
    return -lim <= v ? v : -lim;
}

/* Part 2, the head: turn towards r a step at a time. */
static void m1_cu1_head(uint32_t rob, uint32_t *r)
{
    int turn = 1;
    if ((int32_t)r[1] < 0) {
        r[1] = 0;
        float x = ages_u2f(r[0]);
        if ((int32_t)r[0] < 0)
            turn = (-0.5f > x) || ages_ld16(AGES_G(0) + 4) != 0;
        else
            turn = (0.5f < x) || ages_ld16(AGES_G(0) + 4) != 0x8000;
    }
    int slow = ages_ld8(rob + 0x7D7) != 0;
    if (turn) {
        ages_cop_w(0x13802727);
        ages_cop_w(r[0]);
        ages_cop_w(r[1]);
        int32_t neg = (int32_t)(0u - ages_cop_r());
        int32_t cur = ages_ld16s(AGES_G(0) + 4);
        int32_t a = m1_cu1_clamp(neg - cur, slow ? 0x800 : 0x2000);
        ages_st16(AGES_G(0) + 4, (uint32_t)(a + cur));
    }
    ages_cop_w(0x16802D2D);
    ages_cop_w(r[0]);
    ages_cop_w(r[1]);
    uint32_t len = ages_cop_r();
    ages_cop_w(0x13802727);
    ages_cop_w(len);
    ages_cop_w(r[2]);
    int32_t b = (int16_t)ages_cop_r();
    AGES_G(2) = (uint32_t)ages_ld16s(AGES_G(0) + 2);
    int32_t cur = (int32_t)AGES_G(2);
    b = m1_cu1_clamp(b - cur, slow ? 0x400 : 0x2000) + cur;
    ages_st16(AGES_G(0) + 2, (uint32_t)m1_cu1_clamp(b, 0x2000));
}

/* The joint: ang_x for part 2, offset, 0x3F turn, motion angles. */
static void m1_cu1_joint(uint32_t rob)
{
    uint32_t r[3];
    if (AGES_G(1) == 2) {
        ages_cop_w(0x04000808);
        ages_cop_w((uint32_t)ages_ld16s(rob + 0xBE6));
    }
    ages_cop_w(0x03000606);
    ages_ldn(r, AGES_G(6), 3);
    gems_cop_wn(r, 3);
    ages_cop_w(0x1F803F3F);
    r[0] = (uint32_t)ages_ld16s(AGES_G(5) + 4);
    r[1] = (uint32_t)ages_ld16s(AGES_G(5) + 2);
    r[2] = (uint32_t)ages_ld16s(AGES_G(5));
    gems_cop_wn(r, 3);
    (void)ages_cop_r();
    AGES_G(5) += 6;
    static const uint32_t cmd[3] = {0x04800909, 0x04000808, 0x05000A0A};
    static const uint8_t off[3] = {2, 0, 4};
    for (int i = 0; i < 3; i++) {
        ages_cop_w(cmd[i]);
        ages_cop_w((uint32_t)ages_ld16s(AGES_G(3) + off[i]));
    }
    AGES_G(3) += 6;
}

/* The aimed look angles; returns 0 when the head took them itself. */
static int m1_cu1_aim(uint32_t rob, uint32_t *angz, uint32_t *angy)
{
    uint32_t r[3];
    if (AGES_G(1) == 1)
        m1_cu1_waist(rob);
    ages_cop_w(0x35006A6A);
    ages_ldn(r, AGES_G(4), 3);
    gems_cop_wn(r, 3);
    gems_cop_rn(r, 3);
    if (AGES_G(1) != 2) {
        AGES_AC = (AGES_AC & ~7u) | (2u < AGES_G(1) ? 4u : 1u);
    } else if (ages_ld32(rob) & 0x200000u) {
        ages_st32(rob, ages_ld32(rob) & ~0x200000u);
        AGES_AC = (AGES_AC & ~7u) | 2u;
    } else {
        m1_cu1_head(rob, r);
        return 0;
    }
    ages_cop_w(0x13802727);
    ages_cop_w(r[0]);
    ages_cop_w(r[1]);
    *angz = 0u - (uint32_t)(int16_t)ages_cop_r();
    ages_st16(AGES_G(0) + 4, *angz);
    ages_cop_w(0x16802D2D);
    ages_cop_w(r[0]);
    ages_cop_w(r[1]);
    uint32_t len = ages_cop_r();
    ages_cop_w(0x13802727);
    ages_cop_w(len);
    ages_cop_w(r[2]);
    *angy = (uint32_t)(int16_t)ages_cop_r();
    ages_st16(AGES_G(0) + 2, *angy);
    return 1;
}

static uint32_t afn_calc_unit_1(int entry)
{
    uint32_t rob = AGES_G(7), angz, angy;
    m1_cu1_joint(rob);
    int send = 1;
    if (ages_ld8(rob + 0xBDC) & (1u << (AGES_G(1) & 31u))) {
        angz = ages_ld16(AGES_G(0) + 4);
        angy = ages_ld16(AGES_G(0) + 2);
        AGES_AC = (AGES_AC & ~7u) | 2u;
    } else {
        send = m1_cu1_aim(rob, &angz, &angy);
    }
    if (send) {
        ages_cop_w(0x05000A0A);
        ages_cop_w(angz);
        ages_cop_w(0x04800909);
        ages_cop_w(angy);
    }
    AGES_G(6) += 12;
    AGES_G(4) += 12;
    AGES_G(0) += 6;
    AGES_G(1) += 1;
    if (entry) ages_i960_ret();
    return 0;
}
