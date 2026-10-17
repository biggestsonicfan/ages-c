/* pky_kami_disp: i960 0x1ADDC, PS2 0x1D1240 (called by rob_disp, unit 2).
 *
 * Draws the hair pieces of characters 5 and 18: three pairs of models
 * (0xAA2, 0xAA0, 0xAA4) under the head's frame at 0xA70, each pair at the
 * swing angle 0xA98 and at half of it; then moves the swing on from the
 * head's fall (0x210 against last frame's 0xA7C) and clamps it to
 * 0..0x3000. As the PS2.
 *
 * dsp_draw is the model chooser rob_disp and its helpers repeat inline. */
#pragma once
#include "set_ashi_ptn.h"
#include "set_obj_thd.h"

#define PKY(o) (AGES_G(7) + (o))

/* Draw model g0 (g1 as the caller left it): set_obj_thd with a replaced
 * texture header for the flash (0x1A4 bit 26) and blink (0x720 bit 2, odd
 * frames) of the first five frames, else set_obj, or set_obj_thd with the
 * caller's g2 when plain_thd (rob_disp's models 0x62E..0x64C). */
static void dsp_draw_or(uint32_t model, bool plain_thd)
{
    AGES_G(0) = model;
    if (!dsp_bbc(ages_ld32(PKY(0x1A4)), 26)
        && dsp_cmpo(4, ages_ld16(PKY(0x1AA))) != 4) {
        AGES_G(2) = 1u << 23;
        afn_set_obj_thd(0);
        return;
    }
    if (!dsp_bbc(ages_ld32(PKY(0x720)), 2)) {
        uint32_t f = ages_ld16(PKY(0x1AA));
        if (dsp_cmpo(4, f) != 4 && !dsp_bbc(f, 0)) {
            AGES_G(2) = 0x802000u;
            afn_set_obj_thd(0);
            return;
        }
    }
    if (plain_thd) afn_set_obj_thd(0);
    else afn_set_obj(0);
}

static void dsp_draw(uint32_t model)
{
    dsp_draw_or(model, false);
}

/* push, y rot (if y_rot), z rot, draw, pop. */
static void pky_piece(bool yrot, uint32_t y, uint32_t z, uint32_t model)
{
    dsp_push();
    if (yrot) dsp_cmd(DSP_YROT, y);
    dsp_cmd(DSP_ZROT, z);
    dsp_draw(ages_ld16(PKY(model)));
    dsp_pop();
}

/* 0x1B18C: the swing follows the head's drop. */
static void pky_swing(void)
{
    uint32_t d = dsp_subr(ages_ld32(PKY(0x210)), ages_ld32(PKY(0xA7C)));
    ages_st32(PKY(0xA80), d);
    uint32_t prev = ages_ld32(PKY(0xA84));
    int32_t ang = ages_ld16s(PKY(0xA98));
    int32_t v = ages_ld16s(PKY(0xA9A));
    int32_t k = ages_ld16s(PKY(0xA9C));
    d = dsp_subr(d, prev);
    if (!dsp_bbs(prev, 31)) d ^= 0x80000000u;
    d = dsp_mulr(dsp_mulr(ages_ld32(PKY(0xA88)), 0x447A0000u), d);
    v += (int32_t)dsp_cvtsw(d);
    uint32_t fl = ages_ld32(PKY(0x1A4));
    if (!dsp_bbs(fl, 15) && !dsp_bbc(fl, 16)) k *= 3;
    v -= k;
    int32_t lim = ages_ld16s(PKY(0xA9E));
    if (dsp_cmpi((uint32_t)lim, (uint32_t)v) != 1) v = lim;
    ang += v;
    if (dsp_cmpi(0x3000, (uint32_t)ang) != 1) {
        ang = 0x3000;
        v = 0;
    }
    if (dsp_cmpi(0, (uint32_t)ang) != 4) {
        ang = 0;
        v = 0;
    }
    ages_st16(PKY(0xA98), (uint32_t)ang);
    ages_st16(PKY(0xA9A), (uint32_t)v);
    ages_st32(PKY(0xA7C), ages_ld32(PKY(0x210)));
    ages_st32(PKY(0xA84), ages_ld32(PKY(0xA80)));
}

static uint32_t afn_pky_kami_disp(int entry)
{
    dsp_push();
    dsp_trans(PKY(0xA70));
    dsp_push();
    dsp_cmd(DSP_YROT, ages_ld16s(PKY(0xA92)));
    dsp_cmd(DSP_XROT, ages_ld16s(PKY(0xA90)));
    int32_t a = ages_ld16s(PKY(0xA98));
    pky_piece(false, 0, (uint32_t)-a, 0xAA2);
    a = ages_ld16s(PKY(0xA98));
    pky_piece(false, 0, (uint32_t)-(a >> 1), 0xAA2);
    dsp_pop();
    uint32_t y = ages_ld16s(PKY(0xA8E));
    pky_piece(true, y, ages_ld16s(PKY(0xA98)), 0xAA0);
    y = ages_ld16s(PKY(0xA8E));
    pky_piece(true, y, (uint32_t)(ages_ld16s(PKY(0xA98)) >> 1), 0xAA0);
    a = ages_ld16s(PKY(0xA98));
    pky_piece(true, (uint32_t)(a + ages_ld16s(PKY(0xA96))), ages_ld16s(PKY(0xA94)), 0xAA4);
    a = ages_ld16s(PKY(0xA98)) >> 1;
    pky_piece(true, (uint32_t)(a + ages_ld16s(PKY(0xA96))), ages_ld16s(PKY(0xA94)), 0xAA4);
    dsp_pop();
    pky_swing();
    if (entry) ages_i960_ret();
    return 0;
}
