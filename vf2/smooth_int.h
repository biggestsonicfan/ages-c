/* VF2 smooth_int and rear_smooth_int: i960 0x27130 and 0x2776C. PS2
 * sysCopPatchSmoothInt 0x18C040 (EXT 51; decomp/VF2/FN/ext51_*), its
 * rearSmoothInt 0x18F640 (decomp/VF2/FN/helpers/).
 *
 * smooth_int starts a motion blend: it picks the blend mode (g7+0xBDD) and
 * length (g7+0xBDE, 0xBE0), puts the start pose (get_start_value) at 0x50E000
 * and in the motion's place, the end pose at 0x50E0F0, rebases and turns the
 * keys, and lays the per-frame steps at the motion (g7+0xBD8). rear_smooth_int
 * (get_frame_dat's turn-round) does the same from the end pose to the rear
 * pose by g7+0x814, steps at motion+0xF0.
 *
 * The board code is FV's (fn/smooth_int.h, fn/rear_smooth_int.h) on the TGP
 * over the bus (vf2/set_obj.h), with these differences: an angle goes to the
 * TGP through the ring at 0x90E000 (index byte 0x5001E4) and a no-argument
 * rotate command, and only when it is not 0; 0x1F803F3F has no reply; the
 * angle step is one pass; rear_smooth_int halves g6 under 0x500068 bit 20.
 * get_start_value, get_end_value and set_mirror are byte for byte FV's
 * (0x28780, 0x28848, 0x28AF8), so their C is used, and so is
 * fn/set_mot_dat.h's i960 real ops (mo2_*).
 *
 * The PS2 returns 0x29 or 0x18E: both resume at a `ret` (0x271D4, 0x27768),
 * so the C pops the frame itself and returns 0. */
#pragma once
#include "../fn/set_mot_dat.h"
#include "set_obj.h"

#define VF2_SI_START 0x50E000u   /* start (or rear) pose, 20 triples */
#define VF2_SI_END   0x50E0F0u   /* end pose */

static uint32_t vf2_si_fifo(void)
{
    return AGES_G(11) + AGES_G(12);
}

/* 0x271C4: no blend. state is the frame's r3. */
static void vf2_si_bail(uint32_t rob, uint32_t state)
{
    ages_st8(rob + 0xBE5, state);
    ages_st8(rob + 0xBDD, 0);
    ages_st8(rob + 0xBE4, 0);
}

/* `stt g0` to the TGP with g0..g2 = x, y, z. */
static void vf2_si_stt(uint32_t x, uint32_t y, uint32_t z)
{
    AGES_G(0) = x;
    AGES_G(1) = y;
    AGES_G(2) = z;
    ages_stn(vf2_si_fifo(), &AGES_G(0), 3);
}

/* The angle step's reply: three ldis into g0..g2, x and z negated, all
 * shifted by sh (shri), to dst+4, dst, dst+8 (stis). */
static void vf2_si_step_out(uint32_t dst, uint32_t sh)
{
    AGES_G(0) = (uint32_t)ages_ld16s(vf2_si_fifo());
    AGES_G(1) = (uint32_t)ages_ld16s(vf2_si_fifo());
    AGES_G(2) = (uint32_t)ages_ld16s(vf2_si_fifo());
    AGES_G(0) = (uint32_t)((int32_t)(0u - AGES_G(0)) >> sh);
    ages_st16(dst + 4, AGES_G(0));
    AGES_G(1) = (uint32_t)((int32_t)AGES_G(1) >> sh);
    ages_st16(dst, AGES_G(1));
    AGES_G(2) = (uint32_t)((int32_t)(0u - AGES_G(2)) >> sh);
    ages_st16(dst + 8, AGES_G(2));
}

/* The 24 position steps: dst = (b - a) / (1 << sh), the reciprocal from
 * TGP 0x17; then 0x02 (pop). */
static void vf2_si_lerp(uint32_t dst, uint32_t a, uint32_t b, uint32_t sh)
{
    vf2_tgp_w(0x0B801717u);
    vf2_tgp_w(1u << sh);
    uint32_t inv = mo2_divr(vf2_tgp_r(), 0x3F800000u);
    for (int n = 0; n < 24; n++, dst += 4, a += 4, b += 4)
        ages_st32(dst, mo2_mulr(inv, mo2_subr(ages_ld32(a), ages_ld32(b))));
    vf2_tgp_w(0x01000202u);
}

/* 8 triples: x -= x0, z -= z0 through TGP 0x14, unless both are 0. */
static void vf2_si_rebase(uint32_t base)
{
    uint32_t x0 = ages_ld32(base), z0 = ages_ld32(base + 8);
    if ((x0 | z0) == 0)
        return;
    for (uint32_t p = base, n = 0; n < 8; n++, p += 12) {
        vf2_tgp_w(0x0A001414u);
        vf2_tgp_w(ages_ld32(p));
        vf2_tgp_w(x0);
        ages_st32(p, vf2_tgp_r());
        vf2_tgp_w(0x0A001414u);
        vf2_tgp_w(ages_ld32(p + 8));
        vf2_tgp_w(z0);
        ages_st32(p + 8, vf2_tgp_r());
    }
}

/* An angle for a rotate command: into the ring at 0x90E000, then the
 * command, which takes it from there. Nothing for 0. */
static void vf2_si_ring(uint32_t ang, uint32_t cmd)
{
    if (ang == 0)
        return;
    uint32_t i = ages_ld32(0x5001E4);
    ages_st32(0x90E000u + i, ang);
    ages_st8(0x5001E4, i + 4u);
    vf2_tgp_w(cmd);
}

/* Turn 8 (x, 0, z) triples by the current matrix (TGP 0x29); g0 counts. */
static void vf2_si_turn8(uint32_t p)
{
    for (AGES_G(0) = 8; AGES_G(0) != 0; AGES_G(0)--, p += 12) {
        vf2_tgp_w(0x14802929u);
        vf2_tgp_w(ages_ld32(p));
        vf2_tgp_w(0);
        vf2_tgp_w(ages_ld32(p + 8));
        uint32_t x = vf2_tgp_r();
        (void)vf2_tgp_r();
        uint32_t z = vf2_tgp_r();
        ages_st32(p, x);
        ages_st32(p + 8, z);
    }
}

/* 8 -> 3, 4 -> 2, else 1. */
static uint32_t vf2_si_shift(uint32_t len)
{
    return len == 8 ? 3u : len == 4 ? 2u : 1u;
}

/* ---- rear_smooth_int (0x2776C) ------------------------------------------- */

/* The rear pose by g7+0x814 to 0x50E000 (g0 left past it); 0 if none. */
static int vf2_si_rear_pose(uint32_t rob, uint32_t motion)
{
    switch (ages_ld8(rob + 0x814)) {
    case 0x00: AGES_G(0) = motion + 0x1E0; break;
    case 0x80: AGES_G(0) = motion + 0x2D0; break;
    case 0x03: AGES_G(0) = motion + 0x3C0; break;
    case 0x04: AGES_G(0) = motion + 0x5A0; break;
    case 0x83: AGES_G(0) = motion + 0x4B0; break;
    default: return 0;
    }
    uint32_t t[3];
    for (uint32_t dst = VF2_SI_START, n = 0; n < 20; n++, dst += 12) {
        ages_ldn(t, AGES_G(0), 3);
        ages_stn(dst, t, 3);
        AGES_G(0) += 12;
    }
    return 1;
}

/* 0x2792C: the translations at 0x50E090 and three angles turned by
 * g7+0x812. */
static void vf2_si_rear_turn(uint32_t rob)
{
    uint32_t ang = (uint32_t)ages_ld16s(rob + 0x812);
    vf2_tgp_w(0x00800101u);
    vf2_tgp_w(0x01800303u);
    vf2_si_ring(ang, 0x36806D6Du);
    vf2_si_turn8(0x50E090);
    vf2_tgp_w(0x01000202u);
    static const uint32_t a[3] = {0x50E034, 0x50E028, 0x50E01C};
    for (int i = 0; i < 3; i++)
        ages_st16(a[i], ang + (uint32_t)ages_ld16s(a[i]));
}

/* 0x27A34: steps from the end pose to the rear pose into motion+0xF0. */
static void vf2_si_rear_steps(uint32_t rob, uint32_t motion, uint32_t sh)
{
    uint32_t end = VF2_SI_END, rear = VF2_SI_START, out = motion + 0xF0;
    uint32_t base = rob + 0xBE8;
    vf2_tgp_w(0x00800101u);
    AGES_G(4) = ages_ld32(rob);
    AGES_G(3) = 0;
    for (int n = 0; n < 12; n++, out += 12, rear += 12, end += 12, base += 6) {
        vf2_tgp_w(0x2A805555u);
        vf2_si_stt((uint32_t)ages_ld16s(base + 4), (uint32_t)ages_ld16s(base),
                   (uint32_t)ages_ld16s(base + 2));
        vf2_si_stt(0u - (uint32_t)ages_ld16s(end), 0u - (uint32_t)ages_ld16s(end + 4),
                   0u - (uint32_t)ages_ld16s(end + 8));
        vf2_si_stt((uint32_t)ages_ld16s(rear + 8), (uint32_t)ages_ld16s(rear + 4),
                   (uint32_t)ages_ld16s(rear));
        vf2_si_step_out(out, sh);
    }
    vf2_si_lerp(out, end, rear, sh);
}

/* rear_smooth_int, from get_frame_dat's `call 0x2776c` (its r3 is 0). */
static void vf2_si_rear(void)
{
    uint32_t rob = AGES_G(7);
    if (ages_ld8(rob + 0xBE4) == 1) {
        vf2_si_bail(rob, 0);
        return;
    }
    uint32_t flags = ages_ld32(rob + 0x1A4);
    uint32_t motion = ages_ld32(rob + 0xBD8);
    uint32_t start = ages_ld16(rob + 0xBE0);
    if (ages_ld32(0x500068) & 0x100000u)
        AGES_G(6) >>= 1;
    AGES_G(5) = VF2_SI_END;
    afn_get_end_value(0);
    if (!vf2_si_rear_pose(rob, motion)) {
        ages_st8(rob + 0xBDD, ages_ld8(rob + 0xBDD) & ~2u);
        return;
    }
    vf2_si_rebase(0x50E180);
    vf2_si_rebase(0x50E090);
    if (((ages_ld32(rob + 0x804) >> 21) ^ (ages_ld32(rob) >> 6)) & 1u) {
        AGES_G(5) = VF2_SI_START;
        afn_set_mirror(0);
    }
    if (flags & 0x40u)
        vf2_si_rear_turn(rob);
    AGES_G(6) = ages_ld16(rob + 0x800);
    vf2_si_rear_steps(rob, motion, vf2_si_shift(AGES_G(6) - start));
}

/* ---- smooth_int (0x27130) ------------------------------------------------ */

/* The blend mode (0x2713C..0x271C0), 0 for none; sets the CC of the
 * test that ended it. */
static uint32_t vf2_si_mode(uint32_t rob)
{
    uint32_t v = ages_ld8(0x508008);
    vf2_cmpo(0, v);
    if (v != 0)
        return 0;
    AGES_AC &= ~7u;   /* bbc 26 */
    if (!(ages_ld32(rob) & (1u << 26)))
        return 0;
    vf2_cmpo(1, ages_ld8(rob + 0xBE4));
    if (ages_ld8(rob + 0xBE4) == 1)
        return 0;
    uint32_t f = ages_ld32(rob + 0x1A4);
    if (f & 0x20000u)
        return 2;
    if (ages_ld32(rob + 0x804) & 0xC0000u)
        return 1;
    uint32_t c30 = ages_ld32(rob + 0xC30);
    AGES_AC = (AGES_AC & ~7u) | 2u;   /* bbs 23, then cmpobe */
    if ((c30 & 0x800000u) || (c30 & 0x8040u) == 0x8040u)
        return 0;
    uint32_t k = ages_ld8(rob + 0x814);
    vf2_cmpo(4, k);
    if (k == 4)
        return 0;
    if (k == 0x7F)
        return 1;
    return (f & 0x6600u) ? 1u : 3u;
}

/* 0x27390: turn the end keys at 0x50E180 by the start pose's angles and the
 * fighter's (g7+0xC00.., 0x140..). */
static void vf2_si_turn(uint32_t rob)
{
    static const uint32_t cmd[6] = {0x37006E6E, 0x36006C6C, 0x36806D6D,
                                    0x36006C6C, 0x36806D6D, 0x37006E6E};
    static const uint16_t off[6] = {0xC04, 0xC00, 0xC02, 0x140, 0x142, 0x144};
    vf2_tgp_w(0x00800101u);
    vf2_tgp_w(0x01800303u);
    vf2_tgp_w(0x1F803F3Fu);
    uint32_t a[3] = {(uint32_t)ages_ld16s(0x50E038), (uint32_t)ages_ld16s(0x50E034),
                     (uint32_t)ages_ld16s(0x50E030)};
    ages_stn(vf2_si_fifo(), a, 3);
    for (int i = 0; i < 6; i++)
        vf2_si_ring(0u - (uint32_t)ages_ld16s(rob + off[i]), cmd[i]);
    vf2_si_turn8(0x50E180);
    vf2_tgp_w(0x01000202u);
}

/* 0x27588: g7+0xC30 bit 10 with flags bit 9: scale the keys at 0x50E098. */
static void vf2_si_scale(uint32_t rob, uint32_t c30)
{
    uint32_t a, b;
    if (((ages_ld32(rob) & 0x40u) != 0) == ((c30 & 0x800u) != 0)) {
        a = ages_ld32(0x50E1D0);
        b = ages_ld32(0x50E0E0);
        ages_st32(0x50E1CC, 0x3E0F5C29u);
    } else {
        a = ages_ld32(0x50E1DC);
        b = ages_ld32(0x50E0EC);
    }
    vf2_tgp_w(0x0A001414u);
    vf2_tgp_w(a);
    vf2_tgp_w(b);
    uint32_t k = vf2_tgp_r();
    for (uint32_t p = 0x50E098, n = 0; n < 8; n++, p += 12) {
        vf2_tgp_w(0x09801313u);
        vf2_tgp_w(ages_ld32(p));
        vf2_tgp_w(k);
        ages_st32(p, vf2_tgp_r());
    }
}

/* 0x27628: the steps from the start pose to the end pose at the motion; the
 * first 5 angle triples stay 0 under flags bit 20 or g7+0xC30 bit 6. */
static void vf2_si_steps(uint32_t rob, uint32_t dst, uint32_t sh)
{
    uint32_t start = VF2_SI_START, end = VF2_SI_END, ang = rob + 0xBE8;
    vf2_tgp_w(0x00800101u);
    AGES_G(4) = ages_ld32(rob);
    int keep = (ages_ld32(rob + 0x1A4) & 0x100000u) || (ages_ld32(rob + 0xC30) & 0x40u);
    AGES_G(3) = 0;
    for (uint32_t n = 12; n != 0; n--, dst += 12, end += 12, start += 12, ang += 6) {
        if (keep && n >= 8) {
            ages_st16(dst, AGES_G(3));
            ages_st16(dst + 4, AGES_G(3));
            ages_st16(dst + 8, AGES_G(3));
            continue;
        }
        vf2_tgp_w(0x2A005454u);
        vf2_si_stt(0u - (uint32_t)ages_ld16s(start), 0u - (uint32_t)ages_ld16s(start + 4),
                   0u - (uint32_t)ages_ld16s(start + 8));
        vf2_si_stt((uint32_t)ages_ld16s(end + 8), (uint32_t)ages_ld16s(end + 4),
                   (uint32_t)ages_ld16s(end));
        vf2_si_stt((uint32_t)ages_ld16s(ang + 2), (uint32_t)ages_ld16s(ang),
                   (uint32_t)ages_ld16s(ang + 4));
        vf2_si_step_out(dst, sh);
    }
    vf2_si_lerp(dst, start, end, sh);
    AGES_AC = (AGES_AC & ~7u) | 2u;   /* the last cmpdeco */
}

/* 0x27244: start and end poses, rebased keys. */
static void vf2_si_pose(uint32_t motion)
{
    uint32_t t[3];
    AGES_G(5) = VF2_SI_START;
    afn_get_start_value(0);
    for (uint32_t i = 0; i < 20; i++) {
        ages_ldn(t, motion + 0x690 + i * 12, 3);
        ages_stn(VF2_SI_END + i * 12, t, 3);
    }
    for (uint32_t i = 0; i < 20; i++) {
        ages_ldn(t, VF2_SI_START + i * 12, 3);
        ages_stn(motion + 0x690 + i * 12, t, 3);
    }
    vf2_si_rebase(0x50E090);
    vf2_si_rebase(0x50E180);
}

/* 0x271D8: the blend's length and state; 0 if it ends there (state 2, or a
 * motion of 2 frames or less). */
static int vf2_si_length(uint32_t rob, uint32_t *state, uint32_t *len)
{
    uint32_t n = ages_ld16(rob + 0x800);
    vf2_cmpo(2, n);
    if (n <= 2) {
        vf2_si_bail(rob, *state);
        return 0;
    }
    *len = n <= 16 ? 4u : 8u;   /* 0x271F4 falls through to 4 */
    uint32_t m = n - *len;
    if (ages_ld8(rob + 0xA00) != 0 && m < ages_ld16(rob + 0x808))
        *state &= ~2u;
    if ((ages_ld8(rob + 0xBE5) & 2u) && (ages_ld8(rob + 0xBDD) & 2u))
        *state &= ~1u;
    ages_st8(rob + 0xBDD, *state);
    ages_st16(rob + 0xBDE, *len);
    ages_st16(rob + 0xBE0, m);
    ages_st8(rob + 0xBE5, 0);
    vf2_cmpo(2, *state);
    return *state != 2;
}

static uint32_t afn_vf2_smooth_int(int entry)
{
    uint32_t rob = AGES_G(7), len = 0;
    ages_st16(rob + 0xBE2, 0);
    uint32_t state = vf2_si_mode(rob);
    if (state == 0) {
        vf2_si_bail(rob, 0);
    } else if (vf2_si_length(rob, &state, &len)) {
        uint32_t f = ages_ld32(rob + 0x1A4);
        uint32_t motion = ages_ld32(rob + 0xBD8);
        vf2_si_pose(motion);
        if ((f & 0x100000u) || (ages_ld32(rob + 0xC30) & 0x40u))
            vf2_si_turn(rob);
        uint32_t c30 = ages_ld32(rob + 0xC30);
        if ((c30 & 0x400u) && (f & 0x400u)) {
            AGES_AC = (AGES_AC & ~7u) | 2u;   /* bbs 10 */
            vf2_si_bail(rob, state);
        } else {
            if ((c30 & 0x400u) && (f & 0x200u))
                vf2_si_scale(rob, c30);
            vf2_si_steps(rob, motion, vf2_si_shift(len));
        }
    }
    if (entry) ages_i960_ret();
    return 0;
}
