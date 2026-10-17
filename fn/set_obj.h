/* set_obj: i960 0x5A48, PS2 0x1CC280 (EXT 0d;
 * decomp/FV/FN/ext0d_set_obj_001cc280.c).
 *
 * Draw model g0 under the matrix the COP holds: unless the frame's polygon
 * count (0x50101C) is past the budget (0x501018), count the object
 * (0x501010), then COP 0x78 (Fn_set_obj) with the display-list pointer the
 * GEO RAM hands out (0x2008(g10), moved on 0x48 at 0x1008(g10)), g1, the
 * model's 16-byte entry in the table at 0x20E0004 and the two counters
 * (0x5010D0, 0x50101C), whose replies replace them. Unlike STF's, FV's i960
 * has no model check and no COP 0x13 here. The PS2 hands the model to its
 * own renderer; this is the i960's, write for write.
 *
 * The obj_* helpers are the code set_obj shares with set_obj_tpd, _thd and
 * _fifo (set_obj_common 0x5AE4, set_obj_go 0x5B04, 0x5C38). */
#pragma once

#define OBJ_SET_CC(cc) (AGES_AC = (AGES_AC & ~7u) | (uint32_t)(cc))
#define OBJ_MODELS 0x020E0004u   /* 16-byte entries */

/* i960 cmpo a,b: CC 100 if a < b, 010 if equal, 001 if a > b. */
static void obj_cmpo(uint32_t a, uint32_t b)
{
    OBJ_SET_CC(a < b ? 4 : a == b ? 2 : 1);
}

/* `ld 0x501018,r4; ld 0x50101c,r5; cmpobg r5,r4,<ret>`: false when the
 * frame's polygon count is already past the budget. */
static bool obj_budget(void)
{
    uint32_t budget = ages_ld32(0x501018);
    uint32_t count = ages_ld32(0x50101C);
    obj_cmpo(count, budget);
    return count <= budget;
}

/* A GEO command word: g14 parked at 0x50EFFC while it carries the word. */
static void obj_geo_cmd(uint32_t off, uint32_t word)
{
    ages_st32(0x50EFFC, AGES_G(14));
    AGES_G(14) = word;
    ages_st32(AGES_G(10) + off, AGES_G(14));
    AGES_G(14) = ages_ld32(0x50EFFC);
}

/* Count the object, point g0 at its model-table entry and load the entry. */
static void obj_take_entry(uint32_t *e)
{
    ages_st32(0x501010, ages_ld32(0x501010) + 1u);
    AGES_G(0) = OBJ_MODELS + AGES_G(0) * 16u;
    ages_ldn(e, AGES_G(0), 4);
}

/* set_obj_go (0x5B04): the entry's last word holds a texture offset (high
 * half) and the polygon count (low). With g1 set, the offset moves word 1 and
 * the count goes to 0x5010D0 too; the count goes to 0x50101C, and the entry,
 * last word -1, to the GEO as object command 0x101. */
static void obj_go(uint32_t *e)
{
    uint32_t tex = e[3] >> 16;
    e[3] &= 0xFFFFu;
    obj_cmpo(0, AGES_G(1));
    if (AGES_G(1) != 0) {
        e[1] += tex * 4u;
        ages_st32(0x5010D0, ages_ld32(0x5010D0) + e[3]);
    }
    ages_st32(0x50101C, ages_ld32(0x50101C) + e[3]);
    e[3] = 0xFFFFFFFFu;
    obj_geo_cmd(0x10, 0x101);
    ages_stn(AGES_G(10) + AGES_G(12), e, 4);
}

/* set_obj_common (0x5AE4): the model as it is in the table. */
static void obj_common(void)
{
    uint32_t e[4];
    obj_take_entry(e);
    obj_go(e);
}

/* 0x5C38: the model with entry word `word` replaced by g2. */
static void obj_replaced(int word)
{
    uint32_t e[4];
    obj_take_entry(e);
    e[word] = AGES_G(2);
    obj_go(e);
}

/* set_obj_tpd / _thd head: GEO command 0xB0B, then COP 0x34 (Fn_mov_matrix)
 * lays the current matrix into the display list where the GEO RAM's pointer
 * is, and the pointer moves on 0x30. */
static void obj_mov_matrix(void)
{
    obj_geo_cmd(0xB0, 0xB0B);
    ages_cop_w(0x1A003434u);
    uint32_t dl = ages_ld32(AGES_G(10) + 0x2008);
    ages_st32(AGES_G(10) + 0x1008, dl + 0x30u);
    ages_cop_w(dl);
    (void)ages_cop_r();
}

static uint32_t afn_set_obj(int entry)
{
    if (obj_budget()) {
        uint32_t e[4];
        ages_st32(0x50FFFC, AGES_G(0));      /* the last model drawn */
        ages_st32(0x501010, ages_ld32(0x501010) + 1u);
        AGES_G(0) = OBJ_MODELS + AGES_G(0) * 16u;
        ages_ldn(e, AGES_G(0), 4);
        ages_cop_w(0x3C007878u);             /* COP 0x78, Fn_set_obj */
        uint32_t dl = ages_ld32(AGES_G(10) + 0x2008);
        ages_st32(AGES_G(10) + 0x1008, dl + 0x48u);
        ages_cop_w(dl);
        ages_cop_w(AGES_G(1));
        gems_cop_wn(e, 4);
        ages_cop_w(ages_ld32(0x5010D0));
        ages_cop_w(ages_ld32(0x50101C));
        ages_st32(0x5010D0, ages_cop_r());
        ages_st32(0x50101C, ages_cop_r());
    }
    if (entry) ages_i960_ret();
    return 0;
}
