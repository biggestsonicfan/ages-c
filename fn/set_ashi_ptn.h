/* set_ashi_ptn: i960 0x1AB74, PS2 0x1D2480.
 *
 * Foot pattern for rob_disp: for unit 15 (from 0x18A, 0x2AC) or 12 (from
 * 0x17C, 0x288) scale the leg's angle and stretch through the COP into an
 * index 0..31 and load g1 from the pattern table at 0x5F0(g7) (+0x80 for
 * unit 15). As the PS2, op for op; other units leave g1 alone.
 *
 * The dsp_ helpers below are the i960 pieces rob_disp and its helpers share:
 * the FPU ops as the emulator computes them, CC, and COP words. */
#pragma once

#define DSP_CC(cc) (AGES_AC = (AGES_AC & ~7u) | (uint32_t)(cc))

static uint32_t dsp_cmpo(uint32_t a, uint32_t b)
{
    uint32_t cc = a < b ? 4 : a == b ? 2 : 1;
    DSP_CC(cc);
    return cc;
}

static uint32_t dsp_cmpi(uint32_t a, uint32_t b)
{
    int32_t x = (int32_t)a, y = (int32_t)b;
    uint32_t cc = x < y ? 4 : x == y ? 2 : 1;
    DSP_CC(cc);
    return cc;
}

/* bbs/bbc: CC 010 when the branch is taken, 000 when not. */
static bool dsp_bbs(uint32_t v, int bit)
{
    bool set = (v >> bit) & 1u;
    DSP_CC(set ? 2 : 0);
    return set;
}

static bool dsp_bbc(uint32_t v, int bit)
{
    bool clr = !((v >> bit) & 1u);
    DSP_CC(clr ? 2 : 0);
    return clr;
}

/* The FPU ops on register words, src1 first as the i960 writes them. */
static uint32_t dsp_addr(uint32_t s1, uint32_t s2)
{
    double a = i960_single_to_double(s1), b = i960_single_to_double(s2);
    return i960_double_to_single(i960_nan_result(a + b, a, b));
}

static uint32_t dsp_subr(uint32_t s1, uint32_t s2)   /* s2 - s1 */
{
    double a = i960_single_to_double(s1), b = i960_single_to_double(s2);
    return i960_double_to_single(i960_nan_result(b - a, b, a));
}

static uint32_t dsp_mulr(uint32_t s1, uint32_t s2)
{
    double a = i960_single_to_double(s1), b = i960_single_to_double(s2);
    return i960_double_to_single(i960_nan_result(a * b, a, b));
}

static uint32_t dsp_divr(uint32_t s1, uint32_t s2)   /* s2 / s1; /0 is 0 */
{
    double a = i960_single_to_double(s1), b = i960_single_to_double(s2);
    if (a == 0.0) return i960_double_to_single(0.0);
    return i960_double_to_single(i960_nan_result(b / a, b, a));
}

/* cmpr: CC 100 less, 010 equal, 001 greater, 000 unordered. */
static uint32_t dsp_cmpr(uint32_t s1, uint32_t s2)
{
    double a = i960_single_to_double(s1), b = i960_single_to_double(s2);
    uint32_t cc = a < b ? 4 : a == b ? 2 : a > b ? 1 : 0;
    DSP_CC(cc);
    return cc;
}

/* COP op with arguments, then its one reply. */
static uint32_t dsp_cop1(uint32_t op, uint32_t a)
{
    ages_cop_w(op);
    ages_cop_w(a);
    return ages_cop_r();
}

static uint32_t dsp_cop2(uint32_t op, uint32_t a, uint32_t b)
{
    ages_cop_w(op);
    ages_cop_w(a);
    ages_cop_w(b);
    return ages_cop_r();
}

static uint32_t dsp_cvtsw(uint32_t f)   /* COP 0x18: float to int */
{
    return dsp_cop1(0x0C001818u, f);
}

/* COP op with arguments and no reply. */
static void dsp_cmd(uint32_t op, uint32_t a)
{
    ages_cop_w(op);
    ages_cop_w(a);
}

static void dsp_push(void) { ages_cop_w(0x00800101u); }
static void dsp_pop(void) { ages_cop_w(0x01000202u); }
#define DSP_TRANS 0x03000606u
#define DSP_XROT 0x04000808u
#define DSP_YROT 0x04800909u
#define DSP_ZROT 0x05000A0Au

/* COP trans by the three words at a. */
static void dsp_trans(uint32_t a)
{
    uint32_t v[3];
    ages_ldn(v, a, 3);
    ages_cop_w(DSP_TRANS);
    gems_cop_wn(v, 3);
}

/* COP 0x2C (get_3d_r): the distance between two points, taken in pairs. */
static uint32_t dsp_get_3d_r(uint32_t p, uint32_t q)
{
    uint32_t a[3], b[3];
    ages_ldn(a, p, 3);
    ages_ldn(b, q, 3);
    ages_cop_w(0x16002C2Cu);
    for (int i = 0; i < 3; i++) {
        ages_cop_w(a[i]);
        ages_cop_w(b[i]);
    }
    return ages_cop_r();
}

static uint32_t afn_set_ashi_ptn(int entry)
{
    uint32_t tab = ages_ld32(AGES_G(7) + 0x5F0);
    uint32_t ang, len;
    if (AGES_G(0) == 15) {
        DSP_CC(2);
        tab += 0x80;
        ang = ages_ld16s(AGES_G(7) + 0x18A);
        len = ages_ld32(AGES_G(7) + 0x2AC);
        if (ages_ld8s(AGES_G(7) + 4) == 0) {
            DSP_CC(2);
            (void)dsp_cop2(0x0A801515u, len, 0x42C80000u);
        } else {
            dsp_cmpi(0, ages_ld8s(AGES_G(7) + 4));
        }
    } else {
        if (dsp_cmpi(12, AGES_G(0)) != 2) {
            if (entry) ages_i960_ret();
            return 0;
        }
        ang = ages_ld16s(AGES_G(7) + 0x17C);
        len = ages_ld32(AGES_G(7) + 0x288);
    }
    uint32_t a = dsp_cop2(0x0A001414u, len, 0x3DCCCCCDu);
    uint32_t b = dsp_cop2(0x12002424u, (0xFFFFu - ang) & 0xFFFFu, 0x3EB33333u);
    a = dsp_cop2(0x0A001414u, b, a);
    a = dsp_cop2(0x0A801515u, a, 0x42C80000u);
    int32_t i = (int32_t)dsp_cvtsw(a);
    if (dsp_cmpi(31, (uint32_t)i) == 4) i = 31;
    if (dsp_cmpi(0, (uint32_t)i) == 1) i = 0;
    AGES_G(1) = ages_ld32(tab + (uint32_t)i * 4u);
    if (entry) ages_i960_ret();
    return 0;
}
