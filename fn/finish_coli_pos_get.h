/* finish_coli_pos_get: i960 0x7B73C, PS2 0x1FA6C0. A `balx ...,r6` leaf: the
 * x/z of the fighter that took the finishing blow (0x500804 when 0x574004 has
 * bit 30, else 0x500808): +0x20C/+0x214, or with bit 0 of +0x5B8 +0x284/+0x28C
 * (+0x2A8/+0x2B0 when bit 6 of +0). It writes the caller's r4/r5 (and r7, r15);
 * C callers use colig_finish_pos. */
#pragma once

static void colig_finish_pos(uint32_t *x, uint32_t *z) {
    uint32_t rob = (ages_ld32(0x574004u) & 0x40000000u) ? ages_ld32(0x500804u)
                                                         : ages_ld32(0x500808u);
    uint32_t off = 0x20Cu;
    if (ages_ld32(rob + 0x5B8u) & 1u)
        off = (ages_ld32(rob) & 0x40u) ? 0x2A8u : 0x284u;
    *x = ages_ld32(rob + off);
    *z = ages_ld32(rob + off + 8u);
}

static uint32_t afn_finish_coli_pos_get(int entry) {
    uint32_t x, z;
    colig_finish_pos(&x, &z);
    AGES_R(4) = x;
    AGES_R(5) = z;
    if (entry) ages_branch(AGES_R(6));
    return 0;
}
