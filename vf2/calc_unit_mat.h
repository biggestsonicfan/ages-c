/* VF2 calc_unit_mat: i960 0x16504, PS2 sysCopPatchCalcUnitMat 0x192780
 * (EXT 4f, decomp/VF2/FN/ext4f_*). Walks fighter g7's skeleton on the TGP's
 * matrix stack: each of the 16 parts is stored (0x35, slot 12 * n), its world
 * position read back (0x0F) into g7+0x1F4 + 12n and its balls sent (0x176A0);
 * then the step length (0x2D).
 *
 * Fighting Vipers' (fn/calc_unit_mat.h) with VF2's differences: the TGP's
 * rotations are 0x6C/0x6D/0x6E and each angle that is not 0 also goes to the
 * table at 0x90E000 (pointer 0x5001E4, its low byte moved on 4); a zero angle
 * sends nothing. 0x3F does not answer, there is no 0x51 and no
 * rob_ball_data_make; the extra model is at [g7+0x190]+0x38 and the last
 * position at g7+0x6E0.
 *
 * The PS2 resumes the i960 at its `ret` (0x1769C): the C pops the frame. */
#pragma once

#include "set_obj.h"   /* vf2_tgp_w, vf2_tgp_r, vf2_cmpo */

/* `ldis off(g7)`: a non-zero angle into the table, then the rotation. */
static void vf2_cum_ang(uint32_t cmd, uint32_t off)
{
    uint32_t v = (uint32_t)ages_ld16s(AGES_G(7) + off);
    if (v == 0)
        return;
    uint32_t p = ages_ld32(0x5001E4);
    ages_st32(0x90E000u + p, v);
    ages_st8(0x5001E4, p + 4u);
    vf2_tgp_w(cmd);
}

/* `ldt off(g7); stt (g11)[g12]`: three words to the FIFO. */
static void vf2_cum_w3(uint32_t a)
{
    for (uint32_t i = 0; i < 3; i++)
        vf2_tgp_w(ages_ld32(a + 4u * i));
}

static void vf2_cum_trans(uint32_t off)
{
    vf2_tgp_w(0x03000606);
    vf2_cum_w3(AGES_G(7) + off);
}

/* 0x69: the fighter's slot (0x3A00 / 0x3B00 by g7+4 bit 0) + off. */
static void vf2_cum_ik(uint32_t off)
{
    vf2_tgp_w(0x34806969);
    vf2_tgp_w(((ages_ld8(AGES_G(7) + 4) & 1u) ? 0x3B00u : 0x3A00u) + off);
    vf2_tgp_w((uint32_t)ages_ld16s(AGES_G(7) + 0x26));
}

static void vf2_cum_3f(uint32_t a, uint32_t b, uint32_t c)
{
    vf2_tgp_w(0x1F803F3F);
    vf2_tgp_w((uint32_t)ages_ld16s(AGES_G(7) + a));
    vf2_tgp_w((uint32_t)ages_ld16s(AGES_G(7) + b));
    vf2_tgp_w((uint32_t)ages_ld16s(AGES_G(7) + c));
}

/* `stt q,(g11)[g12]`: three words of q to the FIFO. */
static void vf2_cum_q3(const uint32_t *q)
{
    for (int i = 0; i < 3; i++)
        vf2_tgp_w(q[i]);
}

/* 0x176A0: part g0's balls. Each ball (an index list per part in the ROM) is
 * sent to 0x39; unless [[0x500828]] bit 31, it is also transformed (0x29) and
 * the quad stored at g7+0xD00 + 4 * index, its fourth word the ROM's. */
static void vf2_cum_ball(uint32_t n)
{
    uint32_t list = ages_ld32(0x2007BEEu + n * 4u);
    int32_t k = (int32_t)ages_ld8(0x2007C68u + n);
    uint32_t flag = ages_ld32(ages_ld32(0x500828));
    uint32_t q[4];
    while (k-- > 0) {
        uint32_t i = ages_ld8(list + (uint32_t)k);
        ages_ldn(q, 0x20078A8u + i * 4u, 4);
        vf2_tgp_w(0x1C803939);
        vf2_cum_q3(q);
        if (flag & 0x80000000u)
            continue;
        vf2_tgp_w(0x14802929);
        vf2_cum_q3(q);
        for (int j = 0; j < 3; j++)
            q[j] = vf2_tgp_r();
        ages_stn(AGES_G(7) + 0xD00u + i * 4u, q, 4);
    }
    AGES_AC = (AGES_AC & ~7u) | 2u;
}

static void vf2_cum_part(uint32_t n)
{
    uint32_t v[3];
    vf2_tgp_w(0x1A803535);
    vf2_tgp_w(ages_ld8(AGES_G(7) + 4));
    vf2_tgp_w(12u * n);
    vf2_tgp_w(0x07800F0F);
    for (int i = 0; i < 3; i++)
        v[i] = vf2_tgp_r();
    ages_stn(AGES_G(7) + 0x1F4u + 12u * n, v, 3);
    AGES_G(0) = n;
    vf2_cum_ball(n);
}

/* trans, 0x69, part: one IK-placed part. */
static void vf2_cum_ik_part(uint32_t trans, uint32_t slot, uint32_t n)
{
    vf2_cum_trans(trans);
    vf2_cum_ik(slot);
    vf2_cum_part(n);
}

/* A hand or foot: three angles of the motion, three of the rob. */
static void vf2_cum_end(const uint16_t *a, uint32_t n)
{
    static const uint32_t cmd[6] = {0x37006E6E, 0x36806D6D, 0x36006C6C,
                                    0x36806D6D, 0x36006C6C, 0x37006E6E};
    for (int i = 0; i < 6; i++)
        vf2_cum_ang(cmd[i], a[i]);
    vf2_cum_part(n);
}

/* 0x11: the root's 9 words, saved at 0x501024. */
static void vf2_cum_11(void)
{
    vf2_tgp_w(0x08801111);
    for (uint32_t i = 0; i < 9; i++)
        vf2_tgp_w(ages_ld32(0x501024u + 4u * i));
}

/* The geometry port, `st (g10)[g12]`. */
static void vf2_cum_port(uint32_t v)
{
    ages_st32(AGES_G(10) + AGES_G(12), v);
}

/* A polygon vertex: its own at p, or (index >= 8) a transformed point. */
static void vf2_cum_vtx(uint32_t p, uint32_t idx)
{
    if (idx >= 8)
        p = AGES_G(7) + 0x1800 + idx * 12u;
    for (uint32_t i = 0; i < 3; i++)
        vf2_cum_port(ages_ld32(p + 4u * i));
}

/* The extra model's polygons straight to the geometry port. */
static void vf2_cum_polys(uint32_t mdl)
{
    uint32_t p = mdl + 0xD4, cnt = ages_ld32(mdl + 0xD0);
    uint32_t hdr = cnt * 10u + 1u, idx = p + hdr * 4u;
    uint32_t slot = (uint32_t)ages_ld8(AGES_G(7) + 4) << 8;
    ages_st32(AGES_G(10) + 0x50, slot);
    vf2_cum_port(slot);
    vf2_cum_port(hdr);
    do {
        for (uint32_t i = 0; i < 4; i++)
            vf2_cum_port(ages_ld32(p + 4u * i));
        vf2_cum_vtx(p + 0x10, ages_ld8(idx));
        vf2_cum_vtx(p + 0x1C, ages_ld8(idx + 1));
        p += 0x28;
        idx += 2;
    } while (cnt-- > 1);
    vf2_cum_port(ages_ld32(p));
}

/* The extra model at [g7+0x190]+0x38: 16 points to g7+0x1860, polygons. */
static void vf2_cum_model(void)
{
    uint32_t mdl = ages_ld32(ages_ld32(AGES_G(7) + 0x190) + 0x38), v[3];
    if (mdl == 0)
        return;
    vf2_tgp_w(0x00800101);
    vf2_tgp_w(0x06000C0C);
    for (int i = 0; i < 3; i++)
        vf2_tgp_w(0x09801313);
    (void)vf2_tgp_r();
    vf2_tgp_w(0x1B803737);
    vf2_tgp_w(ages_ld8(AGES_G(7) + 4));
    vf2_tgp_w(12);
    for (uint32_t i = 0; i < 16; i++) {
        vf2_tgp_w(0x14802929);
        vf2_cum_w3(mdl + 0x70 + 12u * i);
        for (int j = 0; j < 3; j++)
            v[j] = vf2_tgp_r();
        ages_stn(AGES_G(7) + 0x1860 + 12u * i, v, 3);
    }
    mdl = ages_ld32(ages_ld32(AGES_G(7) + 0x190) + 0x38);
    vf2_cum_polys(mdl);
    vf2_tgp_w(0x01000202);
}

/* The root at (0, g7+0x650, 0), turned by g7+0x26; 0x12's 9 words saved. */
static void vf2_cum_root(uint32_t rob)
{
    vf2_tgp_w(0x1C003838);
    vf2_tgp_w(ages_ld8(rob + 4));
    vf2_tgp_w(0x00800101);
    vf2_tgp_w(0x01800303);
    vf2_tgp_w(0x07000E0E);
    vf2_tgp_w(0);
    vf2_tgp_w(ages_ld32(rob + 0x650));
    vf2_tgp_w(0);
    vf2_cum_trans(0x18);
    vf2_cum_ang(0x36806D6D, 0x26);
    vf2_tgp_w(0x09001212);
    for (uint32_t i = 0; i < 9; i++)
        ages_st32(0x501024u + 4u * i, vf2_tgp_r());
    if (!(ages_ld32(rob) & 4u)) {
        uint32_t y = ages_ld32(rob + 0x84);
        vf2_tgp_w(0x03000606);
        vf2_tgp_w(ages_ld32(rob + 0x80));
        vf2_tgp_w(ages_f2u(ages_u2f(y) + ages_u2f(ages_ld32(rob + 0xC50))));
        vf2_tgp_w(ages_ld32(rob + 0x88));
    } else if (ages_ld8(rob + 0x840) & 1u) {
        vf2_tgp_w(0x03000606);
        vf2_tgp_w(ages_ld32(rob + 0x80));
        vf2_tgp_w(0);
        vf2_tgp_w(ages_ld32(rob + 0x88));
    }
}

/* Waist, chest, head, the arms. */
static void vf2_cum_upper(void)
{
    static const uint16_t hand1[6] = {0x15E, 0x15C, 0x15A, 0xBEA, 0xBE8, 0xBEC};
    static const uint16_t hand2[6] = {0x16C, 0x16A, 0x168, 0xBF0, 0xBEE, 0xBF2};
    vf2_cum_ik(0x00);
    vf2_cum_part(0);
    vf2_tgp_w(0x00800101);
    vf2_cum_trans(0x8C);
    vf2_cum_ik(0x0C);
    vf2_cum_ang(0x36006C6C, 0xBE6);
    vf2_cum_part(1);
    vf2_tgp_w(0x00800101);
    vf2_cum_trans(0x98);
    vf2_cum_3f(0xBB2, 0xBB0, 0xBAE);
    vf2_cum_ang(0x36806D6D, 0xC0E);
    vf2_cum_ang(0x36006C6C, 0xC0C);
    vf2_cum_ang(0x37006E6E, 0xC10);
    vf2_cum_ang(0x37006E6E, 0x150);
    vf2_cum_ang(0x36806D6D, 0x14E);
    vf2_cum_part(2);
    vf2_tgp_w(0x01000202);
    vf2_tgp_w(0x00800101);
    vf2_cum_ik_part(0xA4, 0x24, 3);
    vf2_cum_ik_part(0xB0, 0x30, 4);
    vf2_cum_trans(0xBC);
    vf2_cum_end(hand1, 5);
    vf2_tgp_w(0x01000202);
    vf2_cum_ik_part(0xC8, 0x48, 6);
    vf2_cum_ik_part(0xD4, 0x54, 7);
    vf2_cum_trans(0xE0);
    vf2_cum_end(hand2, 8);
    vf2_tgp_w(0x01000202);
}

/* Hips, the extra model, the legs. */
static void vf2_cum_lower(void)
{
    static const uint16_t foot1[6] = {0x180, 0x17E, 0x17C, 0xBF6, 0xBF4, 0xBF8};
    static const uint16_t foot2[6] = {0x18E, 0x18C, 0x18A, 0xBFC, 0xBFA, 0xBFE};
    vf2_cum_trans(0xEC);
    vf2_cum_3f(0xBC4, 0xBC2, 0xBC0);
    vf2_cum_ang(0x36806D6D, 0xC20);
    vf2_cum_ang(0x36006C6C, 0xC1E);
    vf2_cum_ang(0x37006E6E, 0xC22);
    vf2_cum_ang(0x37006E6E, 0x172);
    vf2_cum_ang(0x36806D6D, 0x170);
    vf2_cum_part(9);
    vf2_cum_model();
    vf2_tgp_w(0x00800101);
    vf2_cum_ik_part(0xF8, 0x78, 10);
    vf2_cum_ik_part(0x104, 0x84, 11);
    vf2_cum_trans(0x110);
    vf2_cum_11();
    vf2_cum_end(foot1, 12);
    vf2_tgp_w(0x01000202);
    vf2_cum_ik_part(0x11C, 0x9C, 13);
    vf2_cum_ik_part(0x128, 0xA8, 14);
    vf2_cum_trans(0x134);
    vf2_cum_11();
    vf2_cum_end(foot2, 15);
    vf2_tgp_w(0x01000202);
}

static void vf2_cum_cc(uint32_t cc)
{
    AGES_AC = (AGES_AC & ~7u) | cc;
}

/* i960 cmpr a,b: 100 if a < b, 010 if equal, 001 if a > b, 000 unordered. */
static uint32_t vf2_cum_cmpr(float a, float b)
{
    return a < b ? 4u : a == b ? 2u : a > b ? 1u : 0u;
}

/* Keeps the last position at 0x6E0 (old to 0x1E0C); in the 9/9 mode with
 * g7 bit 7 asks the TGP for the step. Only the CC comes of the tests. */
static void vf2_cum_step(uint32_t rob)
{
    uint32_t nu[3], ou[3];
    if (ages_ld32(0x508000) & 0x20u) { vf2_cum_cc(2); return; }
    ages_ldn(nu, rob + 0x1F4, 3);
    ages_ldn(ou, rob + 0x6E0, 3);
    ages_stn(rob + 0x1E0C, ou, 3);
    ages_stn(rob + 0x6E0, nu, 3);
    float dx = ages_u2f(nu[0]) - ages_u2f(ou[0]);
    float dz = ages_u2f(nu[2]) - ages_u2f(ou[2]);
    uint32_t a = ages_ld8(0x50002B), b = ages_ld8(0x500031);
    if (a != 9) { vf2_cmpo(9, a); return; }
    if (b != 9) { vf2_cmpo(9, b); return; }
    if (!(ages_ld32(rob) & 0x80u)) { vf2_cum_cc(0); return; }
    vf2_tgp_w(0x16802D2D);
    vf2_tgp_w(ages_f2u(dx));
    vf2_tgp_w(ages_f2u(dz));
    uint32_t cc = vf2_cum_cmpr(ages_u2f(vf2_tgp_r()), ages_u2f(0x3F333333u));
    vf2_cum_cc(cc);
    if (cc == 4u)
        return;
    uint32_t f = ages_ld32(rob + 0x1A4);
    if ((f >> 14) & 1u) { vf2_cum_cc(2); return; }
    if ((f >> 17) & 1u) { vf2_cum_cc(2); return; }
    if ((f >> 18) & 1u) { vf2_cum_cc(2); return; }
    vf2_cum_cc((f >> 19) & 1u ? 2u : 0u);
}

static uint32_t afn_vf2_calc_unit_mat(int entry)
{
    (void)entry;
    uint32_t rob = AGES_G(7);
    vf2_cum_root(rob);
    vf2_cum_upper();
    vf2_cum_lower();
    vf2_cum_step(rob);
    ages_i960_ret();
    return 0;
}
