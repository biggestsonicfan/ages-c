/* tex_move_poly: i960 0x331B4, PS2 0x1ED0C0 (EXT 6f;
 * decomp/FV/FN/ext6f_tex_move_poly_001ed0c0.c).
 *
 * The stage's moving-texture polygons: the list for stage 0x500064 (& 15) in
 * the table at 0x642065C, 0x24-byte records up to a -1. Each record on
 * screen (tpd_clip, 0x334C0: tpd_clip_stage at y 0x50A018 and 0) is queued
 * (set_tpd_data); send_tpd sends the queue's points, then each queued model
 * (+4) is drawn with set_obj_tpd under the camera (COP push, 0x06 with the
 * negated camera position 0x50A014-0x50A01C, 0x09 with the view tilt) with
 * its points (0x50E200[]). g5 counts, g6 walks; the queue words at
 * 0x50E000-0x50E008 are the board's (the PS2 keeps them on the host). */
#pragma once
#include "set_tpd_data.h"
#include "send_tpd.h"
#include "set_obj_tpd.h"

/* tpd_clip (0x334C0): g0 = 0, then the record's two heights. */
static void tpd_clip(void) {
    AGES_G(0) = 0;
    AGES_G(9) = ages_ld32(0x50A018u);
    afn_tpd_clip_stage(0);
    AGES_G(9) = 0;
    afn_tpd_clip_stage(0);
}

/* One queued model (0x3322C-0x332C0). */
static void tpd_draw(uint32_t k) {
    ages_cop_w(0x800101u);
    uint32_t model = (uint32_t)ages_ld16s(ages_ld32(0x50E100u + k * 4u) + 4u);
    uint32_t pts = ages_ld32(0x50E200u + k * 4u);
    ages_cop_w(0x3000606u);
    ages_cop_w(ages_ld32(0x50A014u) ^ 0x80000000u);
    ages_cop_w(ages_ld32(0x50A018u) ^ 0x80000000u);
    ages_cop_w(ages_ld32(0x50A01Cu) ^ 0x80000000u);
    ages_cop_w(0x4800909u);
    ages_cop_w(0u - (uint32_t)ages_ld16s(0x50A022u));
    AGES_G(2) = pts;
    AGES_G(1) = 0;
    AGES_G(0) = model;
    afn_set_obj_tpd(0);
    ages_cop_w(0x1000202u);
}

static uint32_t afn_tex_move_poly(int entry) {
    AGES_G(6) = ages_ld32(0x642065Cu + ((uint32_t)ages_ld8s(0x500064u) & 15u) * 4u);
    AGES_G(5) = 0;
    ages_st32(0x50E000u, 0);
    ages_st32(0x50E004u, 0);
    ages_st32(0x50E008u, 0);
    uint32_t w = ages_ld32(AGES_G(6));
    col_set_cc(col_cmpo(0xFFFFFFFFu, w));
    if (w != 0xFFFFFFFFu) {
        for (;;) {
            w = ages_ld32(AGES_G(6));
            col_set_cc(col_cmpo(0xFFFFFFFFu, w));
            if (w == 0xFFFFFFFFu) break;
            tpd_clip();
            col_set_cc(col_cmpo(0, AGES_G(0)));
            if (AGES_G(0) != 0) afn_set_tpd_data(0);
            AGES_G(6) += 0x24u;
            AGES_G(5)++;
        }
        afn_send_tpd(0);
        for (AGES_G(5) = 0;; AGES_G(5)++) {
            uint32_t n = ages_ld32(0x50E004u);
            col_set_cc(col_cmpo(AGES_G(5), n));
            if (AGES_G(5) == n) break;
            tpd_draw(AGES_G(5));
        }
    }
    if (entry) ages_i960_ret();
    return 0;
}
