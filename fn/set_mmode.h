/* set_mmode: i960 0x58F0, PS2 0x1CB940 (EXT 66;
 * decomp/FV/FN/ext66_set_mmode_001cb940.c).
 *
 * GEO command 07 (mode word) with g0. The i960 parks g14 at 0x50EFFC while
 * it carries the command and takes it back; that store is kept. */
#pragma once
#include "set_obj.h"

static uint32_t afn_set_mmode(int entry)
{
    obj_geo_cmd(0x70, 0x707);
    ages_st32(AGES_G(10) + AGES_G(12), AGES_G(0));
    if (entry) ages_i960_ret();
    return 0;
}
