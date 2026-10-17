/* get_frame_dat: i960 0x2AF28, PS2 0x1E61C0. Evaluates the fighter's motion
 * at its frame g7+0x1AA (f-curves, the 0x854 frame remap, frame 1's start
 * values), blends in the previous motion's offsets or the turn-round
 * (g7+0xBDD), keeps the feet above the floor, copies the pose into the rob
 * and turns the head (COP 0x63). Unlike the PS2: frame 1 checks kind 24,
 * the look flag is g7+0x7D7, and only the g8+0x20C look remains. */
#pragma once

static void m1_gfd_cc(uint32_t cc)
{
    AGES_AC = (AGES_AC & ~7u) | cc;
}

/* The 0x854 remap: pairs (frame, mapped) searched from a zero "previous"
 * (the native call's zeroed r4/r7). Returns 1 if the frame is a key. */
static int m1_gfd_remap(uint32_t p)
{
    uint32_t key = 0, val = 0, pk, pv = 0;
    do {
        pk = key;
        key = ages_ld8(p);
        if (key == AGES_G(6))
            return 1;
        pv = val;
        val = ages_ld8(p + 1);
        p += 2;
    } while (key < AGES_G(6));
    uint32_t num = (AGES_G(6) - pk) * (val - pv) + (key - pk) * pv;
    float fn = (float)(int32_t)num, fd = (float)(int32_t)(key - pk);
    AGES_G(6) = ages_f2u(fd == 0.0f ? 0.0f : fn / fd);
    afn_get_fcurve_value_f(0);
    return 0;
}

static void m1_gfd_curves(uint32_t rob)
{
    if ((ages_ld32(0x500068) & 0x20000u) && (ages_ld16(0x500092) & 1u) &&
        AGES_G(6) != ages_ld16(rob + 0x800)) {
        AGES_G(6) = ages_f2u((float)(int32_t)AGES_G(6) + 0.5f);
        afn_get_fcurve_value_f(0);
        return;
    }
    uint32_t p = ages_ld32(rob + 0x854);
    if (p != 0 && !m1_gfd_remap(p))
        return;
    if (AGES_G(6) == 1 && ages_ld8(rob + 0x19F) != 24) {
        if (!(ages_ld8(rob + 0xBDD) & 1u))
            afn_get_start_value(0);
    } else {
        m1_fcurve_int();
    }
}

/* g7+0xBE8[n] = g6 * src[n] (36 halfwords), then motion+0x720 += k*src. */
static void m1_gfd_blend(uint32_t rob, uint32_t src, uint32_t motion)
{
    for (uint32_t dst = rob + 0xBE8, n = 0; n < 36; n++, src += 4, dst += 2)
        ages_st16(dst, AGES_G(6) * (uint32_t)ages_ld16s(src));
    ages_cop_w(0x0B801717);
    ages_cop_w(AGES_G(6));
    float k = ages_u2f(ages_cop_r());
    for (uint32_t dst = motion + 0x720, n = 0; n < 24; n++, src += 4, dst += 4)
        ages_st32(dst, ages_f2u(ages_u2f(ages_ld32(src)) * k +
                                ages_u2f(ages_ld32(dst))));
}

/* The turn-round, or 0 if the blend is to be cleared. */
static int m1_gfd_turn(uint32_t rob, uint32_t motion, uint32_t mode)
{
    uint32_t frame = ages_ld16(rob + 0x1AA);
    if (mode == 1)
        return 0;
    uint32_t start = ages_ld16(rob + 0xBE0);
    if (start >= frame)
        return 0;
    uint32_t st = ages_ld8(rob + 0xBE5);
    if (!(st & 1u)) {
        afn_rear_smooth_int(0);
        st |= 1u;
    }
    AGES_G(6) = frame - start;
    if (ages_ld16(rob + 0xBDE) - 1u == AGES_G(6))
        st |= 2u;
    ages_st8(rob + 0xBE5, st);
    m1_gfd_blend(rob, motion + 0xF0, motion);
    return 1;
}

static void m1_gfd_offsets(uint32_t rob, uint32_t motion)
{
    AGES_G(6) = ages_ld16(rob + 0x1AA);
    if (ages_ld32(rob + 0x804) & 0x20010u) {
        AGES_G(6) = ages_ld16(rob + 0xBE2) + 1u;
        ages_st16(rob + 0xBE2, AGES_G(6));
    }
    uint32_t mode = ages_ld8(rob + 0xBDD);
    if (mode != 0) {
        if (mode != 2) {
            uint32_t len = ages_ld16(rob + 0xBDE);
            if (len > AGES_G(6)) {
                AGES_G(6) = len - AGES_G(6);
                m1_gfd_blend(rob, motion, motion);
                return;
            }
        }
        if (m1_gfd_turn(rob, motion, mode))
            return;
    }
    static const uint32_t zero[18];
    ages_stn(rob + 0xBE8, zero, 18);
}

static void m1_gfd_feet(uint32_t rob, uint32_t motion)
{
    float dy = ages_u2f(ages_ld32(rob + 0x1C)) - ages_u2f(ages_ld32(motion + 0x724));
    float ax = ages_u2f(ages_ld32(rob + 0x18) & 0x7FFFFFFFu);
    float az = ages_u2f(ages_ld32(rob + 0x20) & 0x7FFFFFFFu);
    float lim = ages_u2f(ages_ld32(0x50A00C));
    float fl = ages_u2f(ages_ld32(0x50A010));
    if (!(ax >= lim) && !(az >= lim))
        fl = 0.0f;
    float lo = fl + ages_u2f(0x3DCCCCCDu);
    for (uint32_t p = motion + 0x730, n = 0; n < 7; n++, p += 12) {
        float v = ages_u2f(ages_ld32(p)), y = v + dy;
        if (!(lo <= y))
            ages_st32(p, ages_f2u((lo - y) + v));
    }
}

static uint32_t m1_gfd_st3h(uint32_t dst, uint32_t src)
{
    for (int i = 0; i < 3; i++)
        ages_st16(dst + 2u * i, ages_ld32(src + 4u * i));
    return src + 12;
}

static void m1_gfd_copy(uint32_t rob, uint32_t src)
{
    uint32_t t[3];
    for (uint32_t i = 0; i < 4; i++)
        src = m1_gfd_st3h(rob + ages_ld32(0x2C208u + i * 4u), src);
    src = m1_gfd_st3h(rob + 0x140, src);
    for (AGES_G(5) = rob + 0xBA8; AGES_G(5) != rob + 0xBD2; AGES_G(5) += 6)
        src = m1_gfd_st3h(AGES_G(5), src);
    ages_ldn(t, src, 3);
    ages_stn(rob + 0x80, t, 3);
    src += 12;
    for (AGES_G(5) = rob + 0xB00; AGES_G(5) != rob + 0xB54; AGES_G(5) += 12, src += 12) {
        ages_ldn(t, src, 3);
        ages_stn(AGES_G(5), t, 3);
    }
}

/* The head looks at g8+0x20C unless something forbids it; sets the CC
 * of the test that ended it. */
static void m1_gfd_look(uint32_t rob)
{
    ages_st8(rob + 0x7D7, 0);
    uint32_t v = ages_ld32(0x500034), f = ages_ld32(rob);
    if (ages_ld8(0x50002C) & 0x80u) { m1_gfd_cc(2); return; }
    if (0x80u >= v) { m1_gfd_cc(0x80u > v ? 1u : 2u); return; }
    if ((v & 0x3F0000u) || (f & 0xA00000u)) { m1_gfd_cc(4); return; }
    if (f & 0x100u) { m1_gfd_cc(2); return; }
    if (ages_ld32(rob + 0x1A4) & 0x6C119u) { m1_gfd_cc(4); return; }
    m1_gfd_cc(2);
    ages_st8(rob + 0x7D7, 1);
    uint32_t q[4], t[3];
    ages_cop_w(0x31806363);
    ages_ldn(q, rob + 0x18, 3);
    q[3] = (uint32_t)ages_ld16s(rob + 0x26);
    gems_cop_wn(q, 4);
    ages_ldn(t, AGES_G(8) + 0x20C, 3);
    gems_cop_wn(t, 3);
    gems_cop_rn(t, 3);
    ages_stn(rob + 0xB0C, t, 3);
}

static uint32_t afn_get_frame_dat(int entry)
{
    uint32_t rob = AGES_G(7);
    AGES_G(6) = ages_ld16(rob + 0x1AA);
    uint32_t motion = ages_ld32(rob + 0xBD8);
    AGES_G(5) = motion + 0x690;
    m1_gfd_curves(rob);
    m1_gfd_offsets(rob, motion);
    if (ages_ld32(rob) & 4u)
        m1_gfd_feet(rob, motion);
    m1_gfd_copy(rob, motion + 0x690);
    m1_gfd_look(rob);
    if (entry) ages_i960_ret();
    return 0;
}
