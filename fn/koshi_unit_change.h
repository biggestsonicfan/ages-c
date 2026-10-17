/* koshi_unit_change: i960 0x31ADC (called by rob_disp for unit 9).
 *
 * Picks the hip model g0 for the fighter's state: from the per-character
 * table at 0x6422400[0x1B0], by motion (0x1A8/0x1AA), flags (0x1A4) and the
 * counter at 0xC8C(g7). Also runs the slide timer at 0x50A42C.
 *
 * kuc_motion (0x31CE4) is not a function on the board: koshi_unit_change
 * reaches it with bal, sharing its registers, so it is a helper here; the
 * bal leaves g14 = 0x31BE0. */
#pragma once
#include "kuc_sukebo_check.h"

#define KUC_FLAGS (AGES_G(7) + 0x1A4)
#define KUC_COUNT (AGES_G(7) + 0xC8C)

/* Start the slide timer: motion 0x19D/0x19E frame 17 of character 5. */
static void kuc_start_timer(void)
{
    if (dsp_cmpo(5, ages_ld8(AGES_G(7) + 0x1B1)) != 2) return;
    if (dsp_cmpo(20, ages_ld16(0x5000A2)) == 1) return;
    if (dsp_cmpo(0x19D, ages_ld16(AGES_G(7) + 0x1A8)) != 2
        && dsp_cmpo(0x19E, ages_ld16(AGES_G(7) + 0x1A8)) != 2) return;
    if (dsp_cmpo(17, ages_ld16(AGES_G(7) + 0x1AA)) != 2) return;
    if (dsp_bbs(ages_ld32(AGES_G(7) + 0x5B8), 0)) return;
    uint32_t p[3];
    ages_ldn(p, AGES_G(7) + 0x254, 3);
    ages_stn(0x50A420, p, 3);
    ages_st32(0x50A42C, 0x3C23D70Au);
}

static void kuc_run_timer(void)
{
    uint32_t t = ages_ld32(0x50A42C);
    if (dsp_cmpo(0, t) == 2) return;
    t = dsp_addr(0x3E23D70Au, t);
    if (dsp_cmpr(0x41700000u, t) != 1) t = 0;
    ages_st32(0x50A42C, t);
}

/* 0x31CE4: the (motion, table) pairs at 0x1C(tab); g0 from the matching one. */
static void kuc_motion(uint32_t tab, uint32_t off)
{
    uint32_t p = ages_ld32(tab + 0x1C);
    if (dsp_cmpo(0, p) == 2) return;
    for (;; p += 8) {
        uint32_t mot = ages_ld32(p);
        if (dsp_cmpo(0, mot) == 2) return;
        if (dsp_cmpo(mot, ages_ld16(AGES_G(7) + 0x1A8)) == 2) break;
    }
    uint32_t t = ages_ld32(p + 4);
    uint32_t n = ages_ld32(t);
    uint32_t f = ages_ld16(AGES_G(7) + 0x1AA);
    if (dsp_cmpo(n, f) != 1) f = n - 1u;
    AGES_G(0) = ages_ld16(t + 4u + off + f * 4u);
}

/* 0x31C08: standing; the second model past the height 0x1F8 allows. */
static void kuc_stand(uint32_t tab, uint32_t off)
{
    ages_st32(KUC_COUNT, 0);
    uint32_t t = ages_ld32(tab);
    if (dsp_cmpo(0, t) == 2) return;
    AGES_G(0) = ages_ld16(t + off);
    uint32_t h = dsp_bbs(ages_ld32(KUC_FLAGS), 15) ? 0x3F19999Au : 0x3ECCCCCDu;
    if (dsp_cmpo(0, ages_ld16(AGES_G(7) + 0x624)) != 2)
        h = dsp_addr(ages_ld32(0x50A010), h);
    if (dsp_cmpr(h, ages_ld32(AGES_G(7) + 0x1F8)) & 6) return;
    AGES_G(0) = ages_ld16(t + 4u + off);
}

/* 0x31C64: motion 49, by frame up to 40. */
static void kuc_frame40(uint32_t tab)
{
    ages_st32(KUC_COUNT, 0);
    uint32_t t = ages_ld32(tab + 0xC);
    if (dsp_cmpo(0, t) == 2) return;
    uint32_t f = ages_ld16(AGES_G(7) + 0x1AA);
    if (dsp_cmpo(40, f) != 1) f = 40;
    AGES_G(0) = ages_ld16(t + f * 2u);
}

/* 0x31C8C: the counter's run of 50. */
static void kuc_counted(uint32_t tab)
{
    uint32_t t = ages_ld32(tab + 0x14);
    if (dsp_cmpo(0, t) == 2) return;
    uint32_t n = ages_ld32(KUC_COUNT) + 1u;
    if (dsp_cmpo(50, n) != 1) {
        ages_st32(KUC_COUNT, 0);
        return;
    }
    ages_st32(KUC_COUNT, n);
    AGES_G(0) = ages_ld16(t + n * 2u);
}

/* 0x31CBC: motion 0x180, by frame, once 0x5000A2 reaches 10. */
static void kuc_motion180(uint32_t tab)
{
    if (dsp_cmpo(10, ages_ld16(0x5000A2)) == 1) return;
    ages_st32(KUC_COUNT, 0);
    uint32_t t = ages_ld32(tab + 0x10);
    if (dsp_cmpo(0, t) == 2) return;
    AGES_G(0) = ages_ld16(t + ages_ld16(AGES_G(7) + 0x1AA) * 2u);
}

/* 0x31D2C: flag 9, by frame - 1 up to 0x4F. */
static void kuc_flag9(uint32_t tab, uint32_t off)
{
    uint32_t t = ages_ld32(tab + 4);
    if (dsp_cmpo(0, t) == 2) return;
    uint32_t f = ages_ld16(AGES_G(7) + 0x1AA);
    if (dsp_cmpo(0, f) != 2) f--;
    if (dsp_cmpo(0x4F, f) == 4) f = 0x4F;
    AGES_G(0) = ages_ld16(t + off + f * 4u);
}

/* 0x31D64: flag 4, by 0xAEC mod 12 (flag 14) or by 0x7FA. */
static void kuc_flag4(uint32_t tab, uint32_t off, uint32_t aec)
{
    ages_st32(KUC_COUNT, 0);
    if (dsp_cmpo(ages_ld32(AGES_G(7) + 0x1C), ages_ld32(AGES_G(7) + 0xC5C)) == 2) return;
    if (dsp_cmpo(0, ages_ld16(AGES_G(7) + 0xA0E)) != 2) return;
    uint32_t t = ages_ld32(tab + 8);
    if (dsp_cmpo(0, t) == 2) return;
    uint32_t i;
    if (!dsp_bbc(ages_ld32(KUC_FLAGS), 14)) {
        t = ages_ld32(t + 4);
        if (dsp_cmpo(0, t) == 2) return;
        i = aec % 12u;
    } else {
        if (dsp_bbs(ages_ld32(AGES_G(7)), 3)) return;
        if (dsp_cmpo(0, ages_ld16(AGES_G(7) + 0x7FA)) == 2) return;
        t = ages_ld32(t);
        if (dsp_cmpo(0, t) == 2) return;
        i = ages_ld16(AGES_G(7) + 0x7FA);
        if (dsp_cmpo(31, i) != 1) i = i % 17u + 31u;
    }
    AGES_G(0) = ages_ld16(t + off + i * 4u);
}

/* From 0x31BE4, after kuc_motion left g0 alone. */
static void kuc_by_flags(uint32_t tab, uint32_t off, uint32_t aec)
{
    if (dsp_bbs(ages_ld32(KUC_FLAGS), 4)) {
        kuc_flag4(tab, off, aec);
    } else if (dsp_bbs(ages_ld32(KUC_FLAGS), 16)) {
        kuc_stand(tab, off);
    } else if (dsp_cmpo(0, ages_ld32(KUC_COUNT)) != 2) {
        kuc_counted(tab);
    } else if (dsp_bbs(ages_ld32(KUC_FLAGS), 9)) {
        kuc_flag9(tab, off);
    } else {
        kuc_stand(tab, off);
    }
}

static void kuc_pick(uint32_t tab, uint32_t off, uint32_t aec)
{
    uint32_t t = ages_ld32(tab + 0x18);
    if (dsp_cmpo(0, t) != 2 && dsp_cmpo(100, ages_ld16(0x5000A2)) != 1
        && dsp_bbs(ages_ld16(AGES_G(7) + 0x1F48), 9)) {
        AGES_G(0) = ages_ld16(t);
        return;
    }
    if (dsp_cmpo(49, ages_ld16(AGES_G(7) + 0x1A8)) == 2
        && dsp_bbc(ages_ld32(KUC_FLAGS), 29)) {
        kuc_frame40(tab);
        return;
    }
    if (dsp_cmpo(0x180, ages_ld16(AGES_G(7) + 0x1A8)) == 2) {
        kuc_motion180(tab);
        return;
    }
    uint32_t g0 = AGES_G(0);
    AGES_G(14) = 0x31BE0;             /* bal 0x31CE4 */
    kuc_motion(tab, off);
    if (dsp_cmpo(AGES_G(0), g0) == 2) kuc_by_flags(tab, off, aec);
}

static uint32_t afn_koshi_unit_change(int entry)
{
    kuc_start_timer();
    kuc_run_timer();
    uint32_t aec = ages_ld32(AGES_G(7) + 0xAEC);
    uint32_t tab = ages_ld32(0x6422400u + ages_ld8(AGES_G(7) + 0x1B0) * 4u);
    if (dsp_cmpo(0, tab) != 2) {
        uint32_t off = 0;
        if (!dsp_bbc(ages_ld16(AGES_G(7) + 0x1F48), 9)) off = 2;
        afn_kuc_sukebo_check(0);
        kuc_pick(tab, off, aec);
    }
    if (entry) ages_i960_ret();
    return 0;
}
