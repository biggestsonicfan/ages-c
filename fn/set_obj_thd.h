/* set_obj_thd: i960 0x5BE4, PS2 0x1CBB80 (EXT 0f;
 * decomp/FV/FN/ext0f_set_obj_thd_001cbb80.c).
 *
 * As set_obj_tpd, but g2 replaces the model entry's second word (the texture
 * header address). The PS2 ends in its own renderer; this is the i960's tail
 * (0x5C38, set_obj_go). */
#pragma once
#include "set_obj.h"

static uint32_t afn_set_obj_thd(int entry)
{
    if (obj_budget()) {
        obj_mov_matrix();
        obj_replaced(1);
    }
    if (entry) ages_i960_ret();
    return 0;
}
