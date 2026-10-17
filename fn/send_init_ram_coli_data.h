/* send_init_ram_coli_data: i960 0x1B0C, PS2 0x1EC080, EXT 61. Load fighter
 * (g7)'s collision tables from ROM (rob1_ when the byte at g7+4 is 1, else
 * rob0_) and send them (send_ram_coli_data, which it falls into). */
#pragma once

static uint32_t afn_send_init_ram_coli_data(int entry) {
    if (ages_ld8(AGES_G(7) + 4u) == 1u)
        afn_rob1_coli_ball_table_all_init(0);
    else
        afn_rob0_coli_ball_table_all_init(0);
    afn_send_ram_coli_data(0);
    if (entry) ages_i960_ret();
    return 0;
}
