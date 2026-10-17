/* set_mot_dat: i960 0x1B880, PS2 0x1D28C0 (EXT 60;
 * decomp/FV/FN/ext60_set_mot_dat_001d28c0.c).
 *
 * g7: the fighter, g0: the motion number. Clears the motion state, then runs
 * the motion's command stream (0x6411E94[g0]) into g7+0x800.., one opcode
 * byte and its argument bytes at a time, up to opcode 0 or 8. The board's
 * jump table runs on past IDA's end of the function (to 0x1C0B8); the PS2's
 * C is the same. Also holds this group's (motion2) float helpers: the i960's
 * real ops as m2-hle2 runs them (in double, rounded to single, x/0 = 0). */
#pragma once

#define MO2_CC(v) (AGES_AC = (AGES_AC & ~7u) | (v))

static inline uint32_t mo2_fr(double d) { return i960_double_to_single(d); }
static inline double   mo2_fd(uint32_t u) { return i960_single_to_double(u); }
/* addr / subr / mulr / divr s1,s2: s1+s2, s2-s1, s1*s2, s2/s1 */
static inline uint32_t mo2_addr(uint32_t s1, uint32_t s2) {
    double a = mo2_fd(s1), b = mo2_fd(s2);
    return mo2_fr(i960_nan_result(a + b, a, b));
}
static inline uint32_t mo2_subr(uint32_t s1, uint32_t s2) {
    double a = mo2_fd(s1), b = mo2_fd(s2);
    return mo2_fr(i960_nan_result(b - a, b, a));
}
static inline uint32_t mo2_mulr(uint32_t s1, uint32_t s2) {
    double a = mo2_fd(s1), b = mo2_fd(s2);
    return mo2_fr(i960_nan_result(a * b, a, b));
}
static inline uint32_t mo2_divr(uint32_t s1, uint32_t s2) {
    double a = mo2_fd(s1), b = mo2_fd(s2);
    return a != 0.0 ? mo2_fr(i960_nan_result(b / a, b, a)) : 0u;
}
/* cmpr s1,s2: the condition code (4 less, 2 equal, 1 greater, 0 unordered) */
static inline uint32_t mo2_cmpr(uint32_t s1, uint32_t s2) {
    double a = mo2_fd(s1), b = mo2_fd(s2);
    return a < b ? 4u : a == b ? 2u : a > b ? 1u : 0u;
}
static inline uint32_t mo2_cvtir(uint32_t i) { return mo2_fr((double)(int32_t)i); }
static inline uint32_t mo2_le32(uint32_t p) {
    return ages_ld8(p) | ages_ld8(p + 1) << 8 | ages_ld8(p + 2) << 16 | ages_ld8(p + 3) << 24;
}

/* The opcodes that only copy argument bytes: { opcode, length, first pair }
 * over smd_pairs[] (argument byte, g7 offset). */
static const struct { uint16_t from, to; } smd_pairs[] = {
    /* 1 */  {1, 0x802}, {2, 0x803},
    /* 2 */  {1, 0x808}, {2, 0x809}, {3, 0x80A}, {4, 0x80B}, {5, 0x80C}, {6, 0x80D},
    /* 3 */  {1, 0x80E}, {2, 0x80F}, {3, 0x816}, {4, 0x817}, {5, 0x818}, {6, 0x819},
             {7, 0x81C}, {8, 0x81D}, {9, 0x81E}, {10, 0x81F}, {11, 0x83F}, {12, 0x850}, {13, 0x851},
    /* 4 */  {1, 0x820}, {2, 0x821}, {4, 0x83E}, {5, 0x82A}, {6, 0x82B}, {7, 0x828}, {8, 0x829},
             {9, 0x823}, {10, 0x843}, {11, 0x85C}, {12, 0x85D}, {13, 0x85E}, {3, 0x822},
             {3, 0x83D}, {14, 0x874}, {15, 0x876}, {16, 0x877}, {17, 0x86B}, {18, 0x875},
    /* 5 */  {1, 0x828}, {2, 0x829}, {3, 0x822}, {4, 0x824}, {5, 0x825}, {6, 0x826},
             {7, 0x827}, {8, 0x812}, {9, 0x813},
    /* 6 */  {1, 0x872}, {2, 0x873}, {3, 0x812}, {4, 0x813}, {5, 0x85F},
    /* 7 */  {1, 0x812}, {2, 0x813},
    /* 9 */  {1, 0x830}, {2, 0x831}, {3, 0x832}, {4, 0x833}, {5, 0x834}, {6, 0x835},
             {7, 0x836}, {8, 0x837}, {9, 0x5BE},
    /* 11 */ {5, 0x848}, {6, 0x849}, {7, 0x84A}, {8, 0x84B}, {9, 0x84E}, {10, 0x84F},
    /* 12 */ {1, 0x840}, {2, 0x841}, {3, 0x842},
    /* 13 */ {1, 0x84D},
    /* 18 */ {1, 0x864}, {2, 0x865}, {3, 0x868}, {4, 0x869}, {5, 0x878}, {6, 0x879},
    /* 19 */ {1, 0x802}, {2, 0x803}, {3, 0x86C}, {4, 0x86D}, {5, 0x86E}, {6, 0x86F},
    /* 20 */ {1, 0x870}, {2, 0x871},
    /* 23 */ {1, 0x87A}, {2, 0x87B},
};
/* Per opcode: the argument length + 1, the first pair and how many. */
static const struct { uint8_t step, first, n; } smd_ops[25] = {
    [1] = {3, 0, 2},   [2] = {7, 2, 6},    [3] = {14, 8, 13},  [4] = {19, 21, 19},
    [5] = {14, 40, 9}, [6] = {6, 49, 5},   [7] = {3, 54, 2},   [8] = {1, 0, 0},
    [9] = {15, 56, 9}, [10] = {6, 0, 0},   [11] = {11, 65, 6}, [12] = {4, 71, 3},
    [13] = {3, 74, 1}, [14] = {15, 0, 0},  [15] = {3, 0, 0},   [17] = {7, 0, 0},
    [18] = {7, 75, 6}, [19] = {7, 81, 6},  [20] = {3, 87, 2},  [21] = {6, 0, 0},
    [22] = {5, 0, 0},  [23] = {3, 89, 2},  [24] = {13, 0, 0},
};

/* Opcode 14 (0x1BE28): blend g7+0x822 toward a target by where the frame
 * (g7+0x5F4) is between two keys. */
static void smd_op14(uint32_t g7, uint32_t p) {
    uint32_t r8 = mo2_le32(p + 1), r10 = mo2_le32(p + 6), r13 = mo2_le32(p + 11);
    int32_t r3 = ages_ld8s(p + 5), r12 = ages_ld8s(p + 10), r6 = ages_ld8s(g7 + 0x822);
    uint32_t r14 = ages_ld32(g7 + 0x5F4);
    uint32_t r4 = mo2_subr(r10, r8), r5 = mo2_subr(r10, r14);
    if (r12 != 0) {
        uint32_t r7 = mo2_subr(r10, r13);
        if (mo2_cmpr(r7, r5) & 4u) {
            r4 = r7; r6 = r12;
        } else {
            r3 = r12; r4 = mo2_subr(r13, r8); r5 = mo2_subr(r13, r14);
        }
    }
    if ((int32_t)r4 < 0) {
        if (!(mo2_cmpr(r4, r5) & 4u)) r5 = r4;
        else if (!(mo2_cmpr(0, r5) & 1u)) r5 = 0;
    } else {
        if (!(mo2_cmpr(0, r5) & 4u)) r5 = 0;
        else if (!(mo2_cmpr(r4, r5) & 1u)) r5 = r4;
    }
    uint32_t f = mo2_mulr(mo2_cvtir((uint32_t)(r6 - r3)), mo2_divr(r4, r5));
    ages_st8(g7 + 0x822, (uint32_t)r3 + gems_cvtri(f));
}

/* Opcode 21 (0x1C008): ease g7+0x822 over the frames counted at g7+0xA20. */
static void smd_op21(uint32_t g7, uint32_t p) {
    uint32_t r3 = ages_ld8(p + 1), r4 = ages_ld8(g7 + 0xA20), r5 = ages_ld8(g7 + 0xA21);
    if (r3 >= r4 || r4 - r5 <= r3) return;
    uint32_t r10 = (uint32_t)ages_ld8s(g7 + 0x822);
    if ((int32_t)r5 <= 1) {
        ages_st8(g7 + 0x822, r10 + (r10 >> 2));
        return;
    }
    uint32_t r6 = mo2_le32(p + 2);
    uint32_t v = mo2_cvtir(r10);
    uint32_t r9 = mo2_mulr(r6, mo2_divr(mo2_cvtir(r4 - r3), mo2_cvtir(r5)));
    uint32_t sum = mo2_addr(mo2_mulr(r6, v), mo2_mulr(r9, v));
    ages_st8(g7 + 0x822, gems_cvtri(sum));
}

/* The opcodes with more than byte copies. Returns the new stream pointer. */
static uint32_t smd_extra(uint32_t g7, uint32_t op, uint32_t p) {
    uint32_t v;
    switch (op) {
    case 2:
        ages_st8(g7 + 0xA00, 1);
        ages_st32(g7 + 0x1A4, ages_ld32(g7 + 0x1A4) | 2u);
        break;
    case 4:
        if (ages_ld32(g7) & 0x40u) ages_st16(g7 + 0x876, 0u - (uint32_t)ages_ld16s(g7 + 0x876));
        ages_st32(g7 + 0x1230, ages_ld32(g7 + 0x1230) + 1u);
        if (ages_ld32(g7) & 0x40u) ages_st8(g7 + 0x820, ages_ld8(0x640E924u + ages_ld8(g7 + 0x820)));
        break;
    case 5:
        ages_st32(g7 + 0x1230, ages_ld32(g7 + 0x1230) + 1u);
        ages_st32(g7 + 0x1234, ages_ld32(g7 + 0x1234) + 1u);
        break;
    case 9:
        v = mo2_mulr(mo2_le32(p + 10), ages_ld32(0x50A000u)) | 0x80000000u;
        ages_st32(g7 + 0x5CC, v);
        ages_st16(g7 + 0x81A, ages_ld8(p + 14));
        break;
    case 10:
        v = ages_ld8(g7 + 0x83C);
        if (v == 0) ages_st32(g7 + 0x838, p);
        ages_st8(g7 + 0x83C, v + 1u);
        break;
    case 11:
        v = mo2_le32(p + 1);
        if (ages_ld32(g7) & 0x40u) v ^= 0x8000u;
        ages_st32(g7 + 0x844, v);
        ages_st16(g7 + 0x6EE, v & 0x7FFFu);
        break;
    case 13: ages_st16(g7 + 0x858, ages_ld8(p + 2)); break;
    case 14: smd_op14(g7, p); break;
    case 21: smd_op21(g7, p); break;
    case 16:
        v = ages_ld8(p + 1);
        ages_st32(g7 + 0x854, p + 2);
        ages_st16(g7 + 0x800, ages_ld8(p + v * 2u));
        return p + 2 + v * 2u;
    }
    return p + smd_ops[op].step;
}

/* The fixed part before the stream (0x1B8D0..0x1B990). */
static uint32_t smd_head(uint32_t g7, uint32_t mot) {
    static const uint16_t to[12] = { 0x804, 0x805, 0x806, 0x807, 0x860, 0x861,
                                     0x862, 0x863, 0x810, 0x811, 0x814, 0x815 };
    uint32_t p = ages_ld32(0x6411E94u + mot * 4u);
    uint32_t n = ages_ld16(ages_ld32(0x6C00004u + mot * 4u));
    ages_st16(g7 + 0x800, n);
    ages_st16(g7 + 0x80A, n);
    ages_st16(g7 + 0x80C, n);
    ages_st32(g7 + 0xBD4, ages_ld32(g7 + 0x1A4));
    ages_st32(g7 + 0xC30, ages_ld32(g7 + 0x804));
    for (int i = 0; i < 12; i++) ages_st8(g7 + to[i], ages_ld8(p + (uint32_t)i));
    uint32_t f = ages_ld32(g7 + 0x804) & ~(1u << 21);
    if (f & 0x40u) ages_st16(g7 + 0x812, 0x8000u);
    if ((f & 0x100u) && (f & 0x8000u)) f = (f & ~0x8000u) | 0x10000u;
    ages_st32(g7 + 0x1A4, f);
    ages_st32(g7 + 0x720, ages_ld32(g7 + 0x860));
    return p + 12u;
}

static void smd_clear(uint32_t g7) {
    static const uint16_t h[] = { 0x87A, 0x802, 0x86C, 0x870, 0x812, 0x850, 0x818, 0x872 };
    static const uint16_t b[] = { 0xA00, 0x83C, 0x840, 0x823, 0xA25, 0xA28 };
    static const uint16_t w[] = { 0x844, 0x854, 0x864, 0xACC };
    for (size_t i = 0; i < sizeof h / sizeof h[0]; i++) ages_st16(g7 + h[i], 0);
    for (size_t i = 0; i < sizeof b / sizeof b[0]; i++) ages_st8(g7 + b[i], 0);
    for (size_t i = 0; i < sizeof w / sizeof w[0]; i++) ages_st32(g7 + w[i], 0);
}

static uint32_t afn_set_mot_dat(int entry) {
    uint32_t g7 = AGES_G(7);
    smd_clear(g7);
    if (AGES_G(0) == 0) {
        MO2_CC(2u);
    } else {
        uint32_t p = smd_head(g7, AGES_G(0));
        for (;;) {
            uint32_t op = ages_ld8(p);
            if (op == 0 || op == 8) { p += op == 8; break; }
            for (uint32_t i = 0; i < smd_ops[op].n; i++) {
                uint32_t k = smd_ops[op].first + i;
                ages_st8(g7 + smd_pairs[k].to, ages_ld8(p + smd_pairs[k].from));
            }
            p = smd_extra(g7, op, p);
        }
        ages_st32(g7 + 0x82C, p);
        ages_st32(g7 + 0x700, p);
        ages_st8(g7 + 0xA20, 0);
        if (ages_ld32(g7) & 0x40u) {
            ages_st16(g7 + 0x812, 0u - ages_ld16(g7 + 0x812));
            ages_st16(g7 + 0x82A, 0u - ages_ld16(g7 + 0x82A));
            ages_st32(g7 + 0x830, ages_ld32(g7 + 0x830) ^ 0x80000000u);
            MO2_CC(0u);
        } else {
            MO2_CC(2u);
        }
    }
    if (entry) ages_i960_ret();
    return 0;
}
