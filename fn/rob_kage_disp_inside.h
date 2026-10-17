/* rob_kage_disp_inside: i960 0x602F0, PS2 0x1F3C80 (EXT 67;
 * decomp/FV/FN/ext67_rob_kage_disp_inside_001f3c80.c).
 *
 * Fighter g7's shadow on the inside stages. The stage's flags (0x6442BA8,
 * 12 bytes a stage) can turn it off (bit 8; bit 31 without bit 7 or with
 * the low nibble of 0x574004 clear). On an extend stage (0x5FA6C) the
 * shadow falls from the nearest light (calc_near_light, calc_ext_light_vec,
 * rob_kage_set_matrix, rob_kage_set_flag_inout); else the plain shadow and,
 * without flag bit 9, the loose parts'; with g7+0x2A set, the blob model
 * of 0x50B100's record (0x50B00C) instead. The PS2 inlines
 * rob_kage_set_matrix; this calls it, as the board does. */
#pragma once

#include "rob_kage_disp_test.h"
#include "calc_ext_light_vec.h"
#include "rob_kage_set_matrix.h"
#include "rob_kage_set_flag_inout.h"
#include "parts_kage_set_flag.h"
#include "parts_kage_disp.h"

#define RKI_CC(cc) (AGES_AC = (AGES_AC & ~7u) | (cc))

/* 0x60468: the shadow from the nearest light. */
static void rki_extend(void) {
    afn_calc_near_light(0);
    uint32_t light = AGES_G(0);
    afn_calc_ext_light_vec(0);
    if (AGES_G(0) == 0) { RKI_CC(2u); return; }
    afn_rob_kage_set_matrix(0);
    AGES_G(0) = light;
    afn_rob_kage_set_flag_inout(0);
    if (ages_ld8(AGES_G(7) + 0x2Au) != 0) { RKI_CC(4u); return; }
    AGES_G(4) = ages_ld32(AGES_G(7) + 0xAA8u + light * 4u);
    AGES_G(5) = 1;
    afn_rob_kage_disp_test(0);
}

/* 0x6035C: the blob shadow. */
static void rki_blob(void) {
    uint32_t rec = 0x50B100u + (ages_ld8(0x50B00Cu) << 5);
    uint32_t model = ages_ld32(rec + 0x1Cu);
    if (model == 0) { RKI_CC(2u); return; }
    ages_cop_w(0x00800101u);
    ages_cop_w(0x34006868u);
    ages_cop_w(0x3D00u);
    ages_cop_w(0x1B803737u);
    ages_cop_w(ages_ld8(AGES_G(7) + 4u));
    ages_cop_w(0);
    float y = ages_ldf(rec + 8u) - 1.0f;
    ages_cop_w(0x03000606u);
    ages_cop_w(0);
    ages_cop_wf(y);
    ages_cop_w(0);
    AGES_G(1) = 0;
    AGES_G(0) = model;
    afn_set_obj(0);
    ages_cop_w(0x01000202u);
}

/* The stage's flags; false when they turn the shadow off. */
static int rki_on(uint32_t *flags) {
    uint32_t f = ages_ld32(0x6442BA8u + ages_ld8(0x500064u) * 12u);
    *flags = f;
    if (f & 0x100u) { RKI_CC(2u); return 0; }
    if (!(f & 0x80000000u)) return 1;
    if (!(f & 0x80u)) { RKI_CC(0u); return 0; }
    if ((ages_ld32(0x574004u) & 15u) == 0) { RKI_CC(2u); return 0; }
    return 1;
}

static void rki_body(void) {
    uint32_t flags;
    if (!rki_on(&flags)) return;
    if (ages_ld8(0x5FA6Cu + ages_ld8(0x500064u)) != 0) { rki_extend(); return; }
    if (ages_ld8(AGES_G(7) + 0x2Au) != 0) { rki_blob(); return; }
    AGES_G(4) = ages_ld32(AGES_G(7) + 0xAA8u);
    AGES_G(5) = 1;
    afn_rob_kage_disp_test(0);
    if (flags & 0x200u) { RKI_CC(2u); return; }
    afn_parts_kage_set_flag(0);
    afn_parts_kage_disp(0);
}

static uint32_t afn_rob_kage_disp_inside(int entry) {
    rki_body();
    if (entry) ages_i960_ret();
    return 0;
}
