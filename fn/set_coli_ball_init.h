/* set_coli_ball_init: i960 0x1B34, PS2 0x1EC580. Word 3 (the radius) of each
 * of both fighters' 32 ball quads, 0x506000 to 0x50A19C and 0x506200 to
 * 0x50A224. */
#pragma once

static uint32_t afn_set_coli_ball_init(int entry) {
    for (uint32_t i = 0; i < 32u; i++)
        ages_st32(0x50A19Cu + i * 4u, ages_ld32(0x506000u + i * 16u + 0xCu));
    for (uint32_t i = 0; i < 32u; i++)
        ages_st32(0x50A224u + i * 4u, ages_ld32(0x506200u + i * 16u + 0xCu));
    AGES_AC = (AGES_AC & ~7u) | 2u;
    if (entry) ages_i960_ret();
    return 0;
}
