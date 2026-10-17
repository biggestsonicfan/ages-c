/* send_ball_to_unit: i960 0x1DE4, PS2 0x1EC380. For each of 32 balls, a COP
 * handshake and two Fn_write_ram (0x49) words: the words at 0x506580+4i and
 * 0x506600+4i to DM 0x306A0+i and 0x307A0+i.
 *
 * colig_cop_pair is the group's shared send, used by send_ball_to_unit2 and
 * send_coli_ball_R too: three 0x09801313 words and one read (dropped), then
 * two write_ram commands. */
#pragma once

static void colig_cop_pair(uint32_t dm1, uint32_t v1, uint32_t dm2, uint32_t v2) {
    ages_cop_w(0x09801313u);
    ages_cop_w(0x09801313u);
    ages_cop_w(0x09801313u);
    (void)ages_cop_r();
    ages_cop_w(0x24804949u);
    ages_cop_w(dm1);
    ages_cop_w(v1);
    ages_cop_w(0x24804949u);
    ages_cop_w(dm2);
    ages_cop_w(v2);
}

static uint32_t afn_send_ball_to_unit(int entry) {
    for (uint32_t i = 0; i < 32u; i++)
        colig_cop_pair(0x306A0u + i, ages_ld32(0x506580u + i * 4u),
                       0x307A0u + i, ages_ld32(0x506600u + i * 4u));
    AGES_AC = (AGES_AC & ~7u) | 2u;   /* cmpdeco 1, 1 */
    if (entry) ages_i960_ret();
    return 0;
}
