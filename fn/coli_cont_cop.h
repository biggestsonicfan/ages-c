/* coli_cont_cop: i960 0x27E18, PS2 0x1DBBC0, EXT 6B. The fight collision of
 * the frame: COP 0x3E (the pair's x/z), 0x3A (attack masks from
 * calc_attack_flag, kinds +0x821), coli_ram_init, area_coli for each fighter,
 * decide_coli_kind, then either decide_dir alone or COP 0x3B (the narrow
 * phase: contact g13+0x160/0x164, g4, g5; g5 is averaged over four frames
 * with g13+0xF8..0x100) before coli_recalc_pos, which the board reaches by
 * `bal` (g14 = the return address) with the entry x/z still in r4..r10. */
#pragma once
#include "decide_dir.h"            /* colig_ FPU */
#include "coli_recalc_pos.h"

/* g6 bit 5 / 7: the thrower's frame +0x1AA in [+0x80A, +0x80A + 15). */
static int colig_ccc_window(uint32_t rob) {
    uint32_t r3 = (uint32_t)ages_ld16s(rob + 0x1AAu), r7 = (uint32_t)ages_ld16s(rob + 0x80Au);
    return r3 >= r7 && r3 < r7 + 15u;
}

/* 1: decide_dir and the step only; 2: decide_dir, then the narrow phase;
 * 0: the narrow phase. */
static int colig_ccc_held(void) {
    AGES_G(7) = ages_ld32(0x500804u);
    if (ages_ld32(AGES_G(7) + 0x1A4u) & 0x20000000u) return 2;
    AGES_G(8) = ages_ld32(0x500808u);
    if (ages_ld32(AGES_G(8) + 0x1A4u) & 0x20000000u) return 2;
    if (AGES_G(6) & 1u) return 1;
    if (AGES_G(6) & 0x20u) return colig_ccc_window(AGES_G(7));
    if (AGES_G(6) & 0x80u) return colig_ccc_window(AGES_G(8));
    return 0;
}

static void colig_ccc_narrow(void) {
    uint32_t g13 = AGES_G(13);
    AGES_G(7) = ages_ld32(0x500804u);
    uint32_t r11 = ages_ld32(AGES_G(7) + 0x77Cu);
    AGES_G(8) = ages_ld32(0x500808u);
    uint32_t r12 = ages_ld32(AGES_G(8) + 0x77Cu);
    ages_cop_w(0x1D803B3Bu);
    ages_cop_w(ages_ld32(g13 + 0x88u));
    ages_cop_w(r11);
    ages_cop_w(r12);
    ages_cop_w(ages_ld32(g13 + 0xC0u));
    ages_cop_w(ages_ld32(g13 + 0xC4u));
    ages_cop_w(ages_ld32(g13 + 0x264u));
    ages_cop_w(ages_ld32(g13 + 0x268u));
    r12 = ages_cop_r();
    uint32_t r13 = ages_cop_r();
    AGES_G(4) = ages_cop_r();
    AGES_G(5) = ages_cop_r();
    ages_st32(g13 + 0x160u, r12);
    ages_st32(g13 + 0x164u, r13);
    uint32_t r14 = ages_ld32(g13 + 0xFCu);
    uint32_t r15 = colig_addr(r14, ages_ld32(g13 + 0x100u));
    ages_st32(g13 + 0x100u, r14);
    r14 = ages_ld32(g13 + 0xF8u);
    r15 = colig_addr(r14, r15);
    ages_st32(g13 + 0xFCu, r14);
    r15 = colig_addr(AGES_G(5), r15);
    ages_st32(g13 + 0xF8u, AGES_G(5));
    AGES_G(5) = colig_divr(0x40800000u, r15);       /* / 4.0f */
}

static void colig_ccc_area(uint32_t a, uint32_t b) {
    AGES_G(7) = ages_ld32(a);
    AGES_G(8) = ages_ld32(b);
    afn_area_coli(0);
}

static uint32_t afn_coli_cont_cop(int entry) {
    uint32_t g7 = AGES_G(7), g8 = AGES_G(8);
    uint32_t r4 = ages_ld32(g7 + 0x1F4u), r5 = ages_ld32(g7 + 0x1F8u), r6 = ages_ld32(g7 + 0x1FCu);
    uint32_t r8 = ages_ld32(g8 + 0x1F4u), r10 = ages_ld32(g8 + 0x1FCu);
    uint32_t dx = colig_subr(r4, r8), dz = colig_subr(r6, r10);
    ages_cop_w(0x1F003E3Eu);
    ages_cop_w(dx);
    ages_cop_w(dz);
    ages_cop_w(r4);
    ages_cop_w(r5);
    ages_cop_w(r6);
    AGES_G(7) = ages_ld32(0x500804u);
    afn_calc_attack_flag(0);
    uint32_t r3 = AGES_G(3);
    AGES_G(7) = ages_ld32(0x500808u);
    afn_calc_attack_flag(0);
    uint32_t r7 = AGES_G(3);
    uint32_t k0 = ages_ld8(ages_ld32(0x500804u) + 0x821u);
    AGES_G(7) = ages_ld32(0x500808u);
    ages_cop_w(0x1D003A3Au);
    ages_cop_w(r3);
    ages_cop_w(r7);
    ages_cop_w(k0);
    ages_cop_w(ages_ld8(AGES_G(7) + 0x821u));
    afn_coli_ram_init(0);
    colig_ccc_area(0x500804u, 0x500808u);
    colig_ccc_area(0x500808u, 0x500804u);
    afn_decide_coli_kind(0);
    int held = colig_ccc_held();
    if (held) afn_decide_dir(0);
    AGES_G(14) = 0x27F34u;
    if (held != 1) {
        colig_ccc_narrow();
        AGES_G(14) = 0x27FDCu;
    }
    uint32_t cc = colig_recalc_pos(dx, dz);
    AGES_AC = (AGES_AC & ~7u) | cc;
    if (entry) ages_i960_ret();
    return 0;
}
