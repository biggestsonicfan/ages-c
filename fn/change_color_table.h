/* change_color_table: i960 0xD3C, PS2 0x1EAA80 (EXT 89;
 * decomp/FV/FN/ext89_change_color_table_001eaa80.c).
 *
 * The animated colour tables, one quarter a frame: on while 0x57100C bit 1 is
 * set, 0x508000 bit 5 clear and the player record (0x500814) bit 31 set. A new
 * stage (+0x310) restarts the script (0xF4C table into 0x571010, step
 * 0x571004 = 0). Step 0-2 copies 32 rows of 64 bytes from 0x200000 +
 * 0x8000*q + 0x800*script[step] into the colour RAM at 0x1810000 + 0x4000*q
 * (one halfword each, 0x180 between rows); after the fourth the script moves
 * on 4 bytes, back to 0xF6C at a 0xFF. The PS2 keeps the work words at
 * 0x571004-0x571020 on the host; here they are the board's. */
#pragma once
#include "send_tex_col_go.h"

static void col_change_copy(uint32_t step) {
    uint32_t q = step & 3u;
    uint32_t dst = 0x1810000u + (q << 14);
    uint32_t src = 0x200000u + (q << 15)
                 + (ages_ld8(step + ages_ld32(0x571010u)) << 11);
    for (uint32_t row = 0; row < 32; row++, dst += 0x180u)
        for (uint32_t i = 0; i < 64; i++, dst += 2, src++)
            ages_st16(dst, ages_ld8(src));
}

static void col_change_next(void) {
    uint32_t step = ages_ld32(0x571004u) + 1u;
    col_set_cc(col_cmpo(4, step));
    if (step >= 4) {
        uint32_t next = ages_ld32(0x571010u) + 4u;
        uint32_t b = ages_ld8(next);
        col_set_cc(col_cmpo(b, 0xFF));
        if (b == 0xFF) next = 0xF6C;
        ages_st32(0x571010u, next);
        step = 0;
    }
    ages_st32(0x571004u, step);
}

/* The three gates, bbc/bbs: the CC a failed one leaves (0 clear, 2 set). */
static int col_change_on(uint32_t *rec) {
    col_set_cc(0);
    if (!(ages_ld32(0x57100Cu) & 2u)) return 0;
    col_set_cc(2);
    if (ages_ld32(0x508000u) & 0x20u) return 0;
    col_set_cc(0);
    *rec = ages_ld32(0x500814u);
    return (ages_ld32(*rec) & 0x80000000u) != 0;
}

static uint32_t afn_change_color_table(int entry) {
    uint32_t rec = 0;
    if (col_change_on(&rec)) {
        uint32_t stage = ages_ld8(rec + 0x310u);
        if (stage != ages_ld32(0x571020u)) {
            ages_st32(0x571020u, stage);
            ages_st32(0x571010u, ages_ld32(0xF4Cu + stage * 4u));
            ages_st32(0x571004u, 0);
        }
        uint32_t step = ages_ld32(0x571004u);
        if ((step & 3u) != 3u) col_change_copy(step);
        col_change_next();
    }
    if (entry) ages_i960_ret();
    return 0;
}
