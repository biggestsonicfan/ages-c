/* set_obj_tpd: i960 0x5B68, PS2 0x1CBD00 (EXT 0e;
 * decomp/FV/FN/ext0e_set_obj_tpd_001cbd00.c).
 *
 * Draw model g0 with replaced texture points: the current matrix goes into
 * the display list through COP 0x34 (Fn_mov_matrix), then the model as
 * object command 0x101 with its first entry word replaced by g2. The PS2
 * ends in its own renderer; this is the i960's tail (0x5BBC, set_obj_go). */
#pragma once
#include "set_obj.h"

static uint32_t afn_set_obj_tpd(int entry)
{
    if (obj_budget()) {
        obj_mov_matrix();
        obj_replaced(0);
    }
    if (entry) ages_i960_ret();
    return 0;
}
