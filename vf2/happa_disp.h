/* VF2 happa_disp: i960 0x2F134 to 0x2F2EC (PS2 sysCopPatchHappaDisp
 * 0x1866C0, EXT 53; decomp/VF2/FN/ext53_*). The loop over the 23 leaf
 * (happa) records of 44 bytes at r10: a record whose flags (byte 0x29)
 * meet r7 and whose cell mask (0x2A) has bit r9 goes to the TGP (0x01,
 * 0x6F, the 0x6D/0x6E/0x6C colours through the ring at 0x90E000, 0x13 x3,
 * 0x07) and draws its animation frame's model (set_obj, 0x7C60), then 0x02.
 *
 * The C stops before `ld -0x4(sp),g9`, every local as the i960 leaves it:
 * r8 0, r11 0xFD, CC equal from the last `cmpdeci`. */
#pragma once
#include "set_obj.h"

#define VF2_HD_RING 0x0090E000u   /* the colour ring; its byte index at 0x5001E4 */

/* `ld 0x5001e4,r15; st v,0x90e000(r15); addo 4,r15,r15; stob r15,0x5001e4`. */
static void vf2_hd_ring_put(uint32_t *r, uint32_t v)
{
    r[15] = ages_ld32(0x005001E4u);
    ages_st32(VF2_HD_RING + r[15], v);
    r[15] += 4u;
    ages_st8(0x005001E4u, r[15]);
}

/* 0x2F160..0x2F244: the record's position, then its colours. */
static void vf2_hd_colours(uint32_t *r, uint32_t rec)
{
    ages_ldn(&r[4], rec + 0x4u, 3);
    r[15] = ages_ld32(0x005001E4u);
    for (int i = 0; i < 3; i++) {
        if (i) r[15] = (r[15] + 4u) & ~0x100u;
        ages_st32(VF2_HD_RING + r[15], r[4 + i]);
    }
    r[15] += 4u;
    ages_st8(0x005001E4u, r[15]);
    vf2_tgp_w(r[15] = 0x37806F6Fu);
    ages_ldn(&r[4], rec + 0x1Cu, 2);
    r[6] = r[4] >> 16;
    r[4] &= AGES_G(9);
    r[5] &= AGES_G(9);
    static const uint32_t cmd[3] = { 0x36806D6Du, 0x37006E6Eu, 0x36006C6Cu };
    for (int i = 0; i < 3; i++) {
        uint32_t v = r[6 - i];
        if (v == 0) continue;
        vf2_hd_ring_put(r, v);
        vf2_tgp_w(r[15] = cmd[i]);
    }
}

/* 0x2F248..0x2F2DC: sync, the record's matrix row (0x07), its frame's model. */
static void vf2_hd_model(uint32_t *r, uint32_t rec)
{
    r[15] = 0x09801313u;
    for (int i = 0; i < 3; i++) vf2_tgp_w(r[15]);
    r[15] = vf2_tgp_r();
    ages_ldn(&r[4], rec + 0x10u, 3);
    vf2_tgp_w(r[15] = 0x03800707u);
    for (int i = 4; i <= 6; i++) vf2_tgp_w(r[i]);
    r[4] = ages_ld8(rec + 0x28u);
    r[5] = r[4] < 32u ? r[3] >> r[4] : 0;          /* shro */
    r[4] = ages_ld8(rec + 0x23u);
    r[5] += r[4];
    r[4] = ages_ld8(rec + 0x22u);
    r[5] &= r[4];
    r[4] = ages_ld32(rec + 0x24u);
    r[5] = ages_ld8(r[4] + r[5]);
    r[4] = ages_ld32(rec);
    r[4] = ages_ld16(r[4] + r[5] * 2u);
    r[15] = 0xFFFFu;
    if (r[4] != r[15]) {
        AGES_G(1) = 0;
        AGES_G(0) = r[4];
        (void)afn_vf2_set_obj(0);                  /* `call 0x7c60` */
    }
    vf2_tgp_w(r[15] = 0x01000202u);
}

static uint32_t afn_vf2_happa_disp(int entry)
{
    (void)entry;
    uint32_t *r = &AGES_R(0);
    r[11] = 0;
    uint32_t old;
    do {
        uint32_t rec = r[10] + r[11] * 4u;
        r[15] = ages_ld8(rec + 0x29u) & r[7];
        if (r[15] != 0) {
            r[15] = ages_ld16(rec + 0x2Au);
            if ((r[15] >> (r[9] & 31u)) & 1u) {
                vf2_tgp_w(r[15] = 0x00800101u);
                vf2_hd_colours(r, rec);
                vf2_hd_model(r, rec);
            }
        }
        r[11] += 0xBu;
        old = r[8]--;
    } while ((int32_t)old > 1);                        /* cmpdeci 1,r8; bl */
    AGES_AC = (AGES_AC & ~7u) | ((int32_t)old < 1 ? 1u : 2u);
    return 0x1B8u * 2u;                                /* 0x2F2EC (EXT 53: 0x6e) */
}
