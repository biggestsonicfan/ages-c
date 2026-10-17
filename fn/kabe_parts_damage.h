/* kabe_parts_damage: i960 0x273F4, PS2 0x1E0EC0, EXT 5A. A wall hit: g0 / 4
 * (divo: unsigned) added to each of fighter g7's 16 part damages (halfwords
 * at g7+0x1F00), then calc_up_down_damage. */
#pragma once

static uint32_t afn_kabe_parts_damage(int entry) {
    uint32_t add = AGES_G(0) / 4u;
    for (uint32_t i = 0; i < 16u; i++) {
        uint32_t a = AGES_G(7) + 0x1F00u + i * 2u;
        ages_st16(a, (uint32_t)ages_ld16s(a) + add);
    }
    afn_calc_up_down_damage(0);
    if (entry) ages_i960_ret();
    return 0;
}
