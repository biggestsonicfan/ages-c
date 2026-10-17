/* cb_data_init: i960 0x1AA8, PS2 0x1ECF80. 32 collision-ball quads from the
 * ROM table at 0x640E4FC to g0, then 2.0f to 0x50A21C/0x50A2A4 and 30 to
 * 0x50A220/0x50A2A8 (send_coli_ball_R sends them to DM 0x3x6F0/0x3x6F1). */
#pragma once

static uint32_t afn_cb_data_init(int entry) {
    uint32_t q[4];
    for (uint32_t i = 0; i < 32u; i++) {
        ages_ldn(q, 0x640E4FCu + i * 16u, 4);
        ages_stn(AGES_G(0) + i * 16u, q, 4);
    }
    ages_st32(0x50A21Cu, 0x40000000u);
    ages_st32(0x50A2A4u, 0x40000000u);
    ages_st32(0x50A220u, 30u);
    ages_st32(0x50A2A8u, 30u);
    AGES_AC = (AGES_AC & ~7u) | 2u;   /* the loop's last cmpobne */
    if (entry) ages_i960_ret();
    return 0;
}
