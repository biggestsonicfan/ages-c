/* send_tex_col_go: i960 0x818 (a label in send_tex_col_loop_stage2, 0x804),
 * PS2 0x1EA600 (EXT 85; decomp/FV/FN/ext85_send_tex_col_go_001ea600.c).
 *
 * Reached by a branch from send_tex_col_part (0x7F8), _loop_stage2 (0x804) or
 * _loop (0x810), which leave r9 (1 or 2: halfword or word-pair source) and
 * r11 (the source's stride per g1) in the frame. Copies g2+1 blocks of 16
 * colours from g3 + r11*g1 through the lumi tables at 0x544000/0x544200/
 * 0x544400 into the colour RAM at 0x1810060 + 0x200*g0 (R, G, B planes
 * 0x4000 apart). The board's loads are ldos: a word-pair source is
 * (signed + 0x80) >> 1 unsigned, as the i960's addi/shro. The PS2 reads
 * unsigned and from its own copy of the tables. */
#pragma once

/* The i960's cmpo condition code: 4 less, 2 equal, 1 greater (a against b). */
static inline uint32_t col_cmpo(uint32_t a, uint32_t b) {
    return a < b ? 4u : a == b ? 2u : 1u;
}
static inline void col_set_cc(uint32_t cc) { AGES_AC = (AGES_AC & ~7u) | cc; }

/* One colour of the 16: plane c's source, lumi table and colour RAM plane. */
static void col_tex_one(uint32_t src, uint32_t i, uint32_t c, int pair, uint32_t dst) {
    uint32_t v = (uint32_t)ages_ld16s(src + 0x20u * c + i * 2u);
    if (pair)
        v = ((uint32_t)ages_ld16s(src + 0x80u * c + i * 4u) + 0x80u) >> 1;
    ages_st16(dst + 0x10000u + 0x4000u * c, (uint32_t)ages_ld16s(0x544000u + 0x200u * c + v * 2u));
}

static uint32_t afn_send_tex_col_go(int entry) {
    int pair = AGES_R(9) == 2;
    uint32_t dst = 0x1800000u + 0x200u * AGES_G(0);
    uint32_t blk = 0, more;
    do {
        dst += 0x60u;
        uint32_t src = AGES_R(11) * AGES_G(1) + 0x60u * blk + AGES_G(3);
        for (uint32_t i = 0; i < 16; i++, dst += 2)
            for (uint32_t c = 0; c < 3; c++) col_tex_one(src, i, c, pair, dst);
        AGES_G(0) = 16;
        dst += 0x180u;
        more = AGES_G(2) > blk;
        col_set_cc(col_cmpo(AGES_G(2), blk));
        blk++;
    } while (more);
    if (entry) ages_i960_ret();
    return 0;
}
