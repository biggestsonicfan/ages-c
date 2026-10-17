/* trailer_easy_dsp2: i960 0x6DAFC (a display routine set at +0x10 of a stage
 * task by trailer_easy_int2), PS2 0x1FE500 (EXT 70;
 * decomp/FV/FN/ext70_trailer_easy_dsp2_001fe500.c).
 *
 * The trailer (task g6, position +0x18): models 0xBB7 (cab) and 0xBB6 (box)
 * into +0x74/+0x76, the trailer's matrix (COP 0x06, 0x07 with -1, 1, 1),
 * the cab and its four wheels (model 0xAE5, turned by 0x500020 << 12 on COP
 * 0x0A), the box and its four wheels the other way, then the four lamp quads
 * (COP 0x29 pairs) into 0x57406C-0x5740AB; while +0x10 is still this routine
 * the lamps take the -1, 1, 1 scale too (trailer_pos_get_go, 0x6E13C).
 * The PS2's FUN_001c62c0 (a host flag around the draw) has no board side
 * and is left out. */
#pragma once
#include "set_obj.h"
#include "tpd_clip_stage.h"

static const uint32_t trl_wheel_cab[4][3] = {
    { 0, 0x3EEC0831u, 0x3F8CCCCDu }, { 0, 0, 0xC00CCCCDu },
    { 0x4039999Au, 0, 0 },           { 0, 0, 0x400CCCCDu },
};
static const uint32_t trl_wheel_box[4][3] = {
    { 0x40E9999Au, 0x3EEC0831u, 0x3F8CCCCDu }, { 0, 0, 0xC00CCCCDu },
    { 0x3F8CCCCDu, 0, 0 },                     { 0, 0, 0x400CCCCDu },
};

static void trl_cop3(uint32_t cmd, uint32_t a, uint32_t b, uint32_t c) {
    ages_cop_w(cmd);
    ages_cop_w(a);
    ages_cop_w(b);
    ages_cop_w(c);
}

static void trl_set_obj(uint32_t model) {
    AGES_G(1) = 0;
    AGES_G(0) = model;
    afn_set_obj(0);
}

/* Four wheels at w[], each turned by `turn` (COP 0x0A) and back. */
static void trl_wheels(const uint32_t (*w)[3], uint32_t turn) {
    for (int i = 0; i < 4; i++) {
        trl_cop3(0x3000606u, w[i][0], w[i][1], w[i][2]);
        ages_cop_w(0x5000A0Au);
        ages_cop_w(turn);
        trl_set_obj(0xAE5);
        ages_cop_w(0x5000A0Au);
        ages_cop_w(0u - turn);
    }
    ages_cop_w(0x1000202u);
}

/* A lamp quad: the base point moved (COP 0x29), the tip moved, and
 * (base x, base z, tip x - base x, tip z - base z) at 0x57402C + 4*slot. */
static void trl_lamp(const uint32_t *base, uint32_t tx, uint32_t tz, uint32_t slot) {
    trl_cop3(0x14802929u, tx, 0, tz);
    uint32_t q[4] = { base[0], base[2], ages_cop_r(), 0 };
    (void)ages_cop_r();
    q[3] = ages_cop_r();
    q[2] = tpd_subr(q[0], q[2]);
    q[3] = tpd_subr(q[1], q[3]);
    ages_stn(0x57402Cu + slot * 4u, q, 4);
}

static void trl_lamp_base(uint32_t x, uint32_t z, uint32_t *b) {
    trl_cop3(0x14802929u, x, 0, z);
    for (int i = 0; i < 3; i++) b[i] = ages_cop_r();
}

static void trl_lamps(int mirror) {
    uint32_t b[3];
    ages_cop_w(0x800101u);
    ages_cop_w(0x1800303u);
    trl_cop3(0x3000606u, ages_ld32(AGES_G(6) + 0x18u), ages_ld32(AGES_G(6) + 0x1Cu),
             ages_ld32(AGES_G(6) + 0x20u));
    if (mirror) trl_cop3(0x3800707u, 0xBF800000u, 0x3F800000u, 0x3F800000u);
    ages_cop_w(0x800101u);
    trl_lamp_base(0xBEF5C28Fu, 0xBF95C28Fu, b);
    trl_lamp(b, 0x3FBD70A4u, 0xBF9EB852u, 16);
    trl_lamp(b, 0xBEF5C28Fu, 0x3F95C28Fu, 20);
    ages_cop_w(0x1000202u);
    trl_cop3(0x3000606u, 0x40200000u, 0, 0);
    trl_lamp_base(0x41240000u, 0xBFB851ECu, b);
    trl_lamp(b, 0xBF266666u, 0xBFB851ECu, 24);
    trl_lamp(b, 0x41240000u, 0x3FB851ECu, 28);
    ages_cop_w(0x1000202u);
}

static uint32_t afn_trailer_easy_dsp2(int entry) {
    ages_st16(AGES_G(6) + 0x74u, 0xBB7);
    ages_st16(AGES_G(6) + 0x76u, 0xBB6);
    ages_cop_w(0x800101u);
    trl_cop3(0x3000606u, ages_ld32(AGES_G(6) + 0x18u), ages_ld32(AGES_G(6) + 0x1Cu),
             ages_ld32(AGES_G(6) + 0x20u));
    trl_cop3(0x3800707u, 0xBF800000u, 0x3F800000u, 0x3F800000u);
    ages_cop_w(0x800101u);
    uint32_t turn = ages_ld32(0x500020u) << 12;
    trl_set_obj((uint32_t)ages_ld16s(AGES_G(6) + 0x74u));
    trl_wheels(trl_wheel_cab, 0u - turn);
    trl_cop3(0x3000606u, 0x40200000u, 0, 0);
    trl_set_obj((uint32_t)ages_ld16s(AGES_G(6) + 0x76u));
    trl_wheels(trl_wheel_box, turn);
    uint32_t dsp = ages_ld32(AGES_G(6) + 0x10u);
    col_set_cc(col_cmpo(0x6DAFCu, dsp));
    trl_lamps(dsp == 0x6DAFCu);
    if (entry) ages_i960_ret();
    return 0;
}
