/* area_clip: i960 0x23868, PS2 0x1F7F80 (EXT 69;
 * decomp/FV/FN/ext69_area_clip_001f7f80.c).
 *
 * Draws each ground chunk of the list at g2 (a halfword last index, then
 * four corner offsets per chunk) whose corners' outcodes at 0x50E000
 * (clip_point_check_yoko's) AND to zero, with the model numbers at g3 (0:
 * none). Leaves g2 past the list. Unlike STF's, no chunk under the camera. */
#pragma once

static uint32_t afn_area_clip(int entry) {
    const uint32_t oc = 0x50E000u;
    uint32_t last = ages_ld16(AGES_G(2)), i = 0, old;
    AGES_G(2) += 2;
    do {
        uint32_t p = AGES_G(2);
        uint32_t code = ages_ld16(oc + ages_ld16(p));
        code &= ages_ld16(oc + ages_ld16(p + 2));
        code &= ages_ld8(oc + ages_ld16(p + 4));
        code &= ages_ld8(oc + ages_ld16(p + 6));
        AGES_G(2) += 8;
        if (code == 0) {
            uint32_t model = ages_ld16(AGES_G(3) + i * 2);
            if (model != 0) {
                AGES_G(1) = 0;
                AGES_G(0) = model;
                afn_set_obj(0);
            }
        }
        old = i++;                     /* cmpinco r5,r6,r6; bg */
    } while (last > old);
    AGES_AC = (AGES_AC & ~7u) | (last == old ? 2u : 4u);
    if (entry) ages_i960_ret();
    return 0;
}
