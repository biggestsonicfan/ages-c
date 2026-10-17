/* option_control: i960 0x1B240, PS2 0x1D0E00 (called by rob_disp).
 *
 * The held item (option) of a fighter, when rob_disp reaches the unit g0 it
 * hangs from: with an item dropped (flag 27) wait for its unit at 0x7A0 and,
 * on flag 4, delete it (0x1CA68: clear flag 27 and 0xAFC, reload the
 * collision tables); then, unless 0x79E bit 15, call the item's handler
 * from the table at 0x1B288. Every entry there is 0x1B2CC, the swinging
 * item (opt_swing below), which leaves g9 at the item's data.
 * The PS2 has no handler call: it only follows the item's unit. */
#pragma once
#include "set_ashi_ptn.h"
#include "send_init_ram_coli_data.h"

#define OPT(o) (AGES_G(7) + (o))

/* 0x1B2CC: g9 = the item's data, as copy_option_data finds it. */
static void opt_data(void)
{
    uint32_t t = ages_ld32(0x640D2B0u + ages_ld8(OPT(0x1B0)) * 4u);
    AGES_G(9) = ages_ld32(t + 0x2C);
    if (dsp_cmpo(0, ages_ld8(ages_ld32(0x50016C) + 0x3350)) != 2)
        AGES_G(9) = ages_ld32(t + 0x38);
    uint32_t n = ages_ld32(OPT(0xAFC));
    if (dsp_cmpo(0, n) != 2) AGES_G(9) += (n - 1u) * 0x70u;
    if (!dsp_bbc(ages_ld32(OPT(0)), 6)) AGES_G(9) += 56;
}

/* COP int -> float, scale, float -> int. */
static int32_t opt_scale(uint32_t v, uint32_t k)
{
    return (int32_t)dsp_cvtsw(dsp_mulr(k, dsp_cop1(0x0B801717u, v)));
}

/* 0x1B328: the swing (0x7BC, speed 0x7BE) pushed by how fast the fighter
 * moves against 0x1E0C, kept within +-0x1000. */
static void opt_swing(void)
{
    uint32_t d = dsp_get_3d_r(OPT(0x1F4), OPT(0x1E0C));
    uint32_t v = dsp_subr(d, ages_ld32(OPT(0x7C0)));
    ages_st32(OPT(0x7C0), d);
    ages_st32(OPT(0x7C4), v);
    if (dsp_cmpr(0x3DA3D70Au, v & 0x7FFFFFFFu) != 1)
        ages_st32(OPT(0x7C4), dsp_mulr(0x3DCCCCCDu, ages_ld32(OPT(0x7C4))));
    int32_t push = (int32_t)dsp_cvtsw(dsp_mulr(0x47435000u, ages_ld32(OPT(0x7C4))));
    int32_t cur = ages_ld16s(OPT(0x7BE));
    if (!dsp_bbc((uint32_t)cur, 31)) cur = -cur;
    int32_t mag = push;
    if (!dsp_bbc((uint32_t)mag, 31)) mag = -mag;
    ages_st16(OPT(0x7C8), (uint32_t)mag);
    if (dsp_cmpi((uint32_t)cur, (uint32_t)mag) != 1) ages_st16(OPT(0x7BE), (uint32_t)push);
    int32_t base = ages_ld16s(AGES_G(9) + 0x14);
    int32_t pos = ages_ld16s(OPT(0x7BC));
    int32_t sp = opt_scale((uint32_t)ages_ld16s(OPT(0x7BE)), 0x3F7D70A4u);
    pos += sp;
    if (dsp_cmpi(0x1000, (uint32_t)pos) == 4) {
        pos = 0x1000;
        sp = opt_scale((uint32_t)sp, 0xBF333333u);
    } else if (dsp_cmpi((uint32_t)-0x1000, (uint32_t)pos) == 1) {
        pos = -0x1000;
        sp = opt_scale((uint32_t)sp, 0xBF333333u);
    }
    ages_st16(OPT(0x7BE), (uint32_t)sp);
    ages_st16(OPT(0x7BC), (uint32_t)pos);
    ages_st16(OPT(0x798), (uint32_t)(pos + base));
}

/* 0x1B484: up (0x7CA counts to 5) while the fighter is high enough, else
 * down (0x7CB from 5 to 0); returns the count. */
static uint32_t opt_count(void)
{
    uint32_t y = ages_ld32(OPT(0x1F8)), floor = ages_ld32(OPT(0xC80));
    uint32_t h = ages_ld32(0x50A010);
    if (dsp_cmpo(30, ages_ld8(OPT(0xA29))) == 1) h = 0;
    h = dsp_addr(h, 0x3E99999Au);
    uint32_t n;
    if (dsp_cmpr(h, y) != 4) {
        if (!(dsp_cmpr(h, floor) & 3)) ages_st8(OPT(0x7CA), 0);   /* not bge */
        n = ages_ld8(OPT(0x7CA));
        if (dsp_cmpo(5, n) != 2) n++;
        ages_st8(OPT(0x7CA), n);
        uint32_t s = ages_ld32(AGES_G(9));
        ages_st16(OPT(0x798), (uint32_t)ages_ld16s(s + 0x30));
        ages_st16(OPT(0x79A), (uint32_t)ages_ld16s(s + 0x32));
        ages_st16(OPT(0x79C), (uint32_t)ages_ld16s(s + 0x34));
        return n;
    }
    if (dsp_cmpr(h, floor) != 4) {
        ages_st8(OPT(0x7CA), 0);
        ages_st8(OPT(0x7CB), 5);
    }
    n = ages_ld8(OPT(0x7CB));
    if (dsp_cmpo(0, n) != 2) n--;
    ages_st8(OPT(0x7CB), n);
    return n;
}

/* 0x1B51C: the item's place, n/5 of the way from 8(g9) to its pose's. */
static void opt_place(uint32_t n)
{
    uint32_t a[3], b[3];
    ages_ldn(a, ages_ld32(AGES_G(9)) + 0x24, 3);
    ages_ldn(b, AGES_G(9) + 8, 3);
    uint32_t t = dsp_divr(0x40A00000u, dsp_cop1(0x0B801717u, n));
    for (int i = 0; i < 3; i++) b[i] = dsp_subr(dsp_mulr(dsp_subr(a[i], b[i]), t), b[i]);
    ages_stn(OPT(0x78C), b, 3);
}

static void opt_handler(void)
{
    opt_data();
    if (!dsp_bbs(ages_ld32(0x508000), 5)) opt_swing();
    if (dsp_bbs(ages_ld32(OPT(0)), 27)) return;
    opt_place(opt_count());
}

static uint32_t afn_option_control(int entry)
{
    uint32_t o = OPT(0x788);
    if (!dsp_bbc(ages_ld32(OPT(0)), 27)) {
        o = OPT(0x7A0);
        if (dsp_cmpo(AGES_G(0), ages_ld16(o)) != 2) goto out;
        if (!dsp_bbc(ages_ld32(OPT(0)), 4)) {
            ages_st32(OPT(0), ages_ld32(OPT(0)) & ~(1u << 27));
            ages_st32(OPT(0xAFC), 0);
            afn_send_init_ram_coli_data(0);
            o = OPT(0x788);
        }
    }
    if (dsp_cmpo(AGES_G(0), ages_ld16(o)) == 2) {
        uint32_t k = ages_ld16(o + 0x16);
        if (!dsp_bbs(k, 15)) {
            uint32_t fn = ages_ld32(0x1B288u + k * 4u);
            if (fn == 0x1B2CCu) opt_handler();
        }
    }
out:
    if (entry) ages_i960_ret();
    return 0;
}
