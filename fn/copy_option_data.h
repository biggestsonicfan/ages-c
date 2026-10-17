/* copy_option_data: i960 0x1CB2C, PS2 0x1DB640.
 *
 * Copy the fighter's (g7) option record into g7+0x784..0x7B7 and g7+0x7F8.
 * FV's records are at [[g7+0x190]+0x2C], or +0x38 when the byte at
 * [0x50016C]+0x3350 is set; 0x70 bytes each, indexed by g7+0xAFC - 1, and
 * 0x38 more when bit 6 of g7+0 is set. As gems-c's (STF reads +0x18). */
#pragma once

static uint32_t afn_copy_option_data(int entry) {
    static const struct { uint8_t from; uint16_t to; uint8_t size; } fields[] = {
        { 0x00, 0x784, 4 }, { 0x04, 0x788, 2 }, { 0x06, 0x78A, 2 },
        { 0x08, 0x78C, 4 }, { 0x0C, 0x790, 4 }, { 0x10, 0x794, 4 },
        { 0x14, 0x798, 2 }, { 0x16, 0x79A, 2 }, { 0x18, 0x79C, 2 },
        { 0x1A, 0x79E, 2 }, { 0x1C, 0x7A0, 2 }, { 0x1E, 0x7A2, 2 },
        { 0x20, 0x7A4, 4 }, { 0x24, 0x7A8, 4 }, { 0x28, 0x7AC, 4 },
        { 0x2C, 0x7B0, 2 }, { 0x2E, 0x7B2, 2 }, { 0x30, 0x7B4, 2 },
        { 0x32, 0x7B6, 2 }, { 0x34, 0x7F8, 2 },
    };
    uint32_t g7 = AGES_G(7), tab = ages_ld32(g7 + 0x190u);
    uint32_t src = ages_ld32(tab + 0x2Cu);
    if (ages_ld8(ages_ld32(0x50016Cu) + 0x3350u) != 0) src = ages_ld32(tab + 0x38u);
    uint32_t option = ages_ld32(g7 + 0xAFCu);
    if (option != 0) src += (option - 1u) * 0x70u;
    int mirror = (ages_ld32(g7) & 0x40u) != 0;
    if (mirror) src += 0x38u;
    for (size_t i = 0; i < sizeof fields / sizeof fields[0]; i++) {
        if (fields[i].size == 4)
            ages_st32(g7 + fields[i].to, ages_ld32(src + fields[i].from));
        else
            ages_st16(g7 + fields[i].to, ages_ld16(src + fields[i].from));
    }
    AGES_AC = (AGES_AC & ~7u) | (mirror ? 0u : 2u);   /* bbc 6 */
    if (entry) ages_i960_ret();
    return 0;
}
