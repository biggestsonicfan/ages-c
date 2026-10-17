/* rob_grass_disp_p768: i960 0x7789C, PS2 0x1CC380 (EXT 12;
 * decomp/FV/FN/ext12_rob_grass_disp_p768_001cc380.c).
 *
 * Not a function: 0x90 bytes in the middle of rob_grass_disp (its +768),
 * 0x7789C-0x77928. sysExtend's case 0x12 calls it with no argument; it pops
 * nothing and returns 0x120, how far the interpreter moves on in the decoded
 * program (8 bytes per i960 word): 36 words, so the i960 carries on at
 * 0x7792C, where the code it replaces joins. gems_run resumes at site + R/2.
 *
 * The PS2 puts its own host pointers in r12-r15/r13/r14 (texture and model
 * tables on the EE) and leaves out the two counters at 0x5010D0 and
 * 0x50101C; this follows the i960. The condition code is the last compare's
 * or bit test's, as the board's interpreter sets it (bbc: 2 when the bit is
 * clear, else 0). */
#pragma once

static uint32_t grass_cc(uint32_t cc)
{
    AGES_AC = (AGES_AC & ~7u) | cc;
    return cc;
}

/* cmpobl 4,r3: cc of 4 against r3, true when 4 < r3. */
static int grass_more_than_4(void)
{
    uint32_t r3 = AGES_R(3);
    return grass_cc(4u < r3 ? 4u : 4u == r3 ? 2u : 1u) == 4u;
}

/* bbc bit,r3: true when the bit is clear. */
static int grass_bit_clear(int bit)
{
    return grass_cc((AGES_R(3) >> bit) & 1u ? 0u : 2u) == 2u;
}

/* 0x7789C-0x778F4: the grass entry's quad, and the two polygon counters. */
static void grass_load(void)
{
    AGES_G(0) = 0x020E0004u + AGES_G(0) * 16u;
    for (int i = 0; i < 4; i++) AGES_R(12 + i) = ages_ld32(AGES_G(0) + 4u * i);
    AGES_R(3)  = AGES_R(15) >> 16;
    AGES_R(15) &= 0xFFFFu;
    AGES_R(14) = ages_ld8(AGES_G(7) + 4u);
    grass_cc(AGES_R(14) == 0u ? 2u : 4u);       /* cmpobe 0,r14 */
    if (AGES_R(14)) {
        AGES_R(13) += AGES_R(3) * 4u;
        AGES_R(4) = ages_ld32(0x005010D0u) + AGES_R(15);
        ages_st32(0x005010D0u, AGES_R(4));
    }
    AGES_R(14) = (AGES_R(14) << 8) + 4u;
    AGES_R(4) = ages_ld32(0x0050101Cu) + AGES_R(15);
    ages_st32(0x0050101Cu, AGES_R(4));
    AGES_R(15) = 0xFFFFFFFFu;
}

static uint32_t afn_rob_grass_disp_p768(int entry)
{
    (void)entry;
    grass_load();
    AGES_R(3) = ages_ld32(AGES_G(7) + 0x1A4u);
    if (!grass_bit_clear(26)) {
        AGES_R(3) = (uint32_t)ages_ld16s(AGES_G(7) + 0x1AAu);
        if (!grass_more_than_4()) {
            AGES_R(13) = 0x00800000u;
            return 0x120;
        }
    }
    AGES_R(3) = ages_ld32(AGES_G(7) + 0x720u);
    if (grass_bit_clear(2)) return 0x120;
    AGES_R(3) = (uint32_t)ages_ld16s(AGES_G(7) + 0x1AAu);
    if (grass_more_than_4() || grass_bit_clear(0)) return 0x120;
    AGES_R(13) = 0x00802000u;
    return 0x120;
}
