/* clip_point_check_yoko: i960 0x236BC, PS2 0x1F8280 (EXT 68;
 * decomp/FV/FN/ext68_clip_point_check_yoko_001f8280.c).
 *
 * Outcodes for a point list: g1 points at a count, then (x, z) pairs. Each
 * goes through COP 0x29 (y = 0) and gets one byte at 0x50E000 on: 0x90 when
 * w is not past 1, else 0x81 when x is not right of -248, | 0x82 when x is
 * not left of +248 (x in screen units, scaled by 0.6667 of the projection at
 * 0x501084). area_clip ANDs them. g1 ends past the list, g5 one below 1 or 0.
 * The PS2 works out y as well; the board's divr into r5 is dropped. */
#pragma once

static uint32_t afn_clip_point_check_yoko(int entry) {
    const float k = ages_u2f(0x3F2AACDAu);   /* 0.6667 */
    float sx = k * ages_ldf(0x501084u);
    uint32_t out = 0x50E000u, n;
    AGES_G(5) = ages_ld32(AGES_G(1));
    AGES_G(1) += 4;
    do {
        uint32_t pt[2];
        ages_ldn(pt, AGES_G(1), 2);
        AGES_G(1) += 8;
        ages_cop_w(0x14802929u);
        ages_cop_w(pt[0]);
        ages_cop_w(0);
        ages_cop_w(pt[1]);
        float cx = ages_cop_rf();
        (void)ages_cop_r();
        float w = ages_cop_rf();
        float x = (cx * sx) / w;
        uint32_t code = 0x90;
        if (w > 1.0f) {                       /* cmpr; bg: a NaN is not */
            code = 0;
            if (!(x > -248.0f)) code |= 0x81;
            if (!(x < 248.0f)) code |= 0x82;
        }
        ages_st8(out++, code);
        n = AGES_G(5);                        /* cmpdeco 1,g5,g5; bl */
        AGES_G(5) = n - 1;
    } while (1u < n);
    AGES_AC = (AGES_AC & ~7u) | (n == 1u ? 2u : 1u);
    if (entry) ages_i960_ret();
    return 0;
}
