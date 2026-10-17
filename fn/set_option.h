/* set_option: i960 0x1CA84 (a label after play_motion), PS2 0x1DB200.
 * Also delete_option (i960 0x1CA68), which the PS2 writes inline.
 *
 * set_option: copy_option_data, then when the option has a model
 * (g7+0x784), set bit 27 of g7+0, lay its two quads into the model at
 * [g7+0x1D0C], fill in its collision tables and send them
 * (send_ram_coli_data). delete_option: clear bit 27 and g7+0xAFC and send the
 * initial collision data (send_init_ram_coli_data). */
#pragma once

static void opt_delete_option(void) {
    uint32_t g7 = AGES_G(7);
    ages_st32(g7, ages_ld32(g7) & ~(1u << 27));
    ages_st32(g7 + 0xAFC, 0);
    afn_send_init_ram_coli_data(0);
}

static uint32_t afn_set_option(int entry) {
    uint32_t g7 = AGES_G(7), q[4];
    afn_copy_option_data(0);
    uint32_t rec = ages_ld32(g7 + 0x784);
    if (rec == 0) {
        AGES_AC = (AGES_AC & ~7u) | 2u;
    } else {
        uint32_t model = ages_ld32(g7 + 0x1D0C), r12 = ages_ld32(g7 + 0x1D18);
        uint32_t r10 = ages_ld32(g7 + 0x1D1C), r9 = ages_ld32(g7 + 0x1D20);
        uint32_t r8 = ages_ld32(g7 + 0x1D10), r13 = ages_ld32(g7 + 0x1D08);
        ages_st32(g7, ages_ld32(g7) | (1u << 27));
        ages_ldn(q, rec + 0x04, 4); ages_stn(model + 0x1E0, q, 4);
        ages_ldn(q, rec + 0x14, 4); ages_stn(model + 0x1F0, q, 4);
        uint32_t n = ages_ld16(g7 + 0x7A0);
        ages_st32(r12 + 0x78, n);
        ages_st32(r12 + 0x7C, n);
        ages_st8(r10 + 0x1E, n);
        ages_st8(r10 + 0x1F, n);
        uint32_t k = ages_ld8(0x640E914u + n);
        ages_st8(r9 + n, k + 2u);
        uint32_t t = ages_ld32(r8 + n * 4u);
        ages_st8(t + k, 0x78);
        ages_st8(t + k + 1u, 0x7C);
        ages_st32(r13, 32);
        afn_send_ram_coli_data(0);
    }
    if (entry) ages_i960_ret();
    return 0;
}
