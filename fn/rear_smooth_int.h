/* rear_smooth_int: i960 0x2A53C, PS2 0x1E7640. Starts the smoothing of a
 * turn-round: the motion's end pose to 0x50E0F0, the rear pose (by g7+0x814)
 * to 0x50E000, both rebased, mirrored and turned as the fighter is; the
 * per-part steps go to motion+0xF0. Unlike the PS2, no
 * unit_smooth_cancel_rear. g7+0xBE4 == 1 only clears the state. */
#pragma once

/* 8 entries of 12 bytes: x -= x0, z -= z0 through COP 0x14, unless both 0. */
static void m1_rear_rebase(uint32_t base)
{
    uint32_t x0 = ages_ld32(base), z0 = ages_ld32(base + 8);
    if ((x0 | z0) == 0)
        return;
    for (uint32_t p = base, n = 0; n < 8; n++, p += 12) {
        ages_cop_w(0x0A001414);
        ages_cop_w(ages_ld32(p));
        ages_cop_w(x0);
        ages_st32(p, ages_cop_r());
        ages_cop_w(0x0A001414);
        ages_cop_w(ages_ld32(p + 8));
        ages_cop_w(z0);
        ages_st32(p + 8, ages_cop_r());
    }
}

/* The rear pose by the kind at g7+0x814 (g0 left past it); 0 if none. */
static int m1_rear_pose(uint32_t motion)
{
    switch (ages_ld8(AGES_G(7) + 0x814)) {
    case 0x00: AGES_G(0) = motion + 0x1E0; break;
    case 0x80: AGES_G(0) = motion + 0x2D0; break;
    case 0x03: AGES_G(0) = motion + 0x3C0; break;
    case 0x04: AGES_G(0) = motion + 0x5A0; break;
    case 0x83: AGES_G(0) = motion + 0x4B0; break;
    default: return 0;
    }
    for (uint32_t dst = 0x50E000, n = 0; n < 20; n++, dst += 12) {
        uint32_t t[3];
        ages_ldn(t, AGES_G(0), 3);
        ages_stn(dst, t, 3);
        AGES_G(0) += 12;
    }
    return 1;
}

/* Turn the 8 translations at 0x50E090 and three angles by g7+0x812. */
static void m1_rear_turn(void)
{
    int32_t ang = ages_ld16s(AGES_G(7) + 0x812);
    ages_cop_w(0x00800101);
    ages_cop_w(0x01800303);
    ages_cop_w(0x04800909);
    ages_cop_w((uint32_t)ang);
    uint32_t p = 0x50E090;
    for (AGES_G(0) = 8; AGES_G(0) != 0; AGES_G(0)--, p += 12) {
        ages_cop_w(0x14802929);
        ages_cop_w(ages_ld32(p));
        ages_cop_w(0);
        ages_cop_w(ages_ld32(p + 8));
        uint32_t x = ages_cop_r();
        (void)ages_cop_r();
        uint32_t z = ages_cop_r();
        ages_st32(p, x);
        ages_st32(p + 8, z);
    }
    ages_cop_w(0x01000202);
    static const uint32_t a[3] = {0x50E034, 0x50E028, 0x50E01C};
    for (int i = 0; i < 3; i++)
        ages_st16(a[i], (uint32_t)ang + (uint32_t)ages_ld16s(a[i]));
}

/* One part's angle step (COP 0x55), shifted by sh; leaves g0..g2. */
static void m1_rear_ang(uint32_t base, uint32_t end, uint32_t rear,
                        uint32_t out, uint32_t sh)
{
    ages_cop_w(0x2A805555);
    AGES_G(0) = (uint32_t)ages_ld16s(base + 4);
    AGES_G(1) = (uint32_t)ages_ld16s(base);
    AGES_G(2) = (uint32_t)ages_ld16s(base + 2);
    gems_cop_wn(&AGES_G(0), 3);
    AGES_G(0) = 0u - (uint32_t)ages_ld16s(end);
    AGES_G(1) = 0u - (uint32_t)ages_ld16s(end + 4);
    AGES_G(2) = 0u - (uint32_t)ages_ld16s(end + 8);
    gems_cop_wn(&AGES_G(0), 3);
    AGES_G(0) = (uint32_t)ages_ld16s(rear + 8);
    AGES_G(1) = (uint32_t)ages_ld16s(rear + 4);
    AGES_G(2) = (uint32_t)ages_ld16s(rear);
    gems_cop_wn(&AGES_G(0), 3);
    AGES_G(0) = (uint32_t)(int16_t)ages_cop_r();
    AGES_G(1) = (uint32_t)(int16_t)ages_cop_r();
    AGES_G(2) = (uint32_t)(int16_t)ages_cop_r();
    AGES_G(0) = (uint32_t)((int32_t)(0u - AGES_G(0)) >> sh);
    ages_st16(out + 4, AGES_G(0));
    AGES_G(1) = (uint32_t)((int32_t)AGES_G(1) >> sh);
    ages_st16(out, AGES_G(1));
    AGES_G(2) = (uint32_t)((int32_t)(0u - AGES_G(2)) >> sh);
    ages_st16(out + 8, AGES_G(2));
}

/* The steps from end pose to rear pose into motion+0xF0. */
static void m1_rear_steps(uint32_t motion, uint32_t sh)
{
    uint32_t end = 0x50E0F0, rear = 0x50E000, out = motion + 0xF0;
    uint32_t base = AGES_G(7) + 0xBE8;
    ages_cop_w(0x00800101);
    AGES_G(4) = ages_ld32(AGES_G(7));
    AGES_G(3) = 0;
    for (int n = 0; n < 12; n++, out += 12, rear += 12, end += 12, base += 6)
        m1_rear_ang(base, end, rear, out, sh);
    ages_cop_w(0x0B801717);
    ages_cop_w(1u << sh);
    float inv = 1.0f / ages_u2f(ages_cop_r());
    for (int n = 0; n < 24; n++, out += 4, rear += 4, end += 4)
        ages_st32(out, ages_f2u(inv * (ages_u2f(ages_ld32(rear)) -
                                       ages_u2f(ages_ld32(end)))));
    ages_cop_w(0x01000202);
}

static uint32_t afn_rear_smooth_int(int entry)
{
    uint32_t rob = AGES_G(7);
    if (ages_ld8(rob + 0xBE4) == 1) {
        ages_st8(rob + 0xBE5, entry ? AGES_R(3) : 0);   /* the frame's r3 */
        ages_st8(rob + 0xBDD, 0);
        ages_st8(rob + 0xBE4, 0);
        AGES_AC = (AGES_AC & ~7u) | 2u;
        if (entry) ages_i960_ret();
        return 0;
    }
    uint32_t flags = ages_ld32(rob + 0x1A4);
    uint32_t motion = ages_ld32(rob + 0xBD8);
    uint32_t start = ages_ld16(rob + 0xBE0);
    AGES_G(5) = 0x50E0F0;
    afn_get_end_value(0);
    if (!m1_rear_pose(motion)) {
        ages_st8(rob + 0xBDD, ages_ld8(rob + 0xBDD) & ~2u);
        AGES_AC = (AGES_AC & ~7u) | (ages_ld8(rob + 0x814) > 0x83u ? 4u : 1u);
        if (entry) ages_i960_ret();
        return 0;
    }
    m1_rear_rebase(0x50E180);
    m1_rear_rebase(0x50E090);
    uint32_t mirror = ((ages_ld32(rob + 0x804) >> 21) ^ (ages_ld32(rob) >> 6)) & 1u;
    if (mirror) {
        AGES_G(5) = 0x50E000;
        afn_set_mirror(0);
    }
    if (flags & 0x40)
        m1_rear_turn();
    AGES_G(6) = ages_ld16(rob + 0x800);
    uint32_t span = AGES_G(6) - start;
    m1_rear_steps(motion, span == 8 ? 3 : span == 4 ? 2 : 1);
    AGES_AC = (AGES_AC & ~7u) | 2u;
    if (entry) ages_i960_ret();
    return 0;
}
