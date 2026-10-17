/* set_obj_fifo: i960 0x5D38, PS2 0x1CBE40 (EXT 10;
 * decomp/FV/FN/ext10_set_obj_fifo_001cbe40.c).
 *
 * Draw model g0 under the 12-word matrix waiting in the COP's reply FIFO.
 * Nothing more unless g2 is not 1, the polygon budget is not spent, the
 * matrix's first word is at most 1.0 and its last at least 0.2 in size; then
 * the matrix goes to the display list (GEO 0xB0B) and to g3's buffer, and
 * the model, its texture-header word replaced by 0x800000 or 0x802000 for
 * the fighter flags at g7+0x1A4 / +0x720 / +0x1AA. g2 and g4 keep the
 * i960's scratch values. The PS2 ends in its own renderer; this is the
 * i960's tail (0x5C38 or set_obj_common, then set_obj_go). */
#pragma once
#include "set_obj.h"

/* The matrix checks, 0x5D44-0x5D80; false to return. */
static bool obj_fifo_pass(const uint32_t *m)
{
    obj_cmpo(1, AGES_G(2));
    if (AGES_G(2) == 1) return false;
    AGES_G(2) = ages_ld32(0x501018);
    uint32_t count = ages_ld32(0x50101C);
    obj_cmpo(count, AGES_G(2));
    if (count > AGES_G(2)) return false;
    AGES_G(4) = m[0] & 0x7FFFFFFFu;
    obj_cmpo(0x3F800000u, AGES_G(4));
    if (0x3F800000u < AGES_G(4)) return false;
    AGES_G(4) = m[11] & 0x7FFFFFFFu;
    obj_cmpo(0x3E4CCCCDu, AGES_G(4));
    return !(0x3E4CCCCDu > AGES_G(4));
}

/* 0x5DC0-0x5DF4: the replacement texture header, 0 for none. */
static uint32_t obj_fifo_header(void)
{
    if (ages_ld32(AGES_G(7) + 0x1A4) & (1u << 26)) {
        if ((uint32_t)ages_ld16s(AGES_G(7) + 0x1AA) <= 4u) return 0x800000u;
    }
    if (ages_ld32(AGES_G(7) + 0x720) & 4u) {
        uint32_t f = (uint32_t)ages_ld16s(AGES_G(7) + 0x1AA);
        if (f <= 4u && (f & 1u)) return 0x802000u;
    }
    return 0;
}

static uint32_t afn_set_obj_fifo(int entry)
{
    uint32_t m[12];
    gems_cop_rn(m, 12);
    if (obj_fifo_pass(m)) {
        obj_geo_cmd(0xB0, 0xB0B);
        for (int q = 0; q < 12; q += 4) ages_stn(AGES_G(10) + AGES_G(12), &m[q], 4);
        for (int q = 0; q < 12; q += 4) {
            ages_stn(AGES_G(3), &m[q], 4);
            AGES_G(3) += 16;
        }
        uint32_t h = obj_fifo_header();
        if (h) {
            AGES_G(2) = h;
            obj_replaced(1);
        } else {
            obj_common();
        }
    }
    if (entry) ages_i960_ret();
    return 0;
}
