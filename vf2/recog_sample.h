/* VF2 recog_sample: i960 0x52398 (PS2 sysCopPatchRecogSample 0x186480,
 * EXT 54; decomp/VF2/FN/ext54_*), inside 0x52378, the command recogniser:
 * from sample r7 (g13+0x4C) of the move table at g4 (0x26-byte samples, the
 * count in byte 0), the longest run (r9, at most 5) of the sample's codes
 * (0x88) matching the input ring at r6+0x7C from r4 (32 entries, r5 the
 * head). The best sample goes to g13+0x5C, its run to 0x4E (g1 the side).
 *
 * Three exits, every local as the i960 leaves it there:
 * - the ring caught up with the head mid-run: 0x524D8 (PS2 IP 0xA49B0);
 * - the samples ran out and none matched (0x5C still 0xFF = r12): 0x52488
 *   (PS2 IP 0xA4910);
 * - the samples ran out with a match: 0x5243C, after `be` (PS2 0x29). */
#pragma once
#include "set_obj.h"

#define VF2_RS_SITE 0x00052398u

/* 0x523C0..0x523F8: the run from code r8 of sample r3 against the ring;
 * true when the ring reaches its head r5 (the `be 0x524d8`). */
static bool vf2_rs_run(uint32_t *r)
{
    for (;;) {
        r[10] = ages_ld16(r[3] + 0x88u + r[8] * 2u);
        r[11] = ages_ld16(r[6] + 0x7Cu + r[4] * 2u);
        if (r[10] != r[11]) return false;
        uint32_t old = r[4]++;
        r[8]++;
        if (!(0x1Fu > old)) r[4] = 0;
        if (5u <= r[8]) return false;
        if (r[5] == r[4]) return true;
    }
}

static uint32_t afn_vf2_recog_sample(int entry)
{
    (void)entry;
    uint32_t *r = &AGES_R(0), side = AGES_G(13) + AGES_G(1);
    r[7] = ages_ld8(side + 0x4Cu);
    ages_st8(side + 0x4Eu, r[9]);
    r[14] = ages_ld8(AGES_G(4));
    r[15] = 0x26u;
    while (r[14] > r[7]) {
        r[3] = r[7] * r[15] + AGES_G(4);
        if (vf2_rs_run(r)) {
            vf2_cmpo(r[5], r[4]);
            return (0x524D8u - VF2_RS_SITE) * 2u;
        }
        if (r[9] < r[8]) {
            r[9] = r[8];
            ages_st8(side + 0x4Eu, r[9]);
            ages_st8(side + 0x5Cu, r[7]);
        }
        r[7]++;
        r[4] = ages_ld8(side + 0x50u);
        r[8] = 0;
    }
    r[7] = ages_ld8(side + 0x5Cu);
    vf2_cmpo(r[12], r[7]);
    if (r[12] == r[7]) return (0x52488u - VF2_RS_SITE) * 2u;
    return 0x29u * 8u;                                 /* 0x5243C */
}
