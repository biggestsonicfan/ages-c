/* smooth_int: i960 0x29F48, PS2 0x1E4940.
 *
 * Start a motion blend: pick the blend mode (g7+0xBDD) and length
 * (g7+0xBDE/0xBE0), snapshot the start pose (get_start_value into
 * 0x50E000), take the start/end angle differences through the COP (0x54)
 * and lay per-frame steps at the blend buffer (g7+0xBD8). Like gems-c's (STF),
 * but FV's checks g7+0x720 and g7+0x860, has the turn fix-up on g7+0xC30 bit
 * 10, no unit_smooth_cancel_front, and stores each angle step twice (the
 * second pass negates g0 and g2 again): the board's code, kept. */
#pragma once
#include "set_mot_dat.h"   /* mo2_ float helpers */

#define SMI_COP_RS16() ((uint32_t)(int32_t)(int16_t)ages_cop_r())

/* The blend mode, or -1 (and *cc) for no blend (0x29F50..0x29FFC). */
static int smi_mode(uint32_t g7, uint32_t f, uint32_t *cc) {
    *cc = 2u;
    if (ages_ld8(0x508008u) != 0) { *cc = 4u; return -1; }
    if (!(ages_ld32(g7) & (1u << 26)) || (ages_ld32(g7 + 0x720) & 2u) ||
        (ages_ld32(g7 + 0x860) & 4u) || ages_ld8(g7 + 0xBE4) == 1)
        return -1;
    if (f & (1u << 17)) return 2;
    if (ages_ld32(g7 + 0x804) & (1u << 18)) return 3;
    if (ages_ld32(g7 + 0x804) & (1u << 19)) return 1;
    if ((ages_ld32(g7 + 0xC30) & 0x8040u) == 0x8040u) return -1;
    uint32_t k = ages_ld8(g7 + 0x814);
    if (k == 4) return -1;
    if (k == 0x7F) return 1;
    if (f & (1u << 26)) return 3;
    return (f & 0x6600u) ? 1 : 3;
}

static void smi_bail(uint32_t g7, uint32_t state, uint32_t cc) {
    ages_st8(g7 + 0xBE5, state);
    ages_st8(g7 + 0xBDD, 0);
    ages_st8(g7 + 0xBE4, 0);
    AGES_AC = (AGES_AC & ~7u) | cc;
}

/* Two lerp keys (a, b) through op 0x14 over 8 (x, ?, z) triples. */
static void smi_keys(uint32_t start) {
    uint32_t a = ages_ld32(start), b = ages_ld32(start + 8);
    if ((a | b) == 0) return;
    for (uint32_t p = start, i = 0; i < 8; i++, p += 12) {
        ages_cop_w(0x0A001414); ages_cop_w(ages_ld32(p)); ages_cop_w(a);
        ages_st32(p, ages_cop_r());
        ages_cop_w(0x0A001414); ages_cop_w(ages_ld32(p + 8)); ages_cop_w(b);
        ages_st32(p + 8, ages_cop_r());
    }
}

/* Turn the 8 end keys at 0x50E180 by the fighter's angles (0x2A1D0). */
static void smi_turn(uint32_t g7) {
    static const uint32_t cmd[6] = { 0x05000A0A, 0x04000808, 0x04800909,
                                     0x04000808, 0x04800909, 0x05000A0A };
    static const uint16_t off[6] = { 0xC04, 0xC00, 0xC02, 0x140, 0x142, 0x144 };
    ages_cop_w(0x00800101); ages_cop_w(0x01800303); ages_cop_w(0x1F803F3F);
    ages_cop_w((uint32_t)ages_ld16s(0x50E038));
    ages_cop_w((uint32_t)ages_ld16s(0x50E034));
    ages_cop_w((uint32_t)ages_ld16s(0x50E030));
    ages_cop_r();
    for (int i = 0; i < 6; i++) {
        ages_cop_w(cmd[i]);
        ages_cop_w(0u - (uint32_t)ages_ld16s(g7 + off[i]));
    }
    for (uint32_t p = 0x50E180; p < 0x50E180 + 8 * 12; p += 12) {
        uint32_t x = ages_ld32(p), z = ages_ld32(p + 8);
        ages_cop_w(0x14802929); ages_cop_w(x); ages_cop_w(0); ages_cop_w(z);
        x = ages_cop_r(); ages_cop_r(); z = ages_cop_r();
        ages_st32(p, x);
        ages_st32(p + 8, z);
    }
    AGES_G(0) = 0;
    ages_cop_w(0x01000202);
}

/* Bit 10 of g7+0xC30 with bit 9 of the flags: scale the 8 keys at 0x50E098
 * (0x2A310). */
static void smi_scale(uint32_t g7, uint32_t c30) {
    uint32_t a, b;
    if (((ages_ld32(g7) & 0x40u) != 0) == ((c30 & 0x800u) != 0)) {
        a = ages_ld32(0x50E1D0); b = ages_ld32(0x50E0E0);
        ages_st32(0x50E1CC, 0x3E0F5C29);
    } else {
        a = ages_ld32(0x50E1DC); b = ages_ld32(0x50E0EC);
    }
    ages_cop_w(0x0A001414); ages_cop_w(a); ages_cop_w(b);
    uint32_t k = ages_cop_r();
    for (uint32_t p = 0x50E098; p < 0x50E098 + 8 * 12; p += 12) {
        ages_cop_w(0x09801313); ages_cop_w(ages_ld32(p)); ages_cop_w(k);
        ages_st32(p, ages_cop_r());
    }
}

/* One angle triple through op 0x54 (0x2A418..0x2A4BC). */
static void smi_angle(uint32_t dst, uint32_t start, uint32_t end, uint32_t ang, uint32_t sh, int first) {
    ages_cop_w(0x2A005454);
    ages_cop_w(0u - (uint32_t)ages_ld16s(start));
    ages_cop_w(0u - (uint32_t)ages_ld16s(start + 4));
    ages_cop_w(0u - (uint32_t)ages_ld16s(start + 8));
    ages_cop_w((uint32_t)ages_ld16s(end + 8));
    ages_cop_w((uint32_t)ages_ld16s(end + 4));
    ages_cop_w((uint32_t)ages_ld16s(end));
    ages_cop_w((uint32_t)ages_ld16s(ang + 2));
    ages_cop_w((uint32_t)ages_ld16s(ang));
    ages_cop_w((uint32_t)ages_ld16s(ang + 4));
    int32_t g0 = (int32_t)SMI_COP_RS16(), g1 = (int32_t)SMI_COP_RS16(), g2 = (int32_t)SMI_COP_RS16();
    for (int pass = 0; pass < 2; pass++) {
        if (pass == 0 && first && g0 > 0) g0 >>= sh;
        else g0 = (int32_t)(0u - (uint32_t)g0) >> sh;
        g1 >>= sh;
        g2 = (int32_t)(0u - (uint32_t)g2) >> sh;
        ages_st16(dst + 4, (uint32_t)g0);
        ages_st16(dst, (uint32_t)g1);
        ages_st16(dst + 8, (uint32_t)g2);
    }
    AGES_G(0) = (uint32_t)g0; AGES_G(1) = (uint32_t)g1; AGES_G(2) = (uint32_t)g2;
}

/* The steps: 12 angle triples, then 24 position words (0x2A3A8..). */
static void smi_steps(uint32_t g7, uint32_t base, uint32_t step) {
    uint32_t sh = step == 8 ? 3 : step == 4 ? 2 : 1;
    uint32_t start = 0x50E000, ang = g7 + 0xBE8, end = 0x50E0F0, dst = base;
    ages_cop_w(0x00800101);
    AGES_G(4) = ages_ld32(g7);
    int keep = (ages_ld32(g7 + 0x1A4) & (1u << 20)) || (ages_ld32(g7 + 0xC30) & 0x40u);
    AGES_G(3) = 0;
    for (uint32_t n = 12; n >= 1; n--, dst += 12, end += 12, start += 12, ang += 6) {
        if (keep && n >= 8) {
            ages_st16(dst, 0); ages_st16(dst + 4, 0); ages_st16(dst + 8, 0);
        } else {
            smi_angle(dst, start, end, ang, sh, n == 8);
        }
    }
    ages_cop_w(0x0B801717);
    ages_cop_w(1u << sh);
    uint32_t inv = mo2_divr(ages_cop_r(), 0x3F800000u);
    for (uint32_t i = 0; i < 24; i++, dst += 4, end += 4, start += 4)
        ages_st32(dst, mo2_mulr(inv, mo2_subr(ages_ld32(start), ages_ld32(end))));
    ages_cop_w(0x01000202);
}

/* The start pose into 0x50E000; the end pose (base+0x690) to 0x50E0F0 and
 * the start pose in its place (0x2A084..0x2A0E0). */
static void smi_pose(uint32_t base) {
    uint32_t t[3];
    AGES_G(5) = 0x50E000;
    afn_get_start_value(0);
    for (uint32_t i = 0; i < 20; i++) {
        ages_ldn(t, base + 0x690 + i * 12, 3);
        ages_stn(0x50E0F0 + i * 12, t, 3);
    }
    for (uint32_t i = 0; i < 20; i++) {
        ages_ldn(t, 0x50E000 + i * 12, 3);
        ages_stn(base + 0x690 + i * 12, t, 3);
    }
}

/* The blend's length, then the parts after the mode (0x2A018..). */
static void smi_blend(uint32_t g7, uint32_t f, uint32_t state) {
    uint32_t n = ages_ld16(g7 + 0x800);
    if (n <= 2) { smi_bail(g7, state, n == 2 ? 2u : 1u); return; }
    uint32_t step = n <= 16 ? 4 : 8, m = n - step;
    if (ages_ld8(g7 + 0xA00) != 0 && m < ages_ld16(g7 + 0x808)) state &= ~2u;
    if ((ages_ld8(g7 + 0xBE5) & 2u) && (ages_ld8(g7 + 0xBDD) & 2u)) state &= ~1u;
    ages_st8(g7 + 0xBDD, state);
    ages_st16(g7 + 0xBDE, step);
    ages_st16(g7 + 0xBE0, m);
    ages_st8(g7 + 0xBE5, 0);
    AGES_AC = (AGES_AC & ~7u) | 2u;
    if (state == 2) return;

    uint32_t base = ages_ld32(g7 + 0xBD8);
    smi_pose(base);
    smi_keys(0x50E090);
    smi_keys(0x50E180);
    uint32_t c30 = ages_ld32(g7 + 0xC30);
    if ((f & (1u << 20)) || (c30 & 0x40u)) smi_turn(g7);
    c30 = ages_ld32(g7 + 0xC30);
    if (c30 & 0x400u) {
        if (f & 0x400u) { smi_bail(g7, state, 2u); return; }
        if (f & 0x200u) smi_scale(g7, c30);
    }
    smi_steps(g7, base, step);
    AGES_AC = (AGES_AC & ~7u) | 2u;
}

static uint32_t afn_smooth_int(int entry) {
    uint32_t g7 = AGES_G(7), cc;
    ages_st16(g7 + 0xBE2, 0);
    uint32_t f = ages_ld32(g7 + 0x1A4);
    int state = smi_mode(g7, f, &cc);
    if (state < 0) smi_bail(g7, 0, cc);
    else smi_blend(g7, f, (uint32_t)state);
    if (entry) ages_i960_ret();
    return 0;
}
