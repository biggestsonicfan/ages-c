/* get_fcurve_value_f: i960 0x2B5A0, PS2 0x1E9580. Samples the 60 motion
 * channels at frame g6 (a float, its keys compared as unsigned words) into
 * the words at g5: linear keys through COP 0x31, spline keys through 0x32.
 * Then cvtri's the 36 angles to halfwords and mirrors if g7 bit 6. As the
 * PS2. 0x2B59C (m1_fcurve_int) first turns an integer g6 into a float. */
#pragma once

/* One keyed channel: prev is the i960's r7 (key time or spline tangent),
 * carried from channel to channel; the native call starts it at 0. */
static void m1_fcurve_keyed(uint32_t type, uint32_t *prev)
{
    uint32_t frame = AGES_G(6), times = AGES_G(3);
    uint32_t keys = ages_ld8(AGES_G(0));
    AGES_G(0)++;
    uint32_t stride = type == 5 ? 4u : 12u;
    uint32_t k = 0, key = 0;
    int found = 0;
    do {
        key = ages_ld32(times);
        times += 4;
        if (frame <= key) { found = 1; break; }
        k++;
        *prev = key;
    } while (keys > k);
    AGES_G(3) += keys * 4u;
    uint32_t p = AGES_G(2) + k * stride;
    AGES_G(2) += keys * stride;
    if (!found)
        return;
    if (frame == key) {
        ages_st32(AGES_G(5), ages_ld32(p));
        return;
    }
    float span = ages_u2f(key) - ages_u2f(*prev);
    float t = ages_u2f(frame) - ages_u2f(*prev);
    if (type == 5) {
        ages_cop_w(0x18803131u);
        ages_cop_w(ages_ld32(p - 4));
        ages_cop_w(ages_ld32(p));
        ages_cop_wf(t);
        ages_cop_wf(span);
    } else {
        *prev = ages_ld32(p - 4);
        ages_cop_w(0x19003232u);
        ages_cop_wf(span);
        ages_cop_wf(t);
        ages_cop_w(ages_ld32(p - 12));
        ages_cop_w(ages_ld32(p));
        ages_cop_w(*prev);
        ages_cop_w(ages_ld32(p + 4));
    }
    ages_st32(AGES_G(5), ages_cop_r());
}

static uint32_t afn_get_fcurve_value_f(int entry)
{
    uint32_t mot = ages_ld32(AGES_G(7) + 0xBD8u);
    AGES_G(4) = mot + 0x78Cu;
    AGES_G(3) = ages_ld32(mot + 0x788u);
    AGES_G(2) = ages_ld32(mot + 0x780u);
    AGES_G(0) = ages_ld32(mot + 0x784u);
    uint32_t prev = 0;
    for (AGES_G(1) = 60; AGES_G(1) != 0; AGES_G(1)--) {
        uint32_t type = ages_ld8(AGES_G(4));
        if (type == 4) {
            ages_st32(AGES_G(5), ages_ld32(AGES_G(2)));
            AGES_G(2) += 4;
        } else if (type == 3) {
            ages_st32(AGES_G(5), 0);
        } else if (type < 3) {
            AGES_G(2) += 4;
        } else {
            m1_fcurve_keyed(type, &prev);
        }
        AGES_G(4)++;
        AGES_G(5) += 4;
    }
    AGES_G(5) -= 240;
    for (int i = 0; i < 36; i++) {
        ages_st16(AGES_G(5), gems_cvtri(ages_ld32(AGES_G(5))));
        AGES_G(5) += 4;
    }
    AGES_G(5) -= 144;
    int mirror = (ages_ld32(AGES_G(7)) & 0x40u) != 0;
    AGES_AC = (AGES_AC & ~7u) | (mirror ? 2u : 0u);
    if (mirror)
        afn_set_mirror(0);
    if (entry) ages_i960_ret();
    return 0;
}

/* 0x2B59C: cvtir g6, then the above. */
static void m1_fcurve_int(void)
{
    AGES_G(6) = ages_f2u((float)(int32_t)AGES_G(6));
    afn_get_fcurve_value_f(0);
}
