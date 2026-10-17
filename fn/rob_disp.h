/* rob_disp: i960 0x19F10, PS2 0x1CE900, EXT 65.
 *
 * Draws fighter g7: each of its 16 units under the unit matrix the COP
 * holds (with the foot, hand, chest and hip models swapped by set_ashi_ptn,
 * set_te_ptn, mune_unit_change and koshi_unit_change), the extras hung on
 * units 1, 2 and 9 (shoulder pads, hair, skirt), the held item and its
 * four trailing copies, and the shadow ball (flag 24). Flag 25 draws the
 * fighter as balls instead; 0x2A draws a stand-in model.
 *
 * The board, not the PS2: option_control runs the item's i960 handler;
 * the PS2-only calls (FUN_001cad40, FUN_001bbb80, FUN_001bcb00,
 * FUN_001c63c0, sysReadWorkRam1) have no counterpart here. */
#pragma once
#include "kata_pat_disp.h"
#include "set_te_ptn.h"
#include "mune_unit_change.h"
#include "koshi_unit_change.h"
#include "set_mmode.h"
#include "copy_option_data.h"
#include "option_control.h"

#define RD(o) (AGES_G(7) + (o))
#define RD_PAUSED() dsp_bbs(ages_ld32(0x508000), 5)

/* COP ld_unit_mat / mul_unit_mat with the fighter's unit u. */
static void rd_unit_mat(uint32_t op, uint32_t u)
{
    ages_cop_w(op);
    ages_cop_w(ages_ld8(RD(4)));
    ages_cop_w(u * 12u);
}

/* COP 0x0B: load the 12-word matrix at m. */
static void rd_load_mat(uint32_t m)
{
    uint32_t v[12];
    ages_ldn(v, m, 12);
    ages_cop_w(0x05800B0Bu);
    gems_cop_wn(v, 12);
}

static void rd_get_mat(uint32_t m)
{
    uint32_t v[12];
    ages_cop_w(0x02800505u);
    gems_cop_rn(v, 12);
    ages_stn(m, v, 12);
}

/* Call f with every g register kept, as the i960 does around the
 * skirt and the shoulder pads. */
static void rd_saved(uint32_t (*f)(int))
{
    uint32_t g[15];
    for (int i = 0; i < 15; i++) g[i] = AGES_G(i);
    f(0);
    for (int i = 0; i < 15; i++) AGES_G(i) = g[i];
}

/* 0x19F2C: the frame counter 0xAEC and the 0x7FA count of flag 4. */
static void rd_counters(void)
{
    if (!RD_PAUSED()) ages_st32(RD(0xAEC), ages_ld32(RD(0xAEC)) + 1u);
    if (dsp_bbc(ages_ld32(RD(0x1A4)), 4)) {
        ages_st16(RD(0x7FA), 0);
    } else if (!dsp_bbs(ages_ld32(RD(0)), 3) && !RD_PAUSED()) {
        ages_st16(RD(0x7FA), ages_ld16(RD(0x7FA)) + 1u);
    }
}

/* 0x1A12C: the fighter's GEO display-list chain (0x501800, 0x501804). */
static void rd_geo_list(void)
{
    uint32_t dl = ages_ld32(AGES_G(10) + 0x2008);
    obj_geo_cmd(0, 0);
    ages_st32(0x501800, dl);
    uint32_t e = 0x501804u + ages_ld8(RD(4)) * 8u;
    uint32_t last = ages_ld32(e + 4);
    dl = ages_ld32(AGES_G(10) + 0x2008);
    if (dsp_cmpo(0, last) == 2) ages_st32(e, dl);
    else ages_st32(0x900000u + last, dl | 0x80000000u);
    ages_st32(AGES_G(10) + 0x1008, ages_ld32(0x501800));
    e = 0x501804u + ages_ld8(RD(4)) * 8u;
    uint32_t first = ages_ld32(e);
    ages_st32(e + 4, ages_ld32(AGES_G(10) + 0x2008));
    ages_st32(AGES_G(10), first | 0x80000000u);
}

static void rd_unit2(void)
{
    rd_geo_list();
    uint32_t ch = ages_ld8(RD(0x1B0));
    if (dsp_cmpo(5, ch) != 2 && dsp_cmpo(18, ch) != 2) return;
    if (dsp_bbc(ages_ld16(RD(0x1F48)), 2)) return;
    afn_pky_kami_disp(0);
}

/* 0x1A2D8: the hip's under-model straight into the GEO list between
 * mode words 3 and 1, then the skirt. */
static void rd_unit9(void)
{
    uint32_t t = ages_ld32(ages_ld32(RD(0x190)) + 0x24);
    if (!dsp_bbc(ages_ld16(RD(0x1F48)), 1))
        t = ages_ld32(0x642F9ECu + ages_ld8(RD(0x1B0)) * 4u);
    if (dsp_cmpo(0, t) != 2) {
        uint32_t e[4];
        AGES_G(0) = 3;
        afn_set_mmode(0);
        AGES_G(0) = OBJ_MODELS + ages_ld32(t + 8) * 16u;
        ages_ldn(e, AGES_G(0), 4);
        uint32_t tex = e[3] >> 16, n = e[3] & 0xFFFFu;
        uint32_t side = ages_ld8(RD(4));
        if (dsp_cmpo(0, side) != 2) {
            e[1] += tex * 4u;
            ages_st32(0x5010D0, ages_ld32(0x5010D0) + n);
        }
        e[2] = (side << 8) + 4u;
        ages_st32(0x50101C, ages_ld32(0x50101C) + n);
        e[3] = 0xFFFFFFFFu;
        uint32_t f;
        if (!dsp_bbc(ages_ld32(RD(0x1A4)), 26) && dsp_cmpo(4, ages_ld16(RD(0x1AA))) != 4)
            e[1] = 1u << 23;
        else if (!dsp_bbc(ages_ld32(RD(0x720)), 2)
                 && dsp_cmpo(4, f = ages_ld16(RD(0x1AA))) != 4 && !dsp_bbc(f, 0))
            e[1] = 0x802000u;
        obj_geo_cmd(0x10, 0x101);
        ages_stn(AGES_G(10) + AGES_G(12), e, 4);
        AGES_G(0) = 1;
        afn_set_mmode(0);
    }
    rd_saved(afn_kosi_mino_disp);
}

/* 0x19FD8: the unit's model, swapped by the pattern helpers. */
static uint32_t rd_model(uint32_t u)
{
    uint32_t m = ages_ld32(RD(0x40) + u * 4u);
    if (dsp_cmpo(0, ages_ld32(RD(0x5F0))) != 2) {
        AGES_G(0) = u;
        AGES_G(1) = m;
        afn_set_ashi_ptn(0);
    }
    if ((dsp_cmpi(5, u) == 2 || dsp_cmpi(8, u) == 2)
        && dsp_cmpo(0, ages_ld32(RD(0x6AC))) != 2) {
        AGES_G(0) = u;
        afn_set_te_ptn(0);
        m = AGES_G(1);
    }
    if (dsp_cmpi(1, u) == 2) {
        AGES_G(0) = m;
        afn_mune_unit_change(0);
        m = AGES_G(0);
    }
    if (dsp_cmpi(9, u) == 2) {
        AGES_G(0) = m;
        afn_koshi_unit_change(0);
        m = AGES_G(0);
    }
    return m;
}

/* 0x1A69C: the item's four trailing copies, where 0x30 of each is set. */
static void rd_item_copies(void)
{
    uint32_t c = RD(0xF34);
    for (uint32_t n = 4;; n--) {
        if (dsp_cmpo(0, ages_ld16(c + 0x30)) != 2) {
            dsp_push();
            rd_load_mat(c);
            AGES_G(1) = ages_ld8(RD(4));
            dsp_draw(ages_ld16(RD(0x7F8)));
            dsp_pop();
        }
        c += 52;
        if (dsp_cmpo(1, n) != 4) break;
    }
}

/* 0x1A4E0: the held item on its unit (record o), its matrix kept in the
 * ring at 0xF34 (0xF00 the slot) for the copies. */
static void rd_item(uint32_t u, uint32_t o)
{
    if (!RD_PAUSED()) {
        uint32_t s = ages_ld32(RD(0xF00)) + 1u;
        if (dsp_cmpo(4, s) != 1) s = 0;
        ages_st32(RD(0xF00), s);
    }
    uint32_t m = RD(0xF34) + ages_ld32(RD(0xF00)) * 52u;
    uint32_t on = 0;
    if (!dsp_bbc(ages_ld32(RD(0)), 27) && !dsp_bbs(ages_ld32(RD(0x1A4)), 2)) on = 1;
    ages_st16(m + 0x30, on);
    dsp_push();
    rd_unit_mat(0x1B003636u, u);
    dsp_trans(o + 4);
    dsp_cmd(DSP_YROT, (uint32_t)ages_ld16s(o + 0x12));
    dsp_cmd(DSP_XROT, (uint32_t)ages_ld16s(o + 0x10));
    dsp_cmd(DSP_ZROT, (uint32_t)ages_ld16s(o + 0x14));
    rd_get_mat(m);
    dsp_pop();
    dsp_pop();
    dsp_push();
    rd_load_mat(m);
    AGES_G(1) = ages_ld8(RD(4));
    dsp_draw(ages_ld16(o + 2));
    dsp_pop();
    dsp_push();
    rd_item_copies();
}

static void rd_unit(uint32_t u)
{
    dsp_push();
    rd_unit_mat(0x1B803737u, u);
    uint32_t m = rd_model(u);
    bool alt = dsp_cmpo(m, 0x62E) != 4 && dsp_cmpo(m, 0x64C) != 1;
    AGES_G(1) = ages_ld8(RD(4));
    if (alt) AGES_G(2) = 0x68CBC;
    dsp_draw_or(m, alt);
    if (u == 1) rd_saved(afn_kata_pat_disp);
    else if (u == 2) rd_unit2();
    else if (u == 9) rd_unit9();
    afn_copy_option_data(0);
    if (dsp_cmpo(0, ages_ld16(RD(0x78A))) != 2) {
        AGES_G(0) = u;
        afn_option_control(0);
        uint32_t o = dsp_bbc(ages_ld32(RD(0)), 27) ? RD(0x788) : RD(0x7A0);
        if (dsp_cmpo(u, ages_ld16(o)) == 2) rd_item(u, o);
    }
    dsp_pop();
}

/* 0x1A760: the shadow ball under the fighter. */
static void rd_shadow(void)
{
    dsp_push();
    ages_cop_w(DSP_TRANS);
    ages_cop_w(ages_ld32(RD(0x1F4)));
    ages_cop_w(0x3DCCCCCDu);
    ages_cop_w(ages_ld32(RD(0x1FC)));
    dsp_cmd(DSP_YROT, (uint32_t)ages_ld16s(RD(0x616)));
    ages_cop_w(0x03800707u);
    ages_cop_w(0x3F800000u);
    ages_cop_w(0x3F800000u);
    ages_cop_w(0x40400000u);
    dsp_cmd(DSP_XROT, 1u << 14);
    AGES_G(1) = 0;
    AGES_G(0) = 0x14;
    afn_set_obj(0);
    dsp_pop();
}

/* Units of the flag-25 ball body: is unit u lit (0x90F600 masks)? */
static bool rd_ball_lit(uint32_t u)
{
    if (dsp_cmpo(1, ages_ld8(RD(4))) != 2)
        return dsp_cmpo(0, ages_ld32(0x90F600u + u * 4u)) != 2;
    for (int32_t i = 15;; i--) {
        if (dsp_bbs(ages_ld32(0x90F600u + (uint32_t)i * 4u), (int)(u & 31))) return true;
        if (!(dsp_cmpi(1, (uint32_t)i) & 6)) return false;
    }
}

/* One unit's balls: (pos, size) from 0xD00 by the unit's index list. */
static void rd_balls(uint32_t u, uint32_t model)
{
    uint32_t k = 0x3F800000u;
    if (!dsp_bbc(ages_ld32(RD(0xAC8)), (int)(u & 31)) && dsp_cmpo(4, ages_ld8(RD(0x821))) != 2)
        k = ages_ld32(ages_ld32(RD(0x1D04)));
    AGES_G(6) = k;
    uint32_t idx = ages_ld32(ages_ld32(RD(0x1D10)) + u * 4u);
    uint32_t n = ages_ld8(ages_ld32(RD(0x1D20)) + u);
    while (dsp_cmpi(0, n--) != 2) {
        uint32_t b[4];
        ages_ldn(b, RD(0xD00) + ages_ld8(idx + n) * 4u, 4);
        b[3] = dsp_mulr(AGES_G(6), b[3]);
        dsp_push();
        ages_cop_w(DSP_TRANS);
        gems_cop_wn(b, 3);
        ages_cop_w(0x03800707u);
        ages_cop_w(b[3]);
        ages_cop_w(b[3]);
        ages_cop_w(b[3]);
        AGES_G(1) = 0;
        AGES_G(0) = model;
        afn_set_obj(0);
        dsp_pop();
    }
}

/* 0x1A810: flag 25, units 16 down to 1 as balls, coloured by 0x640E4F0. */
static void rd_ball_body(void)
{
    uint32_t w = ages_ld32(0x500828);
    for (uint32_t u = 16;; u--) {
        uint32_t model;
        if (rd_ball_lit(u)) model = ages_ld32(0x640E4F0);
        else if (dsp_cmpo(u, ages_ld32(w + 0x158)) == 2) model = ages_ld32(0x640E4F8);
        else model = ages_ld32(0x640E4F4);
        rd_balls(u, model);
        AGES_G(0) = u;
        afn_option_control(0);
        if (dsp_cmpo(1, u) != 4) break;
    }
}

/* 0x1A930: 0x2A set: a stand-in model at the stage's height, then the
 * display-list chain. */
static void rd_stand_in(void)
{
    if (dsp_bbs(ages_ld8(ages_ld32(0x500814) + 0xFA), 0)) return;
    dsp_push();
    rd_unit_mat(0x1B803737u, 0);
    uint32_t s = 0x50B100u + (ages_ld8(0x50B00C) << 5);
    uint32_t y = dsp_subr(0x3F800000u, ages_ld32(s + 8));
    ages_cop_w(DSP_TRANS);
    ages_cop_w(0);
    ages_cop_w(y);
    ages_cop_w(0);
    AGES_G(1) = 0;
    AGES_G(0) = ages_ld32(s + 0x18);
    afn_set_obj(0);
    rd_geo_list();
    dsp_pop();
}

static void rd_body(void)
{
    uint32_t hide = ages_ld8(ages_ld32(0x500814) + 0xFA);
    for (uint32_t u = 0; u < 16; u++) {
        uint32_t off = ages_ld8(RD(0x6B8) + u);
        bool skip = dsp_bbs(hide, 0) ? dsp_cmpo(0, off) == 2 : dsp_cmpo(0, off) != 2;
        if (!skip) rd_unit(u);
        dsp_cmpo(u + 1, 16);
    }
    if (!dsp_bbc(ages_ld32(RD(0)), 24)) rd_shadow();
}

static uint32_t afn_rob_disp(int entry)
{
    if (dsp_bbc(ages_ld32(RD(0)), 7)) goto out;
    if (dsp_cmpo(0, ages_ld8(RD(0x2A))) != 2) {
        rd_stand_in();
        goto out;
    }
    dsp_cmpo(0, ages_ld32(ages_ld32(RD(0x190)) + 0x24));
    rd_counters();
    if (dsp_bbs(ages_ld32(RD(0)), 25)) rd_ball_body();
    else rd_body();
out:
    if (entry) ages_i960_ret();
    return 0;
}
