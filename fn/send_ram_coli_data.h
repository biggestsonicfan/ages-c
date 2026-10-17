/* send_ram_coli_data: i960 0x1B20, PS2 0x1EC000, EXT 62. Send both fighters'
 * collision balls to the COP: the radii, the units and their colours. On the
 * board this is the tail of send_init_ram_coli_data. */
#pragma once

static uint32_t afn_send_ram_coli_data(int entry) {
    afn_set_coli_ball_init(0);
    afn_send_coli_ball_R(0);
    afn_send_ball_to_unit(0);
    afn_send_ball_to_unit2(0);
    if (entry) ages_i960_ret();
    return 0;
}
