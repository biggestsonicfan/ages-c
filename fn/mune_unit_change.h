/* mune_unit_change: i960 0x31888 (called by rob_disp for unit 1).
 *
 * The chest model g0 for characters with a table at 0x6421F80[0x1B0] and
 * bit 1 of 0x1F48: a swing at 0xC68/0xC6C(g7) driven by how far the point
 * 0x20C moved since the last frame (COP get_3d_r against 0xC7C) picks one of
 * 31 models; a still pose (0xC90) follows the motion's own pattern instead.
 *
 * The i960 has a slip at 0x31A04: past +15 the swing is set to 0.78 (the
 * damping constant just loaded) where past -15 it is set to -15. Kept. */
#pragma once
#include "set_ashi_ptn.h"

#define MUNE(o) (AGES_G(7) + (o))

/* (frame count, patterns...) at *tab: g0 for frame 0x1AA. remo by 0 leaves
 * the index at 0. */
static void mune_pattern(uint32_t t)
{
    uint32_t n = ages_ld32(t);
    uint32_t f = ages_ld16(MUNE(0x1AA));
    uint32_t i = n ? f % n : 0;
    AGES_G(0) = ages_ld16(t + 4u + i * 2u);
}

/* 0x318B4: true when the fighter holds still on its pattern's model. */
static bool mune_still(uint32_t tab)
{
    uint32_t fl = ages_ld32(MUNE(0x1A4));
    if (dsp_bbc(fl, 9) || dsp_bbs(fl, 14) || dsp_bbs(fl, 16)) return false;
    if (dsp_cmpo(0, ages_ld32(MUNE(0xC90))) != 2) return true;
    uint32_t t = ages_ld32(tab);
    if (dsp_cmpo(0, t) == 2) return false;
    uint32_t n = ages_ld32(t);
    uint32_t i = n ? ages_ld16(MUNE(0x1AA)) % n : 0;
    if (dsp_cmpo(ages_ld16(t + 4u + i * 2u), ages_ld16(MUNE(0xC94))) != 2) return false;
    uint32_t sw = ages_ld32(MUNE(0xC68)) & 0x7FFFFFFFu;
    if (dsp_cmpr(sw, 0x3F4CCCCDu) == 1) return false;
    ages_st32(MUNE(0xC90), 1);
    return true;
}

/* 0x3191C..0x319A8: the push from the move, into 0xC88 and 0xC68. */
static void mune_push(void)
{
    ages_st32(MUNE(0xC90), 0);
    uint32_t prev = ages_ld32(MUNE(0xC64));
    uint32_t d = dsp_get_3d_r(MUNE(0x20C), MUNE(0xC7C));
    ages_st32(MUNE(0xC64), d);
    d = dsp_subr(d, prev) & 0x7FFFFFFFu;
    d = dsp_mulr(ages_ld32(MUNE(0xC74)), d);
    if (dsp_cmpr(0x40866666u, d) != 1) d = 0x40866666u;
    ages_st32(MUNE(0xC88), d);
    uint32_t sw = ages_ld32(MUNE(0xC68));
    uint32_t mag = sw & 0x7FFFFFFFu;
    if (dsp_cmpo(0, mag) == 2) {
        ages_st32(MUNE(0xC68), d);
    } else if (dsp_cmpr(d, mag) != 4) {
        ages_st32(MUNE(0xC68), dsp_mulr(dsp_divr(sw, mag), d));
    }
}

/* 0x319AC..0x31A28: swing on, damped and bounced at +-15. */
static void mune_swing(void)
{
    uint32_t v = ages_ld32(MUNE(0xC68));
    uint32_t x = dsp_addr(v, ages_ld32(MUNE(0xC6C)));
    v = dsp_mulr(0x3F6F5C29u, v);
    if (!dsp_bbc(x, 31)) {
        if (dsp_cmpr(0xC1700000u, x) != 4) {
            x = 0xC1700000u;
            v = dsp_mulr(0x3F47AE14u, v ^ 0x80000000u);
        }
    } else if (dsp_cmpr(0x41700000u, x) != 1) {
        v = dsp_mulr(0x3F47AE14u, v ^ 0x80000000u);
        x = 0x3F47AE14u;
    }
    ages_st32(MUNE(0xC6C), x);
    uint32_t pull = dsp_bbs(x, 31) ? ages_ld32(MUNE(0xC70)) : ages_ld32(MUNE(0xC78));
    ages_st32(MUNE(0xC68), dsp_addr(pull, v));
}

/* 0x31A2C: g0 from the swing's model table, rounded to the nearest of 31. */
static void mune_model(uint32_t tab)
{
    uint32_t t = ages_ld32(tab + 4);
    if (dsp_cmpo(0, t) == 2) return;
    uint32_t x = dsp_addr(0x3F000000u, ages_ld32(MUNE(0xC6C)));
    uint32_t i = dsp_cvtsw(x) + 15u;
    uint32_t a = ages_ld8(0x50002B), b = ages_ld8(0x500031);
    if (dsp_cmpo(9, a) == 2 && dsp_cmpo(0x11, b) == 2
        && !dsp_bbc(ages_ld32(MUNE(0x1A4)), 16)) {
        ages_st32(MUNE(0xC68), 0);
        ages_st32(MUNE(0xC6C), 0);
        return;
    }
    AGES_G(0) = ages_ld16(t + i * 2u);
    ages_st16(MUNE(0xC94), AGES_G(0));
}

static uint32_t afn_mune_unit_change(int entry)
{
    uint32_t tab = ages_ld32(0x6421F80u + ages_ld8(MUNE(0x1B0)) * 4u);
    if (dsp_cmpo(0, tab) != 2 && !dsp_bbc(ages_ld16(MUNE(0x1F48)), 1)) {
        if (dsp_bbs(ages_ld32(0x508000), 5)) {
            mune_model(tab);
        } else if (mune_still(tab)) {
            /* 0x31AB0: only with flags exactly 0x200 */
            if (dsp_cmpo(0x200, ages_ld32(MUNE(0x1A4))) == 2) {
                uint32_t t = ages_ld32(tab);
                if (dsp_cmpo(0, t) != 2) mune_pattern(t);
            }
        } else {
            mune_push();
            mune_swing();
            mune_model(tab);
        }
    }
    if (entry) ages_i960_ret();
    return 0;
}
