/* send_tpd: i960 0x3360C, PS2 0x1ED940 (a helper of tex_move_poly; STF's
 * cousin is gems-c's move_tpd_send).
 *
 * set_tpd_data's queue (0x50E004 records, 0x50E008 points) to the GEO: GEO
 * command 0x404 at g10+0x40, the destination (0x805000) and twice the point
 * count down the FIFO (g10+g12), then for each record its points (+0, count
 * at +6) as (x + the record's scroll from 0x50E300, y) halfword pairs, sign
 * extended; then GEO 0x1010 at g10+0x100 and a 0. Where each record's points
 * land goes into 0x50E200[] (one past the last too), for tex_move_poly. */
#pragma once
#include "set_obj.h"
#include "send_tex_col_go.h"

static void tpd_send_points(uint32_t rec, uint32_t scroll) {
    uint32_t list = ages_ld32(rec);
    uint32_t n = (uint32_t)ages_ld16s(rec + 6u), more;
    do {
        uint32_t xy[2] = { (uint32_t)ages_ld16s(list) + scroll, (uint32_t)ages_ld16s(list + 2u) };
        ages_stn(AGES_G(10) + AGES_G(12), xy, 2);
        list += 4u;
        more = n > 1u;
        col_set_cc(col_cmpo(1, n));
        n--;
    } while (more);
}

static uint32_t afn_send_tpd(int entry) {
    uint32_t left = ages_ld32(0x50E004u);
    col_set_cc(col_cmpo(0, left));
    if (left != 0) {
        uint32_t dst = 0x805000u, k = 0, more;
        ages_st32(0x50E200u, dst);
        obj_geo_cmd(0x40, 0x404);
        ages_st32(AGES_G(10) + AGES_G(12), dst);
        ages_st32(AGES_G(10) + AGES_G(12), ages_ld32(0x50E008u) * 2u);
        do {
            uint32_t scroll = ages_ld32(0x50E300u + k * 4u);
            uint32_t rec = ages_ld32(0x50E100u + k * 4u);
            dst += (uint32_t)ages_ld16s(rec + 6u) * 2u;
            ages_st32(0x50E204u + k * 4u, dst);
            tpd_send_points(rec, scroll);
            k++;
            more = left > 1u;
            col_set_cc(col_cmpo(1, left));
            left--;
        } while (more);
        obj_geo_cmd(0x100, 0x1010);
        ages_st32(AGES_G(10) + AGES_G(12), 0);
    }
    if (entry) ages_i960_ret();
    return 0;
}
