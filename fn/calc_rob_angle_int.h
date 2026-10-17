/* calc_rob_angle_int: i960 0x29D08, PS2 0x1E2A00 (EXT 5d;
 * decomp/FV/FN/ext5d_calc_rob_angle_int_001e2a00.c).
 *
 * When g7+0x1A8 names a pose (n != 0): unpacks the pose record at
 * *(0x6C00004 + n*4) into the per-unit kinds at g6+0x78C (g6 = *(g7+0xBD8)),
 * sets g6+0x780/0x784/0x788 to its data/kind/count pointers, then walks the
 * kinds: kind 0 loads g7+0x80, kinds 1 and 2 write cvtri'd angles to the
 * unit's record (table 0x2C218[row-12]) and set its bit in g7+0xBDC.
 * STF's version (gems-c) also loads the model tables; FV's does not.
 * Leaves g2, g5, g6 and CC (equal) as the i960 does. */
#pragma once

/* The unpack: 20 rows of three 2-bit kinds; returns the data pointer. */
static uint32_t m1_rob_angle_unpack(uint32_t rec)
{
    uint32_t p = rec + 2u, q = p + 20u, sum = 0, cnt = 0;
    AGES_G(6) = ages_ld32(AGES_G(7) + 0xBD8u);
    AGES_G(5) = AGES_G(6) + 0x78Cu;
    for (uint32_t row = 0; row < 20u; row++, p++) {
        for (uint32_t shift = 0; shift < 6u; shift += 2u) {
            uint32_t b = ages_ld8(p);
            uint32_t v = (b >> 6) & 3u;
            v = v ? v - 1u : ((b >> shift) & 3u) + 3u;
            ages_st8(AGES_G(5), v);
            AGES_G(5)++;
            if (v > 4u) {
                sum += ages_ld8(q);
                q++;
                cnt++;
            }
        }
    }
    ages_st32(AGES_G(6) + 0x784u, p);
    q += (4u - ((cnt + 22u) & 3u)) & 3u;
    ages_st32(AGES_G(6) + 0x788u, q);
    uint32_t data = q + sum * 4u;
    ages_st32(AGES_G(6) + 0x780u, data);
    AGES_G(2) = p;
    return data;
}

/* Kinds 1 and 2: the unit's record, its bit in g7+0xBDC. */
static uint32_t m1_rob_angle_unit(uint32_t row)
{
    uint32_t s = row - 12u;
    uint32_t bits = ages_ld8(AGES_G(7) + 0xBDCu) | (1u << (s & 31u));
    ages_st8(AGES_G(7) + 0xBDCu, bits);
    return ages_ld32(0x2C218u + s * 4u) + AGES_G(7);
}

/* One kind < 3 (the row's last): returns the data pointer moved on. */
static uint32_t m1_rob_angle_kind(uint32_t v, uint32_t row, uint32_t data)
{
    if (v == 0) {
        uint32_t t[3];
        ages_ldn(t, data, 3);
        ages_stn(AGES_G(7) + 0x80u, t, 3);
        return data + 12u;
    }
    uint32_t rec = m1_rob_angle_unit(row);
    if (v == 1u) {
        for (uint32_t k = 0; k < 3u; k++)
            ages_st16(rec + k * 2u, gems_cvtri(ages_ld32(data + k * 4u)));
        return data + 12u;
    }
    /* The first word, then three from one word on: the ROM's own layout. */
    ages_st16(rec, gems_cvtri(ages_ld32(data)));
    data += 4u;
    for (uint32_t k = 1; k < 4u; k++)
        ages_st16(rec + k * 2u, gems_cvtri(ages_ld32(data + k * 4u)));
    return data + 16u;
}

static uint32_t afn_calc_rob_angle_int(int entry)
{
    uint32_t pose = ages_ld16(AGES_G(7) + 0x1A8u);
    if (pose != 0) {
        uint32_t data = m1_rob_angle_unpack(ages_ld32(0x6C00004u + pose * 4u));
        AGES_G(5) = AGES_G(6) + 0x78Cu;
        ages_st8(AGES_G(7) + 0xBDCu, 0);
        for (uint32_t row = 0; row < 20u; row++) {
            for (uint32_t j = 0; j < 3u; j++) {
                uint32_t v = ages_ld8(AGES_G(5));
                if (v > 5u) {
                    data += ages_ld8(AGES_G(2)) * 12u;
                    AGES_G(2)++;
                } else if (v == 5u) {
                    data += ages_ld8(AGES_G(2)) * 4u;
                    AGES_G(2)++;
                } else if (v == 4u) {
                    data += 4u;
                } else if (v < 3u) {
                    data = m1_rob_angle_kind(v, row, data);
                    AGES_G(5) += 3u;
                    break;
                }
                AGES_G(5)++;
            }
        }
    }
    AGES_AC = (AGES_AC & ~7u) | 2u;   /* the last cmpo: equal */
    if (entry) ages_i960_ret();
    return 0;
}
