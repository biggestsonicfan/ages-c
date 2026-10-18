/* VF2 osage_dsp's record loop: i960 0x644EC..0x6453C. PS2
 * sysCopPatchOsageDsp 0x18BF40 (EXT 52; decomp/VF2/FN/ext52_*).
 *
 * A fighter's sway chain (g13 the chain block, g7 its fighter): walk the
 * script at 0x44(g13) into the records at 0x48(g13). Each word is a record
 * type, copied to the record, and type 1..5 runs its builder through the
 * table at 0x64524 (0x64864 matrix, 0x64E5C collision, 0x65318 etc,
 * 0x64CA0 tsukene, 0x651BC osage); the table at 0x640C0 then moves g9 and g8
 * past the record. A 0 word ends it.
 *
 * The PS2 runs the builders in its own C on its own TGP; these are the
 * i960's, TGP word for word on the bus, with the globals (g0..g6) each
 * leaves. The loop's r3/r4/r5 are left as the i960 leaves them at 0x6453C:
 * r3 the 0 word, r4/r5 the last record's step (ldl overwrites the callx
 * target in r4). */
#pragma once

#include "set_obj.h"

/* The i960's FPU on register words, src1 first as the i960 writes them. */
static uint32_t vf2_os_addr(uint32_t s1, uint32_t s2)
{
    double a = i960_single_to_double(s1), b = i960_single_to_double(s2);
    return i960_double_to_single(i960_nan_result(a + b, a, b));
}

static uint32_t vf2_os_subr(uint32_t s1, uint32_t s2)   /* s2 - s1 */
{
    double a = i960_single_to_double(s1), b = i960_single_to_double(s2);
    return i960_double_to_single(i960_nan_result(b - a, b, a));
}

static uint32_t vf2_os_mulr(uint32_t s1, uint32_t s2)
{
    double a = i960_single_to_double(s1), b = i960_single_to_double(s2);
    return i960_double_to_single(i960_nan_result(a * b, a, b));
}

static uint32_t vf2_os_divr(uint32_t s1, uint32_t s2)   /* s2 / s1; /0 is 0 */
{
    double a = i960_single_to_double(s1), b = i960_single_to_double(s2);
    if (a == 0.0) return 0;
    return i960_double_to_single(i960_nan_result(b / a, b, a));
}

#define VF2_OS_SIGN 0x80000000u

/* Three FIFO words, as `ld (g11)[g12]` three times. */
static void vf2_os_r3(uint32_t *d)
{
    d[0] = vf2_tgp_r();
    d[1] = vf2_tgp_r();
    d[2] = vf2_tgp_r();
}

/* TGP cmd, then a vector. */
static void vf2_os_w3(uint32_t cmd, const uint32_t *v)
{
    vf2_tgp_w(cmd);
    vf2_tgp_w(v[0]);
    vf2_tgp_w(v[1]);
    vf2_tgp_w(v[2]);
}

/* TGP cmd, a vector, then its three results into d (d may be v). */
static void vf2_os_xf(uint32_t cmd, uint32_t *d, const uint32_t *v)
{
    vf2_os_w3(cmd, v);
    vf2_os_r3(d);
}

/* TGP cmd, then (word, 0x4(g7) byte, idx * 12): a fighter's matrix. */
static void vf2_os_mat(uint32_t cmd, uint32_t idx)
{
    vf2_tgp_w(cmd);
    vf2_tgp_w(ages_ld8(AGES_G(7) + 4));
    vf2_tgp_w(idx * 12u);
}

static void vf2_os_cmd1(uint32_t cmd, uint32_t arg)
{
    vf2_tgp_w(cmd);
    vf2_tgp_w(arg);
}

/* n FIFO words to a + 0, 4, .. */
static void vf2_os_pop(uint32_t a, int n)
{
    for (int i = 0; i < n; i++) ages_st32(a + (uint32_t)i * 4u, vf2_tgp_r());
}

static bool vf2_os_flag(uint32_t bit)
{
    return (ages_ld32(AGES_G(13)) >> bit) & 1u;
}

/* `ld 0x5001e4,r15; st g,0x90e000(r15); addo 4; stob r15,0x5001e4`: one
 * word into the byte-indexed ring, then the TGP command that takes it. */
static void vf2_os_ring(uint32_t g, uint32_t cmd)
{
    if (g == 0) return;
    uint32_t i = ages_ld32(0x5001E4);
    ages_st32(0x90E000u + i, g);
    ages_st8(0x5001E4, (uint8_t)(i + 4u));
    vf2_tgp_w(cmd);
}

/* 0x64918: three words into the ring, wrapping on bit 8 between them. */
static void vf2_os_ring3(const uint32_t *g)
{
    uint32_t i = ages_ld32(0x5001E4);
    ages_st32(0x90E000u + i, g[0]);
    i = (i + 4u) & ~0x100u;
    ages_st32(0x90E000u + i, g[1]);
    i = (i + 4u) & ~0x100u;
    ages_st32(0x90E000u + i, g[2]);
    ages_st8(0x5001E4, (uint8_t)(i + 4u));
    vf2_tgp_w(0x37806F6Fu);
}

/* 0x64864 type 1, first half: the matrix's lights and the camera height. */
static void vf2_os_matrix_head(uint32_t g9, uint32_t g13)
{
    vf2_tgp_w(0x00800101u);
    vf2_os_mat(0x1B003636u, ages_ld32(g9));
    ages_ldn(&AGES_G(0), g9 + 4, 3);
    vf2_os_ring(AGES_G(0), 0x37006E6Eu);
    vf2_os_ring(AGES_G(1), 0x36806D6Du);
    vf2_os_ring(AGES_G(2), 0x36006C6Cu);
    ages_ldn(&AGES_G(0), g9 + 0x10, 3);
    vf2_os_ring3(&AGES_G(0));
    vf2_os_cmd1(0x21804343u, 0);
    vf2_tgp_w(0x07800F0Fu);
    vf2_os_r3(&AGES_G(0));
    AGES_G(1) ^= VF2_OS_SIGN;
    AGES_G(0) = ages_ld32(g13 + 0xE4);
    AGES_G(1) = vf2_os_addr(AGES_G(1), AGES_G(0));
    ages_st32(g13 + 0xF4, AGES_G(1));
    vf2_tgp_w(0x06000C0Cu);
    vf2_tgp_w(0x09801313u);
    vf2_tgp_w(0x09801313u);
    vf2_tgp_w(0x09801313u);
    (void)vf2_tgp_r();
    vf2_os_cmd1(0x21804343u, 1);
    ages_ldn(&AGES_G(4), g13 + 0x138, 3);
    uint32_t r4 = ages_ld32(0x50A000) ^ VF2_OS_SIGN;
    r4 = vf2_os_mulr(ages_ld32(g9 + 0x1C), r4);
    AGES_G(5) = vf2_os_addr(AGES_G(5), r4);
}

/* 0x649F4..0x64C9C: the chain's root, its stored matrix, three of them out. */
static void vf2_os_matrix_tail(uint32_t g8, uint32_t g13)
{
    vf2_tgp_w(0x00800101u);
    vf2_tgp_w(0x06800D0Du);
    vf2_os_xf(0x14802929u, &AGES_G(4), &AGES_G(4));
    if (vf2_os_flag(0)) ages_stn(g13 + 0x114, &AGES_G(4), 3);
    vf2_tgp_w(0x2B805757u);
    vf2_os_r3(&AGES_G(0));
    ages_stn(g13 + 0xE8, &AGES_G(0), 3);
    vf2_tgp_w(0x01000202u);
    uint32_t r10 = ages_ld32(g13 + 0x50);
    uint32_t m = g13 + r10;
    if (vf2_os_flag(0)) {
        vf2_tgp_w(0x01800303u);
    } else {
        uint32_t w[12];
        vf2_tgp_w(0x05800B0Bu);
        ages_ldn(w, m, 12);
        for (int i = 0; i < 12; i++) vf2_tgp_w(w[i]);
    }
    vf2_tgp_w(0x02800505u);
    vf2_os_pop(g8 + 0x30, 12);
    vf2_os_w3(0x07000E0Eu, &AGES_G(4));
    vf2_tgp_w(0x02800505u);
    vf2_os_pop(g8 + 0x60, 12);
    vf2_os_cmd1(0x22004444u, 0);
    vf2_tgp_w(0x02800505u);
    vf2_os_pop(m, 12);
    ages_st32(g13 + 0x50, r10 + 0x30u);
    vf2_tgp_w(0x01000202u);
    vf2_tgp_w(0x00800101u);
    vf2_os_cmd1(0x22804545u, 0);
    vf2_tgp_w(0x02800505u);
    vf2_os_pop(g8, 12);
    vf2_tgp_w(0x01000202u);
}

static void vf2_os_matrix(uint32_t g9, uint32_t g8, uint32_t g13)
{
    vf2_os_matrix_head(g9, g13);
    vf2_os_matrix_tail(g8, g13);
}

/* 0x64CA0 type 4 (tsukene), its subtype at 0(g9) through the bx table at
 * 0x64CB0: 0 under the fighter's matrix, 1 the last point again (0xFC(g13)),
 * 2 under the parent's, 3 under the TGP's own. */
static void vf2_os_tsukene(uint32_t g9, uint32_t g8, uint32_t g13)
{
    uint32_t sub = ages_ld32(g9), idx = ages_ld32(g9 + 4);
    if (sub > 3) return;
    if (sub == 1) {
        ages_ldn(&AGES_G(0), g13 + 0xFC, 3);
    } else {
        vf2_tgp_w(0x00800101u);
        if (sub == 0) vf2_os_mat(0x1B003636u, idx);
        ages_ldn(&AGES_G(0), g9 + 8, 3);
        if (sub == 0) vf2_os_xf(0x14802929u, &AGES_G(0), &AGES_G(0));
        vf2_os_cmd1(0x22004444u, sub == 3 ? 2u : 1u);
        if (sub == 2) vf2_os_mat(0x1B803737u, idx);
        vf2_os_xf(0x14802929u, &AGES_G(0), &AGES_G(0));
        if (sub == 2) vf2_os_cmd1(0x21804343u, 2);
        vf2_tgp_w(0x01000202u);
        ages_stn(g13 + 0xFC, &AGES_G(0), 3);
    }
    ages_stn(g8, &AGES_G(0), 3);
    ages_stn(g13 + 0x108, &AGES_G(0), 3);
}

/* 0x64E5C (frozen chains only): one axis's offset (0x20/0x24(g9) -/+ 0x4000)
 * as the TGP's sin and cos into o, o + 4, and sin * 0x38(g8) into o + 8. */
static void vf2_os_coli_sc(uint32_t g8, uint32_t ang, uint32_t o)
{
    vf2_os_cmd1(0x11002222u, ang);
    ages_st32(g8 + o, vf2_tgp_r());
    vf2_os_cmd1(0x10802121u, ang);
    ages_st32(g8 + o + 4, vf2_tgp_r());
    ages_st32(g8 + o + 8, vf2_os_mulr(ages_ld32(g8 + o), ages_ld32(g8 + 0x38)));
}

static void vf2_os_coli_head(uint32_t g9, uint32_t g8)
{
    static const uint8_t in[3] = { 0x0C, 0x18, 0x1C }, out[3] = { 0x1C, 0x30, 0x38 };
    for (int i = 0; i < 3; i++) {
        uint32_t r3 = ages_ld32(g9 + in[i]);
        ages_st32(g8 + out[i], r3);
        ages_st32(g8 + out[i] + 4, vf2_os_mulr(r3, r3));
    }
    vf2_os_coli_sc(g8, ages_ld32(g9 + 0x20) - 0x4000u, 0x40);
    vf2_os_coli_sc(g8, ages_ld32(g9 + 0x24) + 0x4000u, 0x4C);
    ages_st32(g8 + 0x54, ages_ld32(g8 + 0x54) ^ VF2_OS_SIGN);
}

/* 0x65094 / 0x65118: one end of the capsule (scale at a(g9), height added
 * from b(g9)) to out(g8), and its slope against the axis at in(g8) to q(g8). */
static void vf2_os_coli_end(uint32_t g9, uint32_t g8, const uint32_t *k, const uint8_t *o)
{
    uint32_t r4, r5, r6;
    vf2_tgp_w(0x2D805B5Bu);
    vf2_tgp_w(ages_ld32(g9 + o[0]));
    vf2_tgp_w(AGES_G(4));
    vf2_tgp_w(AGES_G(5));
    r4 = vf2_tgp_r();
    r5 = vf2_tgp_r();
    vf2_tgp_w(0x2C805959u);
    vf2_tgp_w(r4);
    vf2_tgp_w(k[0]);
    vf2_tgp_w(r5);
    vf2_tgp_w(k[1]);
    r6 = vf2_os_addr(vf2_tgp_r(), ages_ld32(g9 + o[1]));
    ages_st32(g8 + o[2], r4);
    ages_st32(g8 + o[2] + 4, r5);
    ages_st32(g8 + o[2] + 8, r6);
    uint32_t r8 = ages_ld32(g8 + o[3]), r9 = ages_ld32(g8 + o[3] + 4);
    uint32_t r10 = ages_ld32(g8 + o[3] + 8);
    AGES_G(0) = vf2_os_mulr(r10, r4);
    AGES_G(1) = vf2_os_mulr(r6, r8);
    AGES_G(2) = vf2_os_mulr(r9, r4);
    AGES_G(3) = vf2_os_mulr(r5, r8);
    AGES_G(0) = vf2_os_subr(AGES_G(0), AGES_G(1));
    AGES_G(2) = vf2_os_subr(AGES_G(2), AGES_G(3));
    ages_st32(g8 + o[4], vf2_os_divr(AGES_G(2), AGES_G(0)));
}

/* 0x64E5C type 2 (collision capsule). */
static void vf2_os_coli(uint32_t g9, uint32_t g8, uint32_t g13)
{
    static const uint8_t end0[5] = { 0x2C, 0x30, 0x60, 0x40, 0x58 };
    static const uint8_t end1[5] = { 0x34, 0x38, 0x6C, 0x4C, 0x5C };
    uint32_t v[3], k[2], w[4];
    if (vf2_os_flag(0)) vf2_os_coli_head(g9, g8);
    vf2_tgp_w(0x00800101u);
    vf2_os_mat(0x1B003636u, ages_ld32(g9));
    v[0] = ages_ld32(g9 + 4), v[1] = 0, v[2] = ages_ld32(g9 + 8);
    vf2_os_xf(0x14802929u, &AGES_G(4), v);
    v[0] = ages_ld32(g9 + 0x10), v[2] = ages_ld32(g9 + 0x14);
    vf2_os_xf(0x14802929u, &AGES_G(0), v);
    vf2_os_cmd1(0x22004444u, 1);
    vf2_os_xf(0x14802929u, &AGES_G(4), &AGES_G(4));
    ages_stn(g8 + 0x10, &AGES_G(4), 3);
    vf2_os_xf(0x14802929u, &AGES_G(0), &AGES_G(0));
    ages_stn(g8 + 0x24, &AGES_G(0), 3);
    vf2_os_mat(0x1B803737u, ages_ld32(g9 + 0x28));
    vf2_tgp_w(0x2B005656u);
    vf2_os_r3(v);
    AGES_G(4) = v[0], AGES_G(5) = v[1];
    vf2_tgp_w(0x07800F0Fu);
    vf2_os_r3(v);
    k[0] = v[0], k[1] = v[1];
    vf2_tgp_w(0x2D005A5Au);
    vf2_tgp_w(AGES_G(4));
    vf2_tgp_w(AGES_G(5));
    AGES_G(4) = vf2_tgp_r();
    AGES_G(5) = vf2_tgp_r();
    vf2_os_coli_end(g9, g8, k, end0);
    vf2_os_coli_end(g9, g8, k, end1);
    vf2_tgp_w(0x01000202u);
    ages_ldn(w, g13 + 0xE8, 4);
    ages_stn(g8, w, 4);
}

/* TGP 0x2E005C5C: two vectors interleaved, three results into d. */
static void vf2_os_mix(uint32_t *d, const uint32_t *a, const uint32_t *b)
{
    vf2_tgp_w(0x2E005C5Cu);
    for (int i = 0; i < 3; i++) {
        vf2_tgp_w(a[i]);
        vf2_tgp_w(b[i]);
    }
    vf2_os_r3(d);
}

static void vf2_os_copy(uint32_t to, uint32_t from, int n)
{
    uint32_t w[3];
    ages_ldn(w, from, n);
    ages_stn(to, w, n);
}

/* 0x651BC type 5 (osage, the swaying point). */
static void vf2_os_osage(uint32_t g9, uint32_t g8, uint32_t g13)
{
    uint32_t r[3];
    if (vf2_os_flag(0)) {
        ages_ldn(&AGES_G(0), g13 + 0x114, 3);
        ages_ldn(r, g9 + 0x28, 3);
        vf2_os_mix(&AGES_G(0), r, &AGES_G(0));
        vf2_os_xf(0x18003030u, &AGES_G(0), &AGES_G(0));
        vf2_tgp_w(0x2F005E5Eu);
        vf2_os_xf(ages_ld32(g9 + 0x24), &AGES_G(0), &AGES_G(0));
        ages_ldn(r, g13 + 0x108, 3);
        vf2_os_mix(r, &AGES_G(0), r);
        ages_stn(g13 + 0x108, r, 3);
        ages_stn(g8, r, 3);
        vf2_os_copy(g8 + 0x0C, g9 + 0x0C, 3);
        vf2_os_copy(g8 + 0x18, g9 + 0x18, 1);
        vf2_os_copy(g8 + 0x1C, g9 + 0x24, 1);
        vf2_os_copy(g8 + 0x20, g9 + 0x28, 3);
        return;
    }
    uint32_t f = ages_ld32(g13), s = ages_ld32(g13 + 0x144);
    if (f & 0x80u) {
        r[0] = r[1] = r[2] = 0;
    } else {
        ages_ldn(r, g9 + ((f & 2u) ? 0x34u : 0x28u), 3);
        if (s != 0) {
            vf2_tgp_w(0x2F005E5Eu);
            vf2_os_xf(s, r, r);
        }
    }
    ages_stn(g8 + 0x20, r, 3);
}

/* 0x65318 type 3 (etc): the record's kind and its stiffness (0.8 when
 * 0(g13) bit 6), kind 1 when bit 4. */
static void vf2_os_etc(uint32_t g9, uint32_t g8)
{
    ages_st32(g8, ages_ld32(g9));
    uint32_t r3 = ages_ld32(g9 + 4);
    if (vf2_os_flag(6)) r3 = 0x3F4CCCCDu;
    ages_st32(g8 + 4, r3);
    if (vf2_os_flag(4)) ages_st32(g8, 1);
}

static void vf2_os_record(uint32_t type)
{
    uint32_t g9 = AGES_G(9), g8 = AGES_G(8), g13 = AGES_G(13);
    switch (type) {
    case 1: vf2_os_matrix(g9, g8, g13); break;
    case 2: vf2_os_coli(g9, g8, g13); break;
    case 3: vf2_os_etc(g9, g8); break;
    case 4: vf2_os_tsukene(g9, g8, g13); break;
    case 5: vf2_os_osage(g9, g8, g13); break;
    default: break;
    }
}

/* The loop's `ldl 0x640c0[r3*8],r4; addo r4,g9,g9; addo r5,g8,g8`. */
static void vf2_os_step(uint32_t type)
{
    AGES_R(4) = ages_ld32(0x640C0u + type * 8u);
    AGES_R(5) = ages_ld32(0x640C4u + type * 8u);
    AGES_G(9) += AGES_R(4);
    AGES_G(8) += AGES_R(5);
}

/* EXT 52 at 0x644EC (`ld 0x48(g13),g8`): resume at 0x6453C. */
static uint32_t afn_vf2_osage_dsp(int entry)
{
    (void)entry;
    AGES_G(8) = ages_ld32(AGES_G(13) + 0x48);
    for (;;) {
        uint32_t type = ages_ld32(AGES_G(9));
        AGES_G(9) += 4;
        ages_st32(AGES_G(8), type);
        AGES_G(8) += 4;
        AGES_R(3) = type;
        if (type == 0) break;
        vf2_os_record(type);
        vf2_os_step(type);
    }
    vf2_cmpo(0, 0);
    return 0xA0;   /* 0x6453C */
}
