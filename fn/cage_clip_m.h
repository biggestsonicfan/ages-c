/* cage_clip_m: i960 0x22398, PS2 0x1F7640 (EXT 6a;
 * decomp/FV/FN/ext6a_cage_clip_m_001f7640.c).
 *
 * The cage's fence posts. Pushes the matrix, moves it along z by the side's
 * offset (g4 picks the pair at 0x6CE2048), draws the stage's cage top when
 * flag bit 20 is set, then projects each post of the stage's post list
 * through COP 0x29 and draws the fence segment between two posts (model
 * numbers from g2, the stage's list, or boss_cage's on stage 7) unless both
 * are off the same side. Pops the matrix. The PS2 keeps the posts' y, which
 * the board works out and drops. */
#pragma once

#include "boss_cage.h"

#define CCM_SIX 0x40C00000u   /* 6.0 */

static void ccm_translate(uint32_t x, uint32_t y, uint32_t z) {
    ages_cop_w(0x03000606u);
    ages_cop_w(x);
    ages_cop_w(y);
    ages_cop_w(z);
}

/* translate (0, 0, 6), set_obj(model), translate (-0, -0, -6) or (0, 0, -6) */
static void ccm_draw(uint32_t model, uint32_t back) {
    ccm_translate(0, 0, CCM_SIX);
    AGES_G(1) = 0;
    AGES_G(0) = model;
    afn_set_obj(0);
    ccm_translate(back, back, CCM_SIX ^ 0x80000000u);
}

/* One post: 31 when w is not past 2, else bit 0 when x is not right of
 * -248, bit 1 when not left of +248. */
static uint32_t ccm_outcode(float sx) {
    uint32_t px = ages_ld32(AGES_G(3));
    AGES_G(3) += 4;
    ages_cop_w(0x14802929u);
    ages_cop_w(px);
    ages_cop_w(0);
    ages_cop_w(CCM_SIX);
    float x = ages_cop_rf();
    (void)ages_cop_r();
    float w = ages_cop_rf();
    x = (x * sx) / w;
    if (!(w > 2.0f)) return 31;
    uint32_t code = 0;
    if (!(x > -248.0f)) code |= 1;
    if (!(x < 248.0f)) code |= 2;
    return code;
}

/* The post list and its length from the stage's flags. */
static uint32_t ccm_posts(uint32_t flags) {
    uint32_t n = 8;
    AGES_G(3) = 0x22958u;
    if (flags & (1u << 19)) { AGES_G(3) = 0x2297Cu; n = 4; }
    if (flags & (1u << 21)) { AGES_G(3) = 0x22990u; n = 2; }
    if (flags & (1u << 22)) { AGES_G(3) = 0x2299Cu; n = 1; }
    return n;
}

static uint32_t afn_cage_clip_m(int entry) {
    uint32_t side[2];
    ages_cop_w(0x00800101u);
    ages_ldn(side, 0x6CE2048u + AGES_G(4) * 8u, 2);
    ccm_translate(0, 0, side[0]);
    uint32_t stage = ages_ld8(0x500064u);
    uint32_t rec = stage << 8;
    AGES_G(2) = 0x6CE10CCu + rec + side[1] * 16u;
    uint32_t flags = ages_ld32(0x6CE1048u + rec);
    uint32_t top = ages_ld16(0x6CE1068u + rec);
    if (stage == 7) afn_boss_cage(0);
    if (flags & (1u << 20)) ccm_draw(top, 0x80000000u);

    float sx = ages_ldf(0x501084u);
    uint32_t n = ccm_posts(flags), old;
    uint32_t prev = ccm_outcode(sx);
    do {
        uint32_t cur = ccm_outcode(sx);
        if ((prev & cur) == 0) ccm_draw(ages_ld16(AGES_G(2)), 0);
        prev = cur;
        AGES_G(2) += 2;
        old = n--;                     /* cmpdeco 1,r11,r11; bl */
    } while (1u < old);
    AGES_AC = (AGES_AC & ~7u) | (old == 1u ? 2u : 1u);
    ages_cop_w(0x01000202u);
    if (entry) ages_i960_ret();
    return 0;
}
