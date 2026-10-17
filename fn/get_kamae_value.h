/* get_kamae_value: i960 0x2A900, PS2 0x1E1700. The first frame of motion g0
 * (the motion table at 0x6C00004), as 60 words at g3, the first 36 of them
 * turned to halfwords (cvtri, stis).
 *
 * The motion's header has 20 bytes of channel modes (three 2-bit fields each)
 * and a byte of key counts per keyed channel; get_kamae_value unpacks the
 * modes to 60 bytes at 0x50EA5C (g5, from g6 = 0x50E2D0), then walks them:
 * 0-1 a vector (3 words), 2 the rest of the triple skipped, 3 a zero, 4 a
 * constant, 5 and 6 the first key of a curve whose key count is the next
 * byte at g2 (4 or 12 bytes a key). g2, g3, g5 and g6 are left as the i960
 * leaves them. */
#pragma once

/* Pass 1: the 60 mode bytes. Returns the key data's offset after the count
 * bytes (r8 on the board); *nkeys = the sum of the key counts (r11). */
static uint32_t colig_kamae_modes(uint32_t *hdr, uint32_t *nkeys) {
    uint32_t r9 = *hdr, r8 = r9 + 20u, r11 = 0, r12 = 0;
    AGES_G(5) = AGES_G(6) + 0x78Cu;
    for (uint32_t i = 0; i < 20u; i++, r9++) {
        for (uint32_t sh = 0; sh < 6u; sh += 2u) {
            uint32_t b = ages_ld8(r9), m = (b >> 6) & 3u;
            m = m ? m - 1u : ((b >> sh) & 3u) + 3u;
            ages_st8(AGES_G(5), m);
            AGES_G(5)++;
            if (m > 4u) {
                r11 += ages_ld8(r8);
                r8++;
                r12++;
            }
        }
    }
    *hdr = r9;
    *nkeys = r11;
    return r8 + ((4u - ((r12 + 22u) & 3u)) & 3u);
}

/* One mode byte of pass 2; returns 1 to end the triple (modes 0-2). */
static int colig_kamae_one(uint32_t m, uint32_t *r7) {
    uint32_t g3 = AGES_G(3), q[3];
    if (m <= 1u) {
        ages_ldn(q, *r7, 3);
        ages_stn(g3, q, 3);
        *r7 += 12u;
        AGES_G(3) = g3 + 12u;
    }
    if (m <= 2u) {
        AGES_G(5) += 3u;
        return 1;
    }
    ages_st32(g3, m == 3u ? 0u : ages_ld32(*r7));
    AGES_G(3) = g3 + 4u;
    if (m == 4u) {
        *r7 += 4u;
    } else if (m >= 5u) {
        uint32_t n = ages_ld8(AGES_G(2));
        AGES_G(2)++;
        *r7 += n * (m == 5u ? 4u : 12u);
    }
    AGES_G(5)++;
    return 0;
}

static uint32_t afn_get_kamae_value(int entry) {
    AGES_G(6) = 0x50E2D0u;
    uint32_t hdr = ages_ld32(0x6C00004u + AGES_G(0) * 4u) + 2u, nkeys;
    uint32_t r8 = colig_kamae_modes(&hdr, &nkeys);
    uint32_t r7 = r8 + nkeys * 4u;
    AGES_G(2) = hdr;
    AGES_G(5) = AGES_G(6) + 0x78Cu;
    for (uint32_t i = 0; i < 20u; i++)
        for (uint32_t k = 0; k < 3u; k++)
            if (colig_kamae_one(ages_ld8(AGES_G(5)), &r7)) break;
    AGES_G(3) -= 0xF0u;
    for (uint32_t i = 0; i < 36u; i++) {
        ages_st16(AGES_G(3), gems_cvtri(ages_ld32(AGES_G(3))));
        AGES_G(3) += 4u;
    }
    AGES_AC = (AGES_AC & ~7u) | 2u;   /* cmpdeco 1, 1 */
    if (entry) ages_i960_ret();
    return 0;
}
