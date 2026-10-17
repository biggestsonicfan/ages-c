/* calc_unit_mat: i960 0x17160, PS2 0x1D8500 (EXT 57). Walks fighter g7's
 * skeleton on the COP's matrix stack: each of the 16 parts is stored (0x35,
 * slot 12 * n), its world position read back (0x0F) into g7+0x1F4 + 12n and
 * its balls sent (set_coli_ball_data); 0x51 reads three hip/leg angles to
 * 0xAD0..0xAE0; then rob_ball_data_make and the step length (0x2D).
 * Unlike the PS2: no part scales, no held body position; an extra model
 * (g7+0x190 -> +0x24) has 16 points transformed to g7+0x1860 and its
 * polygons sent straight to the geometry port (g10). */
#pragma once

static void m1_cum_ang(uint32_t cmd, uint32_t off)
{
    ages_cop_w(cmd);
    ages_cop_w((uint32_t)ages_ld16s(AGES_G(7) + off));
}

static void m1_cum_trans(uint32_t off)
{
    uint32_t v[3];
    ages_cop_w(0x03000606);
    ages_ldn(v, AGES_G(7) + off, 3);
    gems_cop_wn(v, 3);
}

/* 0x69: the fighter's slot (0x3A00 / 0x3B00 by g7+4 bit 0) + off. */
static void m1_cum_ik(uint32_t off)
{
    ages_cop_w(0x34806969);
    ages_cop_w(((ages_ld8(AGES_G(7) + 4) & 1u) ? 0x3B00u : 0x3A00u) + off);
    ages_cop_w((uint32_t)ages_ld16s(AGES_G(7) + 0x26));
}

static void m1_cum_3f(uint32_t a, uint32_t b, uint32_t c)
{
    ages_cop_w(0x1F803F3F);
    ages_cop_w((uint32_t)ages_ld16s(AGES_G(7) + a));
    ages_cop_w((uint32_t)ages_ld16s(AGES_G(7) + b));
    ages_cop_w((uint32_t)ages_ld16s(AGES_G(7) + c));
    (void)ages_cop_r();
}

static void m1_cum_part(uint32_t n)
{
    uint32_t v[3];
    ages_cop_w(0x1A803535);
    ages_cop_w(ages_ld8(AGES_G(7) + 4));
    ages_cop_w(12u * n);
    ages_cop_w(0x07800F0F);
    gems_cop_rn(v, 3);
    ages_stn(AGES_G(7) + 0x1F4u + 12u * n, v, 3);
    AGES_G(0) = n;
    afn_set_coli_ball_data(0);
}

/* trans, 0x69, part: one IK-placed part. */
static void m1_cum_ik_part(uint32_t trans, uint32_t slot, uint32_t n)
{
    m1_cum_trans(trans);
    m1_cum_ik(slot);
    m1_cum_part(n);
}

/* A hand or foot: three angles of the motion, three of the rob. */
static void m1_cum_end(const uint16_t *a, uint32_t n)
{
    static const uint32_t cmd[6] = {0x05000A0A, 0x04800909, 0x04000808,
                                    0x04800909, 0x04000808, 0x05000A0A};
    for (int i = 0; i < 6; i++)
        m1_cum_ang(cmd[i], a[i]);
    m1_cum_part(n);
}

/* 0x51: three angles read into g7+dst (the hip's z as 0x8000 - z). The i960
 * saves and restores every register around it. */
static void m1_cum_51(uint32_t dst, int flip)
{
    ages_cop_w(0x28805151);
    uint32_t a = (uint32_t)(int16_t)ages_cop_r();
    uint32_t b = (uint32_t)(int16_t)ages_cop_r();
    uint32_t c = (uint32_t)(int16_t)ages_cop_r();
    ages_st16(AGES_G(7) + dst, a);
    ages_st16(AGES_G(7) + dst + 2, b);
    ages_st16(AGES_G(7) + dst + 4, flip ? 0x8000u - c : c);
}

static void m1_cum_11(void)
{
    uint32_t v[3];
    ages_cop_w(0x08801111);
    for (uint32_t k = 0; k < 3; k++) {
        ages_ldn(v, 0x501024u + 12u * k, 3);
        gems_cop_wn(v, 3);
    }
}

/* A geometry-port register write, g14 parked at 0x50EFFC as the i960 does. */
static void m1_cum_port_reg(uint32_t off, uint32_t v)
{
    uint32_t g14 = AGES_G(14);
    ages_st32(0x50EFFC, g14);
    AGES_G(14) = v;
    ages_st32(AGES_G(10) + off, v);
    AGES_G(14) = ages_ld32(0x50EFFC);
}

static void m1_cum_port(uint32_t v)
{
    ages_st32(AGES_G(10) + AGES_G(12), v);
}

/* A polygon vertex: its own at p, or (index >= 8) a transformed point. */
static void m1_cum_vtx(uint32_t p, uint32_t idx)
{
    uint32_t v[3];
    if (idx >= 8)
        p = AGES_G(7) + 0x1800 + idx * 12u;
    ages_ldn(v, p, 3);
    for (int i = 0; i < 3; i++)
        m1_cum_port(v[i]);
}

/* The extra model's polygons straight to the geometry port. */
static void m1_cum_polys(uint32_t mdl)
{
    uint32_t p = mdl + 0xD4, cnt = ages_ld32(mdl + 0xD0);
    uint32_t hdr = cnt * 10u + 1u, idx = p + hdr * 4u;
    m1_cum_port_reg(0x50, 0x505);
    m1_cum_port((uint32_t)ages_ld8(AGES_G(7) + 4) << 8);
    m1_cum_port(hdr);
    do {
        for (int i = 0; i < 4; i++)
            m1_cum_port(ages_ld32(p + 4u * i));
        m1_cum_vtx(p + 0x10, ages_ld8(idx));
        m1_cum_vtx(p + 0x1C, ages_ld8(idx + 1));
        p += 0x28;
        idx += 2;
    } while (cnt-- > 1);
    m1_cum_port(ages_ld32(p));
    if (ages_ld8(0x500F00) == 2) {
        m1_cum_port_reg(0x100, 0x1010);
        m1_cum_port(0);
    }
}

/* The extra model at [g7+0x190]+0x24: 16 points to g7+0x1860, polygons. */
static void m1_cum_model(void)
{
    uint32_t mdl = ages_ld32(ages_ld32(AGES_G(7) + 0x190) + 0x24), v[3];
    if (mdl == 0)
        return;
    ages_cop_w(0x00800101);
    ages_cop_w(0x06000C0C);
    for (int i = 0; i < 3; i++)
        ages_cop_w(0x09801313);
    (void)ages_cop_r();
    ages_cop_w(0x1B803737);
    ages_cop_w(ages_ld8(AGES_G(7) + 4));
    ages_cop_w(12);
    for (uint32_t i = 0; i < 16; i++) {
        ages_cop_w(0x14802929);
        ages_ldn(v, mdl + 0x70 + 12u * i, 3);
        gems_cop_wn(v, 3);
        gems_cop_rn(v, 3);
        ages_stn(AGES_G(7) + 0x1860 + 12u * i, v, 3);
    }
    m1_cum_polys(mdl);
    ages_cop_w(0x01000202);
}

/* The root at (0, g7+0x678, 0), turned by g7+0x26; 0x12's 9 words saved. */
static void m1_cum_root(uint32_t rob)
{
    uint32_t v[3];
    if (!(ages_ld32(0x508000) & 0x20u)) {
        ages_ldn(v, rob + 0x1F4, 3);
        ages_stn(rob + 0xC7C, v, 3);
    }
    ages_cop_w(0x1C003838);
    ages_cop_w(ages_ld8(rob + 4));
    ages_cop_w(0x00800101);
    ages_cop_w(0x01800303);
    ages_cop_w(0x07000E0E);
    ages_cop_w(0);
    ages_cop_w(ages_ld32(rob + 0x678));
    ages_cop_w(0);
    m1_cum_trans(0x18);
    m1_cum_ang(0x04800909, 0x26);
    ages_cop_w(0x09001212);
    for (uint32_t k = 0; k < 3; k++) {
        gems_cop_rn(v, 3);
        ages_stn(0x501024u + 12u * k, v, 3);
    }
    if (!(ages_ld32(rob) & 4u)) {
        ages_cop_w(0x03000606);
        ages_ldn(v, rob + 0x80, 3);
        v[1] = ages_f2u(ages_u2f(v[1]) + ages_u2f(ages_ld32(rob + 0xC50)));
        gems_cop_wn(v, 3);
    } else if (ages_ld8(rob + 0x83F) & 1u) {
        ages_cop_w(0x03000606);
        ages_cop_w(ages_ld32(rob + 0x80));
        ages_cop_w(0);
        ages_cop_w(ages_ld32(rob + 0x88));
    }
}

/* Waist, chest, head, the arms. */
static void m1_cum_upper(void)
{
    static const uint16_t hand1[6] = {0x15E, 0x15C, 0x15A, 0xBEA, 0xBE8, 0xBEC};
    static const uint16_t hand2[6] = {0x16C, 0x16A, 0x168, 0xBF0, 0xBEE, 0xBF2};
    m1_cum_ik(0x00);
    m1_cum_part(0);
    ages_cop_w(0x00800101);
    m1_cum_trans(0x8C);
    m1_cum_ik(0x0C);
    m1_cum_ang(0x04000808, 0xBE6);
    m1_cum_part(1);
    ages_cop_w(0x00800101);
    m1_cum_trans(0x98);
    m1_cum_3f(0xBB2, 0xBB0, 0xBAE);
    m1_cum_ang(0x04800909, 0xC0E);
    m1_cum_ang(0x04000808, 0xC0C);
    m1_cum_ang(0x05000A0A, 0xC10);
    m1_cum_ang(0x05000A0A, 0x150);
    m1_cum_ang(0x04800909, 0x14E);
    m1_cum_part(2);
    ages_cop_w(0x01000202);
    ages_cop_w(0x00800101);
    m1_cum_ik_part(0xA4, 0x24, 3);
    m1_cum_ik_part(0xB0, 0x30, 4);
    m1_cum_trans(0xBC);
    m1_cum_end(hand1, 5);
    ages_cop_w(0x01000202);
    m1_cum_ik_part(0xC8, 0x48, 6);
    m1_cum_ik_part(0xD4, 0x54, 7);
    m1_cum_trans(0xE0);
    m1_cum_end(hand2, 8);
    ages_cop_w(0x01000202);
}

/* Hips, the extra model, the legs. */
static void m1_cum_lower(void)
{
    static const uint16_t foot1[6] = {0x180, 0x17E, 0x17C, 0xBF6, 0xBF4, 0xBF8};
    static const uint16_t foot2[6] = {0x18E, 0x18C, 0x18A, 0xBFC, 0xBFA, 0xBFE};
    m1_cum_trans(0xEC);
    m1_cum_3f(0xBC4, 0xBC2, 0xBC0);
    m1_cum_ang(0x04800909, 0xC20);
    m1_cum_ang(0x04000808, 0xC1E);
    m1_cum_ang(0x05000A0A, 0xC22);
    m1_cum_ang(0x05000A0A, 0x172);
    m1_cum_ang(0x04800909, 0x170);
    m1_cum_part(9);
    m1_cum_51(0xAD0, 1);
    m1_cum_model();
    ages_cop_w(0x00800101);
    m1_cum_ik_part(0xF8, 0x78, 10);
    ages_cop_w(0x00800101);
    m1_cum_51(0xADC, 0);
    ages_cop_w(0x01000202);
    m1_cum_ik_part(0x104, 0x84, 11);
    m1_cum_trans(0x110);
    m1_cum_11();
    m1_cum_end(foot1, 12);
    ages_cop_w(0x01000202);
    m1_cum_ik_part(0x11C, 0x9C, 13);
    ages_cop_w(0x00800101);
    m1_cum_51(0xAD6, 0);
    ages_cop_w(0x01000202);
    m1_cum_ik_part(0x128, 0xA8, 14);
    m1_cum_trans(0x134);
    m1_cum_11();
    m1_cum_end(foot2, 15);
    ages_cop_w(0x01000202);
}

static void m1_cum_cc(uint32_t cc)
{
    AGES_AC = (AGES_AC & ~7u) | cc;
}

/* Keeps the last position at 0x710 (old to 0x1E0C); in the 9/9 mode with
 * g7 bit 7 asks the COP for the step. Only the CC comes of the tests. */
static void m1_cum_step(uint32_t rob)
{
    uint32_t nu[3], ou[3];
    if (ages_ld32(0x508000) & 0x20u) { m1_cum_cc(2); return; }
    ages_ldn(nu, rob + 0x1F4, 3);
    ages_ldn(ou, rob + 0x710, 3);
    ages_stn(rob + 0x1E0C, ou, 3);
    ages_stn(rob + 0x710, nu, 3);
    float dx = ages_u2f(nu[0]) - ages_u2f(ou[0]);
    float dz = ages_u2f(nu[2]) - ages_u2f(ou[2]);
    uint32_t a = ages_ld8(0x50002B), b = ages_ld8(0x500031);
    if (a != 9) { m1_cum_cc(9u < a ? 4u : 1u); return; }
    if (b != 9) { m1_cum_cc(9u < b ? 4u : 1u); return; }
    if (!(ages_ld32(rob) & 0x80u)) { m1_cum_cc(0); return; }
    ages_cop_w(0x16802D2D);
    ages_cop_w(ages_f2u(dx));
    ages_cop_w(ages_f2u(dz));
    float len = ages_u2f(ages_cop_r()), lim = ages_u2f(0x3F333333u);
    if (len < lim) { m1_cum_cc(4); return; }
    uint32_t f = ages_ld32(rob + 0x1A4);
    if ((f >> 14) & 1u) { m1_cum_cc(2); return; }
    if ((f >> 17) & 1u) { m1_cum_cc(2); return; }
    if ((f >> 18) & 1u) { m1_cum_cc(2); return; }
    m1_cum_cc((f >> 19) & 1u ? 2u : 0u);
}

static uint32_t afn_calc_unit_mat(int entry)
{
    uint32_t rob = AGES_G(7);
    m1_cum_root(rob);
    m1_cum_upper();
    m1_cum_lower();
    afn_rob_ball_data_make(0);
    m1_cum_step(rob);
    if (entry) ages_i960_ret();
    return 0;
}
