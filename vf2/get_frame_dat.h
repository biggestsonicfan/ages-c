/* VF2 get_frame_dat: i960 0x28184, PS2 sysCopPatchGetFrameDat 0x190BC0
 * (EXT 50; decomp/VF2/FN/ext50_*).
 *
 * Evaluates the fighter's motion at its frame g7+0x1AA: the f-curves
 * (0x2891C) at the frame, a slowed frame (0x500068 bit 17 or 20) or the 0x854
 * frame remap, or frame 1's start values; blends in the previous motion's
 * offsets or the turn-round (g7+0xBDD, rear_smooth_int in vf2/smooth_int.h);
 * drifts one part by g7+0xC34 over frames 2..; keeps the feet above the
 * floor; copies the pose into the rob; turns the head to g8+0x20C, and for
 * motions 0xD8 and 0x519 an arm to a point beside the opponent.
 *
 * Like FV's (fn/get_frame_dat.h), on the TGP over the bus (vf2/set_obj.h);
 * the 0x500068 bit 20 frame scale, the g7+0xC34 drift and the 0xD8/0x519 look
 * are VF2's own, and frame 1 has no kind-24 test. get_start_value and
 * set_mirror are FV's (byte for byte the same code, 0x28780 and 0x28AF8).
 *
 * The PS2 returns 0x17E: the i960 resumes at 0x2877C, the function's `ret`,
 * so the C pops the frame itself and returns 0. */
#pragma once
#include "smooth_int.h"

/* One keyed channel of 0x2891C: prev is the i960's r7 (a key time, or a
 * spline's tangent), carried from channel to channel from 0. */
static void vf2_gfd_keyed(uint32_t type, uint32_t *prev)
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
    uint32_t span = mo2_subr(*prev, key), t = mo2_subr(*prev, frame);
    if (type == 5) {
        vf2_tgp_w(0x18803131u);
        vf2_tgp_w(ages_ld32(p - 4));
        vf2_tgp_w(ages_ld32(p));
        vf2_tgp_w(t);
        vf2_tgp_w(span);
    } else {
        vf2_tgp_w(0x19003232u);
        vf2_tgp_w(span);
        vf2_tgp_w(t);
        vf2_tgp_w(ages_ld32(p - 12));
        vf2_tgp_w(ages_ld32(p));
        *prev = ages_ld32(p - 4);
        vf2_tgp_w(*prev);
        vf2_tgp_w(ages_ld32(p + 4));
    }
    ages_st32(AGES_G(5), vf2_tgp_r());
}

/* 0x2891C: the 60 channels at frame g6 (a float) into the words at g5, the
 * first 36 then cvtri'd to halfwords, mirrored if g7 bit 6. */
static void vf2_gfd_fcurve(void)
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
            vf2_gfd_keyed(type, &prev);
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
    if (ages_ld32(AGES_G(7)) & 0x40u)
        afn_set_mirror(0);
}

/* The 0x854 remap: (frame, mapped) byte pairs, searched from a zero
 * "previous" pair (the call's zeroed r4/r7). 1 if the frame is a key. */
static int vf2_gfd_remap(uint32_t p)
{
    uint32_t key = 0, val = 0, pk, pv;
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
    AGES_G(6) = mo2_divr(mo2_cvtir(key - pk), mo2_cvtir(num));
    vf2_gfd_fcurve();
    return 0;
}

/* 0x28184..0x28270: the pose at the frame. */
static void vf2_gfd_curves(uint32_t rob)
{
    uint32_t speed = ages_ld32(0x500068);
    if ((speed & 0x20000u) && (ages_ld16(0x500092) & 1u) &&
        AGES_G(6) != ages_ld16(rob + 0x800)) {
        AGES_G(6) = mo2_addr(0x3F000000u, mo2_cvtir(AGES_G(6)));
        vf2_gfd_fcurve();
        return;
    }
    if (speed & 0x100000u) {
        if (AGES_G(6) != 1) {
            /* frames 1..n-1 spread over 1..n/2 */
            uint32_t n = ages_ld16(rob + 0x800);
            AGES_G(6) = (AGES_G(6) - 1u) * ((n >> 1) - 1u);
            AGES_G(6) = mo2_divr(mo2_cvtir(n - 1u), mo2_cvtir(AGES_G(6)));
            AGES_G(6) = mo2_addr(0x3F800000u, AGES_G(6));
            vf2_gfd_fcurve();
            return;
        }
    } else {
        uint32_t p = ages_ld32(rob + 0x854);
        if (p != 0 && !vf2_gfd_remap(p))
            return;
    }
    if (AGES_G(6) == 1) {
        if (!(ages_ld8(rob + 0xBDD) & 1u))
            afn_get_start_value(0);
    } else {
        AGES_G(6) = mo2_cvtir(AGES_G(6));   /* 0x28918 */
        vf2_gfd_fcurve();
    }
}

/* g7+0xBE8[n] = g6 * src[n] (36 halfwords), then motion+0x720 += k*src. */
static void vf2_gfd_blend(uint32_t rob, uint32_t src, uint32_t motion)
{
    for (uint32_t dst = rob + 0xBE8, n = 0; n < 36; n++, src += 4, dst += 2)
        ages_st16(dst, AGES_G(6) * (uint32_t)ages_ld16s(src));
    vf2_tgp_w(0x0B801717u);
    vf2_tgp_w(AGES_G(6));
    uint32_t k = vf2_tgp_r();
    for (uint32_t dst = motion + 0x720, n = 0; n < 24; n++, src += 4, dst += 4)
        ages_st32(dst, mo2_addr(ages_ld32(dst), mo2_mulr(k, ages_ld32(src))));
}

/* The turn-round (0x2831C), or 0 if the blend is to be cleared. */
static int vf2_gfd_turn(uint32_t rob, uint32_t motion, uint32_t mode)
{
    uint32_t frame = ages_ld16(rob + 0x1AA);
    if (mode == 1)
        return 0;
    uint32_t start = ages_ld16(rob + 0xBE0);
    if (start >= frame)
        return 0;
    uint32_t st = ages_ld8(rob + 0xBE5);
    if (!(st & 1u)) {
        vf2_si_rear();
        st |= 1u;
    }
    AGES_G(6) = frame - start;
    if (ages_ld16(rob + 0xBDE) - 1u == AGES_G(6))
        st |= 2u;
    ages_st8(rob + 0xBE5, st);
    vf2_gfd_blend(rob, motion + 0xF0, motion);
    return 1;
}

/* 0x28274..0x283E8. */
static void vf2_gfd_offsets(uint32_t rob, uint32_t motion)
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
                vf2_gfd_blend(rob, motion, motion);
                return;
            }
        }
        if (vf2_gfd_turn(rob, motion, mode))
            return;
    }
    static const uint32_t zero[18];
    ages_stn(rob + 0xBE8, zero, 18);
}

/* The part g7+0x820 drifts (0 none) and its step over the frames. */
static uint32_t vf2_gfd_drift_part(uint32_t code, uint32_t sub, uint32_t *step)
{
    static const uint8_t tab[][3] = {   /* code, code, part / 4 */
        {0x05, 0x1A, 0xB8 / 4}, {0x06, 0x1B, 0xC4 / 4},
        {0x09, 0x0E, 0xDC / 4}, {0x0A, 0x0F, 0xE8 / 4},
    };
    for (int i = 0; i < 4; i++) {
        if (code != tab[i][0] && code != tab[i][1])
            continue;
        *step = i < 2 ? 0xBDCCCCCDu : 0xBE4CCCCDu;   /* -0.1, -0.2 */
        return i >= 2 && sub == 2 ? 0 : tab[i][2] * 4u;
    }
    return 0;
}

/* Frame 2 (0x283F8): the drift of g7+0xC34 for the kinds g8+0x1B1 6, 8 and
 * 10 by g7+0x820/0x821, its per-frame steps to g7+0xC38 and 0xC3C. */
static void vf2_gfd_drift_start(uint32_t rob)
{
    uint32_t kind = ages_ld8(AGES_G(8) + 0x1B1), part = 0, step = 0;
    uint32_t sub = ages_ld8(rob + 0x821);
    if ((kind == 10 || kind == 6 || kind == 8) && (ages_ld32(rob + 0x804) & 0x100u) && sub < 4)
        part = vf2_gfd_drift_part(ages_ld8(rob + 0x820), sub, &step);
    ages_st32(rob + 0xC34, part);
    if (part == 0)
        return;
    uint32_t mid = ages_ld16(rob + 0x808), n = ages_ld16(rob + 0x800);
    ages_st32(rob + 0xC38, mo2_divr(mo2_cvtir(mid - 2u), step));
    ages_st32(rob + 0xC3C, mo2_divr(mo2_cvtir(n - mid), step));
}

/* Frames 3.. (0x284CC): the part's value moves on by a step a frame, up to
 * g7+0x808, then back, kept above 0.14. */
static void vf2_gfd_drift(uint32_t rob, uint32_t motion, uint32_t frame)
{
    uint32_t part = ages_ld32(rob + 0xC34);
    if (part == 0)
        return;
    uint32_t p = motion + part + 0x690, k;
    uint32_t mid = ages_ld16(rob + 0x808), n = ages_ld16(rob + 0x800);
    if (mid >= frame) {
        frame -= 2u;
        k = ages_ld32(rob + 0xC38);
    } else {
        frame = n - frame;
        k = ages_ld32(rob + 0xC3C);
    }
    uint32_t v = mo2_addr(mo2_mulr(mo2_cvtir(frame), k), ages_ld32(p));
    uint32_t cc = mo2_cmpr(0x3E0F5C29u, v);
    AGES_AC = (AGES_AC & ~7u) | cc;
    if (!(cc & 4u))
        v = 0x3E0F5C29u;
    ages_st32(p, v);
}

static void vf2_gfd_feet(uint32_t rob, uint32_t motion)
{
    uint32_t dy = mo2_subr(ages_ld32(motion + 0x724), ages_ld32(rob + 0x1C));
    int32_t ax = (int32_t)(ages_ld32(rob + 0x18) & 0x7FFFFFFFu);
    int32_t az = (int32_t)(ages_ld32(rob + 0x20) & 0x7FFFFFFFu);
    uint32_t fl = ages_ld32(0x50A010);
    int32_t lim = (int32_t)ages_ld32(0x50A00C);
    if (ax < lim && az < lim)
        fl = 0;
    uint32_t lo = mo2_addr(0x3DCCCCCDu, fl);
    for (uint32_t p = motion + 0x730, n = 0; n < 7; n++, p += 12) {
        uint32_t v = ages_ld32(p), y = mo2_addr(v, dy);
        if (!(mo2_cmpr(lo, y) & 6u))
            ages_st32(p, mo2_addr(v, mo2_subr(y, lo)));
    }
}

static uint32_t vf2_gfd_st3h(uint32_t dst, uint32_t src)
{
    for (uint32_t i = 0; i < 3; i++)
        ages_st16(dst + 2u * i, ages_ld32(src + 4u * i));
    return src + 12;
}

/* 0x2859C: the pose into the rob; leaves g5 at g7+0xB54. */
static void vf2_gfd_copy(uint32_t rob, uint32_t src)
{
    uint32_t t[3];
    for (uint32_t i = 0; i < 4; i++)
        src = vf2_gfd_st3h(rob + ages_ld32(0x29714u + i * 4u), src);
    src = vf2_gfd_st3h(rob + 0x140, src);
    for (AGES_G(5) = rob + 0xBA8; AGES_G(5) != rob + 0xBD2; AGES_G(5) += 6)
        src = vf2_gfd_st3h(AGES_G(5), src);
    ages_ldn(t, src, 3);
    ages_stn(rob + 0x80, t, 3);
    src += 12;
    for (AGES_G(5) = rob + 0xB00; AGES_G(5) != rob + 0xB54; AGES_G(5) += 12, src += 12) {
        ages_ldn(t, src, 3);
        ages_stn(AGES_G(5), t, 3);
    }
}

/* TGP 0x63: the rob's position and angle g7+0x26, a point, the turned
 * point to dst. */
static void vf2_gfd_aim(uint32_t rob, const uint32_t *pt, uint32_t dst)
{
    uint32_t q[4], t[3];
    vf2_tgp_w(0x31806363u);
    ages_ldn(q, rob + 0x18, 3);
    q[3] = (uint32_t)ages_ld16s(rob + 0x26);
    ages_stn(vf2_si_fifo(), q, 4);
    ages_stn(vf2_si_fifo(), pt, 3);
    ages_ldn(t, vf2_si_fifo(), 3);
    ages_stn(rob + dst, t, 3);
}

/* 0x28634: the head looks at g8+0x20C unless something forbids it. */
static void vf2_gfd_look(uint32_t rob)
{
    uint32_t v = ages_ld32(0x500034), pt[3];
    if (6u >= v || (v & 0x3F0000u) || (ages_ld32(rob) & 0xA00000u) ||
        (ages_ld32(rob + 0x1A4) & 0x6C111u))
        return;
    ages_ldn(pt, AGES_G(8) + 0x20C, 3);
    vf2_gfd_aim(rob, pt, 0xB0C);
}

/* 0x286A4: in motions 0xD8 and 0x519, frames 13..157, an arm aims at
 * (0.15, 0, -0.05) off the opponent (g8+4's matrix) to g7+0xB18, or 0xB24
 * mirrored. Sets the CC of the test that ended it. */
static void vf2_gfd_arm(uint32_t rob)
{
    uint32_t m = ages_ld16(rob + 0x1A8);
    vf2_cmpo(0xD8, m);
    if (m != 0xD8) {
        vf2_cmpo(0x519, m);
        if (m != 0x519)
            return;
    }
    uint32_t f = ages_ld16(rob + 0x1AA);
    vf2_cmpo(0xD, f);
    if (0xDu > f)
        return;
    vf2_cmpo(0x9D, f);
    if (0x9Du < f)
        return;
    static const uint32_t off[3] = {0x3E19999Au, 0, 0xBD4CCCCDu};
    uint32_t pt[3];
    vf2_tgp_w(0x00800101u);
    vf2_tgp_w(0x1B003636u);
    vf2_tgp_w(ages_ld8(AGES_G(8) + 4));
    vf2_tgp_w(0x18);
    vf2_tgp_w(0x14802929u);
    ages_stn(vf2_si_fifo(), off, 3);
    ages_ldn(pt, vf2_si_fifo(), 3);
    vf2_tgp_w(0x01000202u);
    int mirror = (ages_ld32(rob) & 0x40u) != 0;
    AGES_AC = (AGES_AC & ~7u) | (mirror ? 2u : 0u);
    vf2_gfd_aim(rob, pt, mirror ? 0xB24 : 0xB18);
}

static uint32_t afn_vf2_get_frame_dat(int entry)
{
    uint32_t rob = AGES_G(7);
    AGES_G(6) = ages_ld16(rob + 0x1AA);
    uint32_t motion = ages_ld32(rob + 0xBD8);
    AGES_G(5) = motion + 0x690;
    vf2_gfd_curves(rob);
    vf2_gfd_offsets(rob, motion);
    uint32_t frame = ages_ld16(rob + 0x1AA);
    if (frame == 2)
        vf2_gfd_drift_start(rob);
    else if (frame > 2)
        vf2_gfd_drift(rob, motion, frame);
    if (ages_ld32(rob) & 4u)
        vf2_gfd_feet(rob, motion);
    vf2_gfd_copy(rob, motion + 0x690);
    vf2_gfd_look(rob);
    vf2_gfd_arm(rob);
    if (entry) ages_i960_ret();
    return 0;
}
