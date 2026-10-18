/* VF2 set_obj and its tails: i960 0x7C60 (called from 140 sites), 0x7CA4,
 * 0x7DBC. PS2 sysCopPatchSetObj 0x197800, sysCopSetObjCommon 0x197C00,
 * sysCopSetObjSame 0x197A80 (EXT 4c, 58, 59; decomp/VF2/FN/ext4c_*, ext58_*,
 * ext59_*).
 *
 * Draw model g0 under the TGP's current matrix: unless the frame's polygon
 * count (0x50101C) is past the budget (0x501018), TGP 0x1A003434 lays the
 * matrix into the display list at the pointer the GEO RAM hands out
 * (0x2008(g10), moved on 0x30 at 0x1008(g10)), then the model's 16-byte entry
 * in the table at 0x20E0004 goes to the GEO with its polygon count added to
 * 0x50101C (and 0x5010D0 with g1 set).
 *
 * VF2 is a Model 2A: the TGP is at g11 + g12 on the bus (ages.h). The PS2 runs
 * its own copExec there and hands the model to its renderer
 * (sysVf2GetObjAddr goes into the entry's last word); this is the i960's,
 * write for write, and the last word is -1 as the i960 writes it.
 *
 * EXT 4c is patched over each `call 0x7c60`, so the C does the call and
 * resumes after it. */
#pragma once

#define VF2_MODELS 0x020E0004u   /* 16-byte entries */

/* i960 cmpo a,b: CC 100 if a < b, 010 if equal, 001 if a > b. */
static void vf2_cmpo(uint32_t a, uint32_t b)
{
    AGES_AC = (AGES_AC & ~7u) | (a < b ? 4u : a == b ? 2u : 1u);
}

/* The TGP's FIFO, as `st rX,(g11)[g12*1]` and `ld (g11)[g12*1]`. */
static void vf2_tgp_w(uint32_t v)
{
    ages_st32(AGES_G(11) + AGES_G(12), v);
}

static uint32_t vf2_tgp_r(void)
{
    return ages_ld32(AGES_G(11) + AGES_G(12));
}

/* `ld 0x501018,r4; ld 0x50101c,r5; cmpobg r5,r4,<ret>`: false when the
 * frame's polygon count is already past the budget. */
static bool vf2_obj_budget(void)
{
    uint32_t budget = ages_ld32(0x501018);
    uint32_t count = ages_ld32(0x50101C);
    vf2_cmpo(count, budget);
    return count <= budget;
}

/* 0x7CA4: count the object, load its entry, add its polygon count, hand the
 * entry to the GEO (word 0 at 0x10(g10) too), last word -1. */
static void vf2_obj_common(void)
{
    uint32_t e[4];
    ages_st32(0x501010, ages_ld32(0x501010) + 1u);
    AGES_G(0) = VF2_MODELS + AGES_G(0) * 16u;
    ages_ldn(e, AGES_G(0), 4);
    uint32_t tex = e[3] >> 16;
    e[3] &= 0xFFFFu;
    vf2_cmpo(0, AGES_G(1));
    if (AGES_G(1) != 0) {
        e[1] += tex * 4u;
        ages_st32(0x5010D0, ages_ld32(0x5010D0) + e[3]);
    }
    ages_st32(0x50101C, ages_ld32(0x50101C) + e[3]);
    e[3] = 0xFFFFFFFFu;
    ages_st32(AGES_G(10) + 0x10, e[0]);
    ages_stn(AGES_G(10) + AGES_G(12), e, 4);
}

/* 0x7C60, from a `call 0x7c60` site: resume after the call. */
static uint32_t afn_vf2_set_obj(int entry)
{
    (void)entry;
    if (vf2_obj_budget()) {
        ages_st32(AGES_G(10) + 0xB0, 0);
        vf2_tgp_w(0x1A003434u);
        uint32_t dl = ages_ld32(AGES_G(10) + 0x2008);
        ages_st32(AGES_G(10) + 0x1008, dl + 0x30u);
        vf2_tgp_w(dl);
        (void)vf2_tgp_r();
        vf2_obj_common();
    }
    return 8;   /* site + 4: past the call */
}

/* `b 0x7ca4` at the end of 0x7D14 and 0x7D6C (set_obj from the TGP's or g2's
 * three quads): the common tail, then its `ret`. */
static uint32_t afn_vf2_set_obj_common(int entry)
{
    (void)entry;
    vf2_obj_common();
    ages_i960_ret();
    return 0;
}

/* 0x7DBC: `cmpobl r5,r4,0x7ca4; ret`, on the budget the caller loaded. */
static uint32_t afn_vf2_set_obj_same(int entry)
{
    (void)entry;
    uint32_t count = AGES_R(5), budget = AGES_R(4);
    vf2_cmpo(count, budget);
    if (count < budget) vf2_obj_common();
    ages_i960_ret();
    return 0;
}
