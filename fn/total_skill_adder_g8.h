/* total_skill_adder_g8: i960 0x2904C, PS2 0x1DF100. Add g9 to the skill score
 * at 0x50041C when the mode word 0x500088 has bit 0 and not bit 7 and the
 * side of g8 (byte +4: 0x500248 or 0x50024C) has bit 2. */
#pragma once

static uint32_t afn_total_skill_adder_g8(int entry) {
    uint32_t mode = (uint32_t)ages_ld16s(0x500088u);
    if ((mode & 1u) && !(mode & 0x80u)) {
        uint32_t side = ages_ld8(AGES_G(8) + 4u) ? ages_ld32(0x50024Cu) : ages_ld32(0x500248u);
        if (side & 4u)
            ages_st32(0x50041Cu, ages_ld32(0x50041Cu) + AGES_G(9));
    }
    if (entry) ages_i960_ret();
    return 0;
}
