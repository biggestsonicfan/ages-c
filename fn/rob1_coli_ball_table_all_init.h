/* rob1_coli_ball_table_all_init: i960 0x19A4, PS2 0x1EC780. Fighter 1's collision
 * tables from ROM: the ball quads, the unit address and unit tables, the
 * radii, the colours and the unit counts, each to its fixed RAM block
 * through g0 (and g1). g0 and g1 are left as the last helper leaves them. */
#pragma once

static uint32_t afn_rob1_coli_ball_table_all_init(int entry) {
    AGES_G(0) = 0x506200u;
    afn_cb_data_init(0);
    AGES_G(0) = 0x506440u;
    AGES_G(1) = 0x506500u;
    afn_ucb_adr_init(0);
    AGES_G(0) = 0x506500u;
    afn_ucb_unit_init(0);
    AGES_G(0) = 0x506600u;
    afn_coli_bu_rd_init(0);
    AGES_G(0) = 0x5066A0u;
    afn_coli_bu_co_init(0);
    AGES_G(0) = 0x5066D0u;
    afn_coli_ucb_num_init(0);
    if (entry) ages_i960_ret();
    return 0;
}
