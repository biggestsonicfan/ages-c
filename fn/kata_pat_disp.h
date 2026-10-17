/* kata_pat_disp: i960 0x32A90 (called by rob_disp for unit 1).
 *
 * The shoulder pads: the chest's lean (0x20C against 0x1F4) gives two
 * angles at 0xACA/0xAC8 of the side's block (g6 = [0x500844], +0x60 for
 * fighter 1); then for each shoulder whose entry in the character's table
 * (0x642147C[0x1B0]) is not -1, find the arm's point (0xD4, 0xB0) against
 * the chest, lift the pad by it, save its matrix at 0x1AC0/0x1AF0(g7) and
 * draw it, with an optional second model (slot 0x1B28/0x1B2A).
 * g6 and g3 (the last pad's entry) are left as the i960 leaves them. */
#pragma once
#include "kosi_mino_disp.h"

#define KP(o) (AGES_G(6) + (o))

typedef struct {
    uint32_t tab, skip, k, src, pt, out0, out1, out2, tr, z, y, mat, slot;
    bool right;
} kp_pad_t;

static const kp_pad_t kp_pads[2] = {
    { 0, 6, 0x3A48, 0xD4, 0xA9C, 0xADC, 0xAD8, 0xAE0, 0xA90, 0xABE, 0xABC, 0x1AC0, 0x1B28, false },
    { 4, 3, 0x3A24, 0xB0, 0xAA8, 0xAD0, 0xACC, 0xAD4, 0xA84, 0xAB8, 0xAB6, 0x1AF0, 0x1B2A, true },
};

/* 0x32AA4: the side's block and the chest's lean. */
static void kp_lean(void)
{
    uint32_t a[3], b[3];
    AGES_G(6) = ages_ld32(0x500844);
    if (dsp_cmpo(0, ages_ld8(AGES_G(7) + 4)) != 2) AGES_G(6) += 0x60;
    km_op13();
    ages_ldn(a, AGES_G(7) + 0x20C, 3);
    ages_ldn(b, AGES_G(7) + 0x1F4, 3);
    uint32_t dy = dsp_subr(b[1], a[1]);
    uint32_t dz = dsp_subr(b[2], a[2]);
    uint32_t dx = dsp_subr(b[0], a[0]);
    uint32_t ry = dsp_cop2(0x13802727u, dz, dx);
    ages_st16(KP(0xACA), ry);
    uint32_t h = dsp_cop2(0x12002424u, ry, dx);
    h = dsp_addr(h, dsp_cop2(0x12802525u, ry, dz));
    ages_st16(KP(0xAC8), dsp_cop2(0x13802727u, h, dy));
}

/* The arm's point turned into the chest's frame; its lift angle. */
static uint32_t kp_lift(const kp_pad_t *p)
{
    uint32_t v[3];
    dsp_push();
    ages_cop_w(0x01800303u);
    ages_cop_w(0x34806969u);
    ages_cop_w((ages_ld8(AGES_G(7) + 4) & 1u) ? p->k + 0x100u : p->k);
    ages_cop_w((uint32_t)ages_ld16s(AGES_G(7) + 0x26));
    dsp_trans(AGES_G(7) + p->src);
    ages_cop_w(0x07800F0Fu);
    gems_cop_rn(v, 3);
    ages_stn(KP(p->pt), v, 3);
    dsp_pop();
    uint32_t ax = (uint32_t)ages_ld16s(KP(0xAC8)), ay = (uint32_t)ages_ld16s(KP(0xACA));
    ages_ldn(v, KP(p->pt), 3);
    uint32_t r10 = dsp_cop2(0x12002424u, ax, v[1]);
    uint32_t r11 = dsp_cop2(0x12002424u, ay, v[0]);
    r11 = dsp_addr(r11, dsp_cop2(0x12802525u, ay, v[2]));
    r11 = dsp_cop2(0x12802525u, ax, r11);
    ages_st32(KP(p->out0), r10);
    ages_st32(KP(p->out1), r11);
    ages_st32(KP(p->out2), dsp_addr(r10, r11));
    uint32_t len = ages_ld32(AGES_G(7) + p->src);
    uint32_t h = ages_ld32(KP(p->out2));
    if (dsp_bbs(h, 31)) return 0;
    uint32_t s = dsp_subr(dsp_mulr(h, h), dsp_mulr(len, len));
    s = dsp_cop1(0x0D001A1Au, s);
    uint32_t a = dsp_cop2(0x13802727u, s, h);
    uint32_t lim = (uint32_t)ages_ld16s(AGES_G(3) + 2);
    if (dsp_cmpo(lim, a) != 1) a = lim;
    return a;
}

/* Draw model m with g1 = the fighter's side. */
static void kp_draw(uint32_t m)
{
    AGES_G(1) = ages_ld8(AGES_G(7) + 4);
    dsp_draw(m);
}

static void kp_pad(const kp_pad_t *p)
{
    uint32_t e = ages_ld32(0x642147Cu + ages_ld8(AGES_G(7) + 0x1B0) * 4u);
    AGES_G(3) = ages_ld32(e + p->tab);
    if (dsp_cmpi(0xFFFFFFFFu, (uint32_t)ages_ld16s(AGES_G(3))) == 2
        || dsp_bbs(ages_ld16(AGES_G(7) + 0x1F48), p->skip)) {
        ages_st16(AGES_G(7) + p->slot, 0);
        return;
    }
    int32_t a = (int32_t)kp_lift(p);
    a = p->right ? ages_ld16s(KP(0xAB4)) - a : ages_ld16s(KP(0xABA)) + a;
    ages_st16(KP(0xAC0), (uint32_t)a);
    uint32_t m = ages_ld16(AGES_G(3));
    km_frame(true, KP(p->tr));
    dsp_cmd(DSP_ZROT, (uint32_t)ages_ld16s(KP(p->z)));
    int32_t y = ages_ld16s(KP(p->y));
    dsp_cmd(DSP_YROT, (uint32_t)(p->right ? y : -y));
    dsp_cmd(DSP_XROT, (uint32_t)ages_ld16s(KP(0xAC0)));
    uint32_t mat[12];
    ages_cop_w(0x02800505u);
    gems_cop_rn(mat, 12);
    ages_stn(AGES_G(7) + p->mat, mat, 12);
    dsp_cmd(0x23004646u, 3);
    kp_draw(m);
    m = ages_ld16(AGES_G(3) + 0x16);
    if (dsp_cmpo(0, m) != 2) {
        dsp_trans(AGES_G(3) + 0x18);
        ages_st16(AGES_G(7) + p->slot, m);
        kp_draw(m);
    }
    dsp_pop();
}

static uint32_t afn_kata_pat_disp(int entry)
{
    dsp_cmd(0x21804343u, 3);
    kp_lean();
    kp_pad(&kp_pads[0]);
    kp_pad(&kp_pads[1]);
    if (entry) ages_i960_ret();
    return 0;
}
