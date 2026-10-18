/* VF2 set_osage_after's record loop: i960 0x6539C..0x653EC. PS2
 * sysCopPatchSetOsageAfter 0x197180 (EXT 4b; decomp/VF2/FN/ext4b_*).
 *
 * After the TGP has solved a sway chain (TGP 0x25004A4A on 0x4C(g13)), walk
 * the script at 0x44(g13) and the records at 0x48(g13) again: each turn pops
 * one FIFO word (the PS2 leaves this out; the i960 has it at 0x653A0), and
 * type 5 (osage) draws its point's model through set_obj_fifo (0x7DC4) and
 * keeps its height at 0xF8(g13); types 1..4 call a bare `ret` (0x640F0).
 * The table at 0x640C0 moves g9 and g8 past each record, as in osage_dsp.
 *
 * The i960 resumes at the `ret` (0x653EC) with r3 the 0 word, r4/r5 the last
 * record's step. */
#pragma once

#include "osage_dsp.h"

/* 0x7DC4 set_obj_fifo: the TGP's three quads (a matrix) are popped; unless g2
 * is 1, the polygon budget is spent, |m[0]| > 1 or |m[11]| < 0.2, they go to
 * the GEO and to g3 (moved on 0x30), and model g0 is drawn (0x7CA4). */
static void vf2_os_obj_fifo(void)
{
    uint32_t m[12];
    for (int i = 0; i < 12; i++) m[i] = vf2_tgp_r();
    if (AGES_G(2) == 1) return;
    AGES_G(2) = ages_ld32(0x501018);
    if (ages_ld32(0x50101C) > AGES_G(2)) return;
    AGES_G(4) = m[0] & ~VF2_OS_SIGN;
    if (AGES_G(4) > 0x3F800000u) return;
    AGES_G(4) = m[11] & ~VF2_OS_SIGN;
    if (AGES_G(4) < 0x3E4CCCCDu) return;
    ages_st32(AGES_G(10) + 0xB0, 0);
    for (int i = 0; i < 12; i += 4) ages_stn(AGES_G(10) + AGES_G(12), m + i, 4);
    ages_stn(AGES_G(3), m, 12);
    AGES_G(3) += 0x30;
    vf2_obj_common();
}

/* 0x65404 / 0x65470: model at a(g9), hidden when 0(g13) bit 5 or the model
 * is 0; a drawn model leaves 0x1C(g9) after its matrix at 0x50E400. */
static void vf2_os_draw(uint32_t a)
{
    uint32_t g9 = AGES_G(9);
    AGES_G(3) = ages_ld32(0x50E400);
    AGES_G(2) = (ages_ld32(AGES_G(13)) >> 5) & 1u;
    AGES_G(0) = ages_ld32(g9 + a);
    AGES_G(1) = ages_ld8(AGES_G(7) + 4);
    AGES_G(2) |= AGES_G(0) == 0;
    uint32_t was = AGES_G(3);
    vf2_os_obj_fifo();
    if (AGES_G(3) == was) return;
    ages_st32(AGES_G(3), ages_ld32(g9 + 0x1C));
    AGES_G(3) += 4;
    ages_st32(0x50E400, AGES_G(3));
}

/* 0x653F0 type 5: the swaying point's model (0x20(g9) normally, 0x18(g9)
 * when 0x500068 bit 25 is clear or the fighter's 0x700 bit 5 is set). */
static void vf2_os_after_osage(void)
{
    uint32_t g13 = AGES_G(13), g8 = AGES_G(8);
    bool alt = !((ages_ld32(0x500068) >> 25) & 1u)
            || ((ages_ld32(AGES_G(7) + 0x700) >> 5) & 1u);
    if (!alt) {
        vf2_os_draw(0x20);
    } else {
        uint32_t f = ages_ld32(g13);
        if ((f & 0x100u) && AGES_G(9) == 0x69524u)
            ages_st32(g13, ages_ld32(g13) & ~0x20u);
        vf2_os_draw(0x18);
    }
    ages_st32(g13 + 0xF8, ages_ld32(g8));
    if (ages_ld16(g13 + 6) < 2) {
        static const uint32_t z[3] = { 0, 0, 0 };
        ages_stn(g8 + 0x0C, z, 3);
    }
}

/* EXT 4b at 0x6539C (`ld 0x48(g13),g8`): resume at the `ret`, 0x653EC. */
static uint32_t afn_vf2_set_osage_after(int entry)
{
    (void)entry;
    AGES_G(8) = ages_ld32(AGES_G(13) + 0x48);
    for (;;) {
        (void)vf2_tgp_r();
        uint32_t type = ages_ld32(AGES_G(9));
        AGES_G(9) += 4;
        AGES_G(8) += 4;
        AGES_R(3) = type;
        if (type == 0) break;
        if (type == 5) vf2_os_after_osage();
        vf2_os_step(type);
    }
    vf2_cmpo(0, 0);
    return 0xA0;   /* 0x653EC */
}
