/* kosi_mino_disp: i960 0x31E54 (called by rob_disp for unit 9).
 *
 * The skirt of characters 0, 1, 13 and 14: from where the thighs (0x128,
 * 0x104) are against the hip (COP get_point) work out the leg angles 0xAD8
 * and 0xADE, then draw four skirt panels, each opened by the legs' push
 * (COP 0x21/0x22 scaled) and saved as a matrix at 0x1A00/0x1A30/0x1A60/
 * 0x1A90(g7) with its model at 0x1B20.. Otherwise the four models are 0.
 * g9 is left at the character's entry, 0x6421DE0 + 16 * 0x1B0. */
#pragma once
#include "pky_kami_disp.h"

#define KM(o) (AGES_G(7) + (o))

static void km_op13(void)
{
    ages_cop_w(0x09801313u);
    ages_cop_w(0x09801313u);
    ages_cop_w(0x09801313u);
    (void)ages_cop_r();
}

/* A thigh against the hip: its point (from src) into pt, the swing into
 * sw and the leg angle into ang. */
static void km_side(uint32_t k, uint32_t src, uint32_t pt, uint32_t sw, uint32_t ang)
{
    uint32_t p[3];
    dsp_push();
    ages_cop_w(0x01800303u);
    ages_cop_w(0x34806969u);
    ages_cop_w((ages_ld8(KM(4)) & 1u) ? k + 0x100u : k);
    ages_cop_w((uint32_t)ages_ld16s(0x50A398));
    dsp_trans(KM(src));
    ages_cop_w(0x07800F0Fu);
    gems_cop_rn(p, 3);
    ages_stn(pt, p, 3);
    dsp_pop();
    ages_ldn(p, pt, 3);
    uint32_t a = (uint32_t)ages_ld16s(KM(0xAD4));
    uint32_t s = dsp_cop2(0x12002424u, a, p[2]);
    s = dsp_addr(s, dsp_cop2(0x12802525u, a, p[1]));
    ages_st32(sw, s);
    ages_st16(KM(ang), dsp_cop2(0x13802727u, s, p[0]));
}

/* 0x0AD4 + leg - 0x8000 into at; by its sign into neg or pos. */
static void km_diff(uint32_t leg, uint32_t at, uint32_t pos, uint32_t neg)
{
    int32_t d = ages_ld16s(KM(0xAD4)) + ages_ld16s(KM(leg)) - 0x8000;
    ages_st16(at, (uint32_t)d);
    ages_st16(dsp_bbc((uint32_t)d, 15) ? pos : neg, (uint32_t)d);
}

/* v scaled by the COP's factor for angle a (0 if negative, as int). */
static uint32_t km_scale(uint32_t op, uint32_t a, uint32_t v, bool flip)
{
    uint32_t f = dsp_cop1(op, a);
    if (flip) f ^= 0x80000000u;
    if (dsp_bbs(f, 31)) return 0;
    return dsp_cvtsw(dsp_mulr(f, dsp_cop1(0x0B801717u, v)));
}

/* max(a, b) less the rest angle, not below 0. */
static int32_t km_open(int32_t a, int32_t b, uint32_t rest)
{
    if (dsp_cmpi((uint32_t)a, (uint32_t)b) != 1) a = b;
    a -= ages_ld16s(rest);
    if (dsp_cmpi(0, (uint32_t)a) != 4) a = 0;
    return a;
}

/* The matrix into dst, the model (entry word g9off) into slot, drawn. */
static void km_panel(uint32_t dst, uint32_t g9off, uint32_t slot)
{
    uint32_t m[12];
    ages_cop_w(0x02800505u);
    gems_cop_rn(m, 12);
    ages_stn(KM(dst), m, 12);
    dsp_cmd(0x23004646u, 3);
    uint32_t model = ages_ld16(AGES_G(9) + g9off);
    ages_st16(KM(slot), model);
    AGES_G(1) = ages_ld8(KM(4));
    dsp_draw(model);
    dsp_pop();
}

/* Push, the base matrix and the panel's place. */
static void km_frame(bool op13, uint32_t tr)
{
    dsp_push();
    if (op13) km_op13();
    ages_cop_w(0x01800303u);
    dsp_trans(tr);
}

static void km_front(void)
{
    km_frame(true, 0x50A324);
    dsp_cmd(DSP_ZROT, (uint32_t)ages_ld16s(0x50A358));
    dsp_cmd(DSP_YROT, (uint32_t)ages_ld16s(0x50A356));
    uint32_t l = (uint32_t)ages_ld16s(0x50A380), r = (uint32_t)ages_ld16s(0x50A386);
    uint32_t a = (uint32_t)(ages_ld16s(KM(0xAD8)) + 0x8000);
    l = km_scale(0x11002222u, a, l, false);
    ages_st16(0x50A3B8, km_scale(0x10802121u, a, (uint32_t)ages_ld16s(0x50A380), false));
    a = (uint32_t)(ages_ld16s(KM(0xADE)) + 0x8000);
    r = km_scale(0x11002222u, a, r, false);
    ages_st16(0x50A3BC, km_scale(0x10802121u, a, (uint32_t)ages_ld16s(0x50A386), true));
    int32_t x = km_open((int32_t)l, (int32_t)r, 0x50A36C) + ages_ld16s(0x50A354);
    ages_st16(0x50A36E, (uint32_t)x);
    dsp_cmd(DSP_XROT, (uint32_t)x);
    km_panel(0x1A00, 4, 0x1B20);
}

/* The back: the legs' angles without the half turn, except the last
 * scale, which reuses the left leg's (the i960 keeps it in r14). */
static void km_back(void)
{
    km_frame(true, 0x50A330);
    dsp_cmd(DSP_ZROT, (uint32_t)ages_ld16s(0x50A35E));
    dsp_cmd(DSP_YROT, (uint32_t)ages_ld16s(0x50A35C));
    uint32_t l = (uint32_t)-ages_ld16s(0x50A382), r = (uint32_t)-ages_ld16s(0x50A388);
    uint32_t a = (uint32_t)ages_ld16s(KM(0xAD8));
    l = km_scale(0x11002222u, a, l, false);
    uint32_t half = a + 0x8000u;
    ages_st16(0x50A3BA, km_scale(0x10802121u, half, (uint32_t)-ages_ld16s(0x50A382), false));
    a = (uint32_t)ages_ld16s(KM(0xADE));
    r = km_scale(0x11002222u, a, r, false);
    ages_st16(0x50A3BE, km_scale(0x10802121u, half, (uint32_t)-ages_ld16s(0x50A388), false));
    int32_t x = ages_ld16s(0x50A35A) - km_open((int32_t)l, (int32_t)r, 0x50A370);
    ages_st16(0x50A372, (uint32_t)x);
    dsp_cmd(DSP_XROT, (uint32_t)x);
    km_panel(0x1A30, 6, 0x1B22);
}

/* A side panel: dropped by the entry's length (negated on the right),
 * turned about y by the larger of its two scaled pushes. */
typedef struct { uint32_t tr, z, y, p0, p1, rest, out, x, mat, g9off, slot; } km_side_t;

static void km_sidepanel(const km_side_t *s, bool right)
{
    km_frame(right, s->tr);
    ages_cop_w(DSP_TRANS);
    ages_cop_w(0);
    ages_cop_w(0);
    ages_cop_w(ages_ld32(AGES_G(9) + 8) ^ (right ? 0x80000000u : 0));
    dsp_cmd(DSP_ZROT, (uint32_t)ages_ld16s(s->z));
    int32_t y = ages_ld16s(s->y);
    int32_t o = km_open(ages_ld16s(s->p1), ages_ld16s(s->p0), s->rest);
    y = right ? y - o : y + o;
    dsp_cmd(DSP_YROT, (uint32_t)y);
    ages_st16(s->out, (uint32_t)y);
    dsp_cmd(DSP_XROT, (uint32_t)ages_ld16s(s->x));
    km_panel(s->mat, s->g9off, s->slot);
}

static const km_side_t km_left = {
    0x50A33C, 0x50A364, 0x50A362, 0x50A3B8, 0x50A3BA, 0x50A374, 0x50A376, 0x50A360,
    0x1A60, 2, 0x1B24 };
static const km_side_t km_right = {
    0x50A348, 0x50A36A, 0x50A368, 0x50A3BC, 0x50A3BE, 0x50A378, 0x50A37A, 0x50A366,
    0x1A90, 0, 0x1B26 };

static uint32_t afn_kosi_mino_disp(int entry)
{
    dsp_cmd(0x21804343u, 3);
    uint32_t ch = ages_ld8(KM(0x1B0));
    AGES_G(9) = 0x6421DE0u + ch * 16u;
    bool has = dsp_cmpo(1, ch) == 2 || dsp_cmpo(14, ch) == 2
            || dsp_cmpo(0, ch) == 2 || dsp_cmpo(13, ch) == 2;
    if (!has || dsp_bbs(ages_ld16(KM(0x1F48)), 9)) {
        for (uint32_t o = 0x1B20; o <= 0x1B26; o += 2) ages_st16(KM(o), 0);
        if (entry) ages_i960_ret();
        return 0;
    }
    ages_st16(0x50A380, 0);
    ages_st16(0x50A382, 0);
    ages_st16(0x50A386, 0);
    ages_st16(0x50A388, 0);
    km_side(0x3A9C, 0x128, 0x50A38C, 0x50A3A0, 0xAD8);
    km_side(0x3A78, 0x104, 0x50A3A4, 0x50A3B4, 0xADE);
    km_diff(0xADA, 0x50A37C, 0x50A380, 0x50A382);
    km_diff(0xAE0, 0x50A37E, 0x50A386, 0x50A388);
    km_front();
    km_back();
    km_sidepanel(&km_left, false);
    km_sidepanel(&km_right, true);
    if (entry) ages_i960_ret();
    return 0;
}
