/* efc_face_change_cont: i960 0x2DDB0, PS2 0x1F34C0 (EXT 5b;
 * decomp/FV/FN/ext5b_efc_face_change_cont_001f34c0.c).
 *
 * g7: the fighter's work area. g0: two halfwords, the face to show without
 * and with bit 2 of the halfword at g7+0x1F48. The one picked goes, sign
 * extended, into word 2 of the face slots at g7+0x40 (g7+0x48).
 *
 * The board's own: ldos 0x1F48(g7); bbc 2; ldis (g0)[r4*2]; st 0x40(g7)[r3*4]
 * with r3 = 2. The PS2's C is the same; the locals it leaves (r3, r4, r15) are
 * the callee frame's, which the ret drops. */
static uint32_t afn_efc_face_change_cont(int entry) {
    uint32_t with = (ages_ld16s(AGES_G(7) + 0x1F48u) & 4u) != 0;
    ages_st32(AGES_G(7) + 0x40u + 2u * 4u, (uint32_t)ages_ld16s(AGES_G(0) + with * 2u));
    if (entry) ages_i960_ret();
    return 0;
}
