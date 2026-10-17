/* sound_chainlink_hit: i960 0x424E0, PS2 0x1F35C0.
 * Request the chain-link hit sound: the stage's entry (0x64256F4 by the
 * byte at 0x6425700+stage), or g7+0xACC, or the other fighter's g8+0xACC
 * when set; then check_same_sound and sound_request_u. */
#pragma once

static uint32_t afn_sound_chainlink_hit(int entry) {
    uint32_t k = ages_ld8(0x6425700u + ages_ld8(0x500064u));
    uint32_t s = ages_ld32(0x64256F4u + k * 4u);
    uint32_t own = ages_ld32(AGES_G(7) + 0xACC), other = ages_ld32(AGES_G(8) + 0xACC);
    if (own != 0) s = own;
    else if (other != 0) s = other;
    AGES_G(0) = s;
    afn_check_same_sound(0);
    afn_sound_request_u(0);
    if (entry) ages_i960_ret();
    return 0;
}
