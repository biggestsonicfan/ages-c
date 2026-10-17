/* stage_contact_sound: i960 0x41F68, PS2 0x1F3B00.
 * Request contact sound g0 for the stage (table 0x64256C0, column from the
 * stage byte table 0x6CE105C; column 0 while g7+0x624 is set): note it in
 * the frame's list (check_same_sound) and request it (sound_request_u). */
#pragma once

static uint32_t afn_stage_contact_sound(int entry) {
    uint32_t g7 = AGES_G(7);
    uint32_t tab = ages_ld32(0x64256C0u + AGES_G(0) * 4u);
    uint32_t col = ages_ld8(0x6CE105Cu + (ages_ld8(0x500064u) << 8));
    if (ages_ld16(g7 + 0x624) != 0) col = 0;
    AGES_G(0) = ages_ld32(tab + col * 4u);
    afn_check_same_sound(0);
    afn_sound_request_u(0);
    if (entry) ages_i960_ret();
    return 0;
}
