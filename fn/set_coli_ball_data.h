/* set_coli_ball_data: i960 0x18304, PS2 0x1DAF80. Sends part g0's
 * collision balls to the COP, last first: 0x39 (x, y, z, 3 * the kind
 * byte at 0x182E4[ball >> 2]) and, unless [[0x500828]] bit 31, 0x29 on the
 * same point (reply read and dropped). Unlike the PS2 the list is a
 * pointer table (0x1D10) and the kind comes from a table, not ball / 4. */
#pragma once

static uint32_t afn_set_coli_ball_data(int entry)
{
    uint32_t rob = AGES_G(7);
    uint32_t pos = ages_ld32(rob + 0x1D0C);
    uint32_t list = ages_ld32(ages_ld32(rob + 0x1D10) + AGES_G(0) * 4u);
    uint32_t n = ages_ld8(ages_ld32(rob + 0x1D20) + AGES_G(0));
    uint32_t mode = ages_ld32(ages_ld32(0x500828));
    uint32_t p[3];
    while (n-- > 0) {
        uint32_t ball = ages_ld8(list + n);
        ages_ldn(p, pos + ball * 4u, 3);
        ages_cop_w(0x1C803939);
        gems_cop_wn(p, 3);
        ages_cop_w(3u * ages_ld8(0x182E4u + (ball >> 2)));
        if (!(mode & 0x80000000u)) {
            ages_cop_w(0x14802929);
            gems_cop_wn(p, 3);
            gems_cop_rn(p, 3);
        }
    }
    AGES_AC = (AGES_AC & ~7u) | 2u;   /* cmpdeci 0, 0 */
    if (entry) ages_i960_ret();
    return 0;
}
