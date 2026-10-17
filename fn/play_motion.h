/* play_motion: i960 0x1C0FC, PS2 0x1D5700 (EXT 59).
 *
 * Step the fighter's motion one frame: start the next motion when this one
 * ends (set_motion, or smooth_int for 0xFFFF), set the angles
 * (calc_rob_angle), the phase flags at g7+0x1A4 by g7+0xA00, then run the
 * motion's event stream (g4 = g7+0x82C) up to the current frame through the
 * 31-opcode table at 0x1D028. Opcodes 10, 11 and 23 can end the frame at
 * once (ret, the stream pointer not stored). The stream pointer is g4
 * itself, so a callee that moves g4 moves the stream as on the board. */
#pragma once
#include "set_mot_dat.h"
#include "set_option.h"

#define PM_CC(v) (AGES_AC = (AGES_AC & ~7u) | (v))

static uint32_t pm_b(uint32_t off) { return ages_ld8(AGES_G(4) + off); }
static uint32_t pm_h(uint32_t off) { return pm_b(off) | pm_b(off + 1) << 8; }

/* Op handlers: return the stream step, or -cc to end the frame. */
typedef int (*pm_op_fn)(uint32_t g7, int eq);

static void pm_bit(uint32_t a, int how) {   /* 0 set, 1 clear, 2 flip */
    uint32_t v = ages_ld32(a), m = 1u << (pm_b(3) & 31);
    ages_st32(a, how == 0 ? v | m : how == 1 ? v & ~m : v ^ m);
}

static int pm_quit(uint32_t g7, uint32_t off, uint32_t cc) {
    ages_st8(g7 + 0x60C, 31);
    ages_st8(g7 + 0x852, pm_b(off));
    ages_st8(g7 + 0x853, pm_b(off + 1));
    return -(int)cc;
}

static int pm_op1(uint32_t g7, int eq) {
    (void)eq; ages_st32(g7 + 0x1A4, ages_ld32(g7 + 0x1A4) ^ (1u << 21)); return 3;
}
static int pm_op2(uint32_t g7, int eq) {
    (void)eq; ages_st32(g7 + 0x198, 13u << 24); ages_st16(g7 + 0x5DE, 0); return 3;
}

/* Move: the step along the turned heading, then fix_hara. */
static int pm_op3(uint32_t g7, int eq) {
    (void)eq;
    uint32_t a = pm_h(3);
    if (ages_ld32(g7) & 0x40u) a = 0u - a;
    a += (uint32_t)ages_ld16s(g7 + 0x26);
    uint32_t r12 = 0u - a;
    ages_cop_w(0x10802121); ages_cop_w(r12);
    uint32_t s = ages_cop_r();
    ages_cop_w(0x11002222); ages_cop_w(r12);
    uint32_t c = ages_cop_r();
    uint32_t r3 = mo2_le32(AGES_G(4) + 5), r6 = mo2_le32(AGES_G(4) + 9);
    ages_st32(g7 + 0x2C, mo2_mulr(r3, s));
    ages_cop_w(0x0A801515); ages_cop_w(r3); ages_cop_w(c);
    ages_st32(g7 + 0x34, ages_cop_r());
    ages_cop_w(0x0A801515); ages_cop_w(r6); ages_cop_w(ages_ld32(0x50A000));
    ages_st32(g7 + 0x30, ages_cop_r());
    afn_fix_hara(0);
    return 13;
}

static void pm_sound(uint32_t s) {
    AGES_G(0) = s;
    afn_check_same_sound(0);
    afn_sound_request_u(0);
}

static int pm_op4(uint32_t g7, int eq) {
    (void)g7;
    if (!eq || ages_ld32(0x50002C) == 0x80) return 7;
    uint32_t s = mo2_le32(AGES_G(4) + 3);
    if (s == 0xAE1153) { AGES_G(0) = 3; afn_sound_chainlink_hit(0); }
    else if (s == 0xAE113D) { AGES_G(0) = 0; afn_stage_contact_sound(0); }
    else pm_sound(s);
    return 7;
}

static int pm_op5(uint32_t g7, int eq) { (void)eq; pm_bit(g7 + 0x1A4, 0); return 4; }
static int pm_op6(uint32_t g7, int eq) { (void)eq; pm_bit(g7 + 0x1A4, 1); return 4; }
static int pm_op7(uint32_t g7, int eq) { (void)eq; pm_bit(g7 + 0x1A4, 2); return 4; }

/* Hit stop: add to g7+0x5DE, take from g7+0x1AC. */
static int pm_op8(uint32_t g7, int eq) {
    (void)eq;
    if (ages_ld16s(0x500028) <= 0) return 4;
    int32_t d = (int8_t)pm_b(3), left = ages_ld16s(g7 + 0x1AC);
    if (ages_ld8(g7 + 0xA28) & 1u)
        ages_st16(g7 + 0x5DE, ages_ld16(g7 + 0x5DE) + (uint32_t)d);
    else
        ages_st16(g7 + 0x5DE, (uint32_t)d);
    ages_st8(g7 + 0xA28, 1);
    left -= d;
    ages_st16(AGES_G(8) + 0x1F70, (uint32_t)d);
    if (left <= 0) { ages_st32(g7, ages_ld32(g7) | 0x20u); left = 0; }
    ages_st16(g7 + 0x1AC, (uint32_t)left);
    return 4;
}

static int pm_op9(uint32_t g7, int eq) {
    if (!eq || ages_ld32(0x50002C) == 0x80) return 4;
    uint32_t k = pm_b(3), who = ages_ld8(g7 + 0x1B1);
    if (k != 9 && ages_ld16(0x504070) != 0) return 4;
    uint32_t s = ages_ld32(ages_ld32(0x642592C + k * 4) + who * 4);
    if (s == 0) return 4;
    pm_sound(s);
    uint32_t t = ages_ld32(0x6425978 + k * 4);
    if (t == 0) return 4;
    uint32_t v = ages_ld16(t + who * 2);
    if (v != 0) ages_st16(g7 + 0x7D4, v);
    return 4;
}

/* Input check: end the frame when the input history holds the command. */
static int pm_op10(uint32_t g7, int eq) {
    (void)eq;
    uint32_t cmd = pm_h(3), mask = pm_b(7), any = pm_b(8), hist = g7 + 0x1282;
    if (mask == 1)
        return ages_ld16(g7 + 0x1AC) != 0 ? 9 : pm_quit(g7, 5, 2u);
    if (mask == 0xF7) hist = g7 + 0x1280;
    AGES_G(0) = cmd;
    afn_check_front_back(0);
    cmd = AGES_G(0);
    uint32_t i = ages_ld8(g7 + 0x1202);
    do {
        i = (i - 1u) & 0x3Fu;
        uint32_t v = ages_ld8(hist + i * 4);
        if ((any == 0 || (ages_ld8(g7 + 0x1280 + i * 4) & 0x10u)) && (v & mask) == cmd)
            return pm_quit(g7, 5, 2u);
    } while (ages_ld8(g7 + 0x1204) != i);
    return 9;
}

static int pm_op11(uint32_t g7, int eq) {
    (void)eq;
    uint32_t n = pm_h(3);
    if (n == 0) return pm_quit(g7, 5, 2u);
    afn_rand(0);
    return AGES_G(0) < n ? pm_quit(g7, 5, 4u) : 7;
}

static int pm_op12(uint32_t g7, int eq) {
    (void)eq; (void)g7; pm_bit(AGES_G(8) + 0x1A4, 1); return 4;
}
static int pm_op13(uint32_t g7, int eq) {
    (void)eq;
    uint32_t v = ages_ld32(g7 + 0x1A4) | (1u << 25);
    if (pm_b(3) != 0) v &= ~(1u << 25);
    ages_st32(g7 + 0x1A4, v);
    return 4;
}
static int pm_op14(uint32_t g7, int eq) {
    (void)g7;
    if (!eq) return 7;
    uint32_t p = ages_ld32(0x500814);
    for (uint32_t i = 0; i < 4; i++) ages_st8(p + 0xFC + i, pm_b(3 + i));
    return 7;
}
static int pm_op15(uint32_t g7, int eq) {
    if (!eq) return 3;
    uint32_t n = ages_ld8(g7 + 0x6CC) + 1u;
    ages_st8(g7 + 0x6CC, n > 8 ? 8 : n);
    return 3;
}
static int pm_op16(uint32_t g7, int eq) {
    (void)eq;
    AGES_G(0) = ages_ld32(ages_ld32(0x640D31C + pm_b(3) * 4) + ages_ld8(g7 + 0x1B0) * 4);
    afn_efc_face_change_cont(0);
    return 4;
}

/* Option model on (set_option) or off (delete_option). */
static int pm_op17(uint32_t g7, int eq) {
    if (!eq) return 4;
    uint32_t on = ages_ld32(g7) & (1u << 27), n = pm_b(3);
    if (n == 0) {
        if (on) opt_delete_option();
    } else if (!on) {
        ages_st32(g7 + 0xAFC, n);
        afn_set_option(0);
    }
    return 4;
}

static int pm_op18(uint32_t g7, int eq) {
    (void)eq;
    uint32_t t = ages_ld32(0x640EBF4 + ages_ld8(g7 + 0x1B0) * 4), side = pm_b(3);
    AGES_G(0) = t;
    if (ages_ld32(g7) & 0x40u) side = side == 1 ? 0 : 1;
    uint32_t k = pm_b(4) + side * 4;
    if (ages_ld16(t + k * 2) != 0)
        ages_st32(g7 + 0x40 + (side * 3 + 5) * 4, (uint32_t)ages_ld16s(t + k * 2));
    return 7;
}

static int pm_op19(uint32_t g7, int eq) { (void)eq; ages_st16(g7 + 0x5DE, pm_h(3)); return 5; }
static int pm_op20(uint32_t g7, int eq) { (void)eq; (void)g7; return 4; }
static int pm_op21(uint32_t g7, int eq) { if (eq) ages_st8(g7 + 0x6CE, pm_b(3)); return 4; }
static int pm_op22(uint32_t g7, int eq) {
    (void)eq;
    ages_st16(g7 + 0x812, pm_h(3));
    ages_st32(g7 + 0x1A4, ages_ld32(g7 + 0x1A4) | 0x40u);
    return 5;
}
static int pm_op23(uint32_t g7, int eq) {
    (void)eq;
    uint32_t want = pm_b(3) != 0, have = ages_ld32(g7 + 0x5B8) & 1u;
    return want == have ? pm_quit(g7, 4, 2u) : 6;
}
static int pm_op24(uint32_t g7, int eq) {
    (void)eq;
    uint32_t v = ages_ld32(g7 + 0x720) & ~1u;
    ages_st32(g7 + 0x720, v | (pm_b(3) == 1));
    for (uint32_t i = 0; i < 4; i++) ages_st8(g7 + 0xA02 + i, pm_b(4 + i));
    for (uint32_t i = 0; i < 4; i++) ages_st8(g7 + 0xACC + i, pm_b(8 + i));
    return 12;
}
static int pm_op25(uint32_t g7, int eq) { (void)eq; pm_bit(g7 + 0x720, 0); return 4; }
static int pm_op26(uint32_t g7, int eq) { (void)eq; pm_bit(g7 + 0x720, 1); return 4; }
static int pm_op27(uint32_t g7, int eq) {
    (void)eq;
    uint32_t v = ages_ld32(g7 + 0x720) & ~8u;
    ages_st32(g7 + 0x720, v | (pm_b(3) == 1 ? 8u : 0u));
    return 4;
}
static int pm_op28(uint32_t g7, int eq) {
    (void)eq;
    ages_st8(g7 + 0xA25, pm_b(3) | pm_b(4) << 4);
    ages_st8(g7 + 0xA24, pm_b(5));
    return 6;
}
static int pm_op29(uint32_t g7, int eq) {
    (void)eq;
    if (ages_ld8(0x50002B) == 9 && ages_ld8(0x500031) == 0xF) return 3;
    ages_st32(0x50A310, ages_ld8(g7 + 4) << 7 | 1u);
    AGES_G(0) = ages_ld8(g7 + 0x822);
    afn_kabe_parts_damage(0);
    return 3;
}
static int pm_op30(uint32_t g7, int eq) { (void)eq; ages_st16(g7 + 0xC56, pm_b(3)); return 4; }

static const pm_op_fn pm_ops[31] = {
    0, pm_op1, pm_op2, pm_op3, pm_op4, pm_op5, pm_op6, pm_op7, pm_op8,
    pm_op9, pm_op10, pm_op11, pm_op12, pm_op13, pm_op14, pm_op15, pm_op16,
    pm_op17, pm_op18, pm_op19, pm_op20, pm_op21, pm_op22, pm_op23, pm_op24,
    pm_op25, pm_op26, pm_op27, pm_op28, pm_op29, pm_op30,
};

/* The next motion when this one has run out (0x1C0FC..0x1C178). */
static void pm_next(uint32_t g7) {
    if (ages_ld16(g7 + 0x1A8) == 0) {
        AGES_G(0) = ages_ld16(AGES_G(6));
        afn_set_motion(0);
    } else {
        uint32_t at = ages_ld16(g7 + 0x1AA), len = ages_ld16(g7 + 0x800);
        if (at != 0 && at <= len) return;
        if (at != 0) {
            AGES_G(0) = ages_ld16(g7 + 0x802);
            if (AGES_G(0) == 0xFFFF) {
                ages_st32(g7 + 0x82C, ages_ld32(g7 + 0x700));
                ages_st32(g7 + 0xBD4, ages_ld32(g7 + 0x1A4));
                afn_smooth_int(0);
            } else {
                if (ages_ld16(g7 + 0x86C) != 0 && ages_ld16(g7 + 0xA1C) > ages_ld16(g7 + 0x86E)) {
                    ages_st16(g7 + 0x802, ages_ld16(g7 + 0x86C));
                    AGES_G(0) = ages_ld16(g7 + 0x802);
                }
                if (AGES_G(0) == 0) { ages_st16(g7 + 0x1AA, len); return; }
                afn_set_motion(0);
            }
        }
    }
    ages_st16(g7 + 0x1AA, 1);
}

static void pm_inc_a00(uint32_t g7) { ages_st8(g7 + 0xA00, ages_ld8(g7 + 0xA00) + 1u); }

/* Phase 1 of the flags (0x1C19C); returns 0 to stop at the store. */
static int pm_phase1(uint32_t g7, int32_t at, uint32_t *f) {
    int32_t lim = (int32_t)ages_ld16(g7 + 0x808);
    uint32_t w = ages_ld32(g7 + 0x804);
    if ((w & 0x100u) && (w & 0x8000u) && at >= lim) *f &= ~(1u << 16);
    if (at < lim) return 0;
    *f &= ~2u;
    pm_inc_a00(g7);
    return 1;
}

/* Phase 2 (0x1C1C8). */
static int pm_phase2(uint32_t g7, int32_t at, uint32_t *f) {
    if (at < (int32_t)ages_ld16(g7 + 0x80A)) return 0;
    if (*f & 0x100u) *f &= ~0x8000u;
    *f = (*f & ~0x4010100u) | 4u;
    ages_st16(g7 + 0x1224, 0);
    ages_st16(g7 + 0x1248, 0);
    pm_inc_a00(g7);
    return 1;
}

/* Phase 3 (0x1C20C). */
static void pm_phase3(uint32_t g7, int32_t at, uint32_t *f) {
    if ((*f & 0x1000u) && (*f & 0x10u)) {
        int32_t e = (int32_t)ages_ld16(g7 + 0x850);
        if (e != 0 && at >= e) *f &= ~1u;
    }
    if (at < (int32_t)ages_ld16(g7 + 0x80C)) return;
    *f &= ~0x1001u;
    pm_inc_a00(g7);
}

/* The phase flags by g7+0xA00 (0x1C180..0x1C244); phases past 3 run as 1. */
static void pm_phase(uint32_t g7) {
    uint32_t ph = ages_ld8(g7 + 0xA00);
    int32_t at = (int32_t)ages_ld16(g7 + 0x1AA);
    uint32_t f = ages_ld32(g7 + 0x1A4);
    if (ph == 0) return;
    if ((ph == 2 || ph == 3 || pm_phase1(g7, at, &f)) &&
        (ph == 3 || pm_phase2(g7, at, &f)))
        pm_phase3(g7, at, &f);
    ages_st32(g7 + 0x1A4, f);
}

/* The end of the stream (0x1C94C..0x1CA1C); returns the cc. */
static uint32_t pm_end(uint32_t g7) {
    ages_st32(g7 + 0x82C, AGES_G(4));
    if (ages_ld32(g7 + 0x1A4) & (1u << 20)) {
        ages_st32(g7 + 0x1A4, ages_ld32(g7 + 0x1A4) & ~(1u << 20));
        if (ages_ld16(g7 + 0x870) != 0) {
            uint32_t a = ages_ld16(g7 + 0x616);
            ages_st16(g7 + 0x26, a);
            ages_st16(g7 + 0x61A, a);
        } else {
            ages_st16(g7 + 0x26, ages_ld16(g7 + 0x26) ^ 0x8000u);
            ages_st16(g7 + 0x61A, ages_ld16(g7 + 0x61A) ^ 0x8000u);
        }
    }
    uint32_t f = ages_ld32(g7 + 0x1A4);
    if (ages_ld32(g7 + 0x804) & 0x80u) f |= 0x20u;
    if (f & 0x80u) {
        f &= ~0x80u;
        ages_cop_w(0x2D805B5B); ages_cop_w((uint32_t)ages_ld16s(g7 + 0x26));
        ages_cop_w(ages_ld32(g7 + 0x80)); ages_cop_w(ages_ld32(g7 + 0x88));
        uint32_t x = ages_cop_r(), z = ages_cop_r();
        ages_st32(g7 + 0x18, mo2_subr(x, ages_ld32(g7 + 0x1F4)));
        ages_st32(g7 + 0x20, mo2_subr(z, ages_ld32(g7 + 0x1FC)));
    }
    ages_st32(g7 + 0x1A4, f);
    uint32_t w = ages_ld32(g7);
    if (!(w & 0x1000u)) return 2u;
    ages_st32(g7, w & ~0x1000u);
    ages_st32(g7 + 0x1A4, ages_ld32(g7 + 0x1A4) | 1u);
    return 0u;
}

static uint32_t afn_play_motion(int entry) {
    uint32_t g7 = AGES_G(7), cc;
    pm_next(g7);
    afn_calc_rob_angle(0);
    pm_phase(g7);
    AGES_G(4) = ages_ld32(g7 + 0x82C);
    for (;;) {
        uint32_t op = pm_b(0), at = pm_h(1), now = ages_ld16(g7 + 0x1AA);
        if (op == 0 || now < at || op > 30) { cc = pm_end(g7); break; }
        int r = pm_ops[op](g7, now == at);
        if (r < 0) { cc = (uint32_t)-r; break; }
        AGES_G(4) += (uint32_t)r;
    }
    PM_CC(cc);
    if (entry) ages_i960_ret();
    return 0;
}
