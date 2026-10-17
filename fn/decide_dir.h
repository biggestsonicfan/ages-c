/* decide_dir: i960 0x28034, PS2 0x1E0A80. g7/g8 = the fighters at 0x500804 /
 * 0x500808. Unless either has bit 29 or 18 of rob+0x1A4, a follow rate (0.4;
 * 0.3 when the thrower, rob+0x804 bit 8, has kind 5 at +0x821; else 0.85 on
 * bit 14 of g7's +0x1A4; 0 unless g7's +0x5F4 < +0x7D8) goes to g13+0x270,
 * and it stops there when that is above g13+0x168. Then g13+0xD4..0xDC = the
 * unit x/z vector from g7 to g8 (COP 0x30), or zeros.
 *
 * The colig_ FPU helpers below are the group's: the i960's own arithmetic on
 * register words, src1 first as the i960 writes them. */
#pragma once

static uint32_t colig_addr(uint32_t s1, uint32_t s2) {
    double a = i960_single_to_double(s1), b = i960_single_to_double(s2);
    return i960_double_to_single(i960_nan_result(a + b, a, b));
}

static uint32_t colig_subr(uint32_t s1, uint32_t s2) {   /* s2 - s1 */
    double a = i960_single_to_double(s1), b = i960_single_to_double(s2);
    return i960_double_to_single(i960_nan_result(b - a, b, a));
}

static uint32_t colig_mulr(uint32_t s1, uint32_t s2) {
    double a = i960_single_to_double(s1), b = i960_single_to_double(s2);
    return i960_double_to_single(i960_nan_result(a * b, a, b));
}

static uint32_t colig_divr(uint32_t s1, uint32_t s2) {   /* s2 / s1; /0 is 0 */
    double a = i960_single_to_double(s1), b = i960_single_to_double(s2);
    if (a == 0.0) return 0;
    return i960_double_to_single(i960_nan_result(b / a, b, a));
}

/* cmpr s1, s2: 4 less, 2 equal, 1 greater, 0 unordered. */
static uint32_t colig_cmpr(uint32_t s1, uint32_t s2) {
    double a = i960_single_to_double(s1), b = i960_single_to_double(s2);
    return a < b ? 4u : a == b ? 2u : a > b ? 1u : 0u;
}

static uint32_t colig_follow_rate(uint32_t g7, uint32_t g8) {
    uint32_t r12 = 0x3ECCCCCDu;                     /* 0.4f */
    uint32_t r6 = ages_ld32(g7 + 0x804u), r7 = ages_ld32(g8 + 0x804u);
    uint32_t kind = (r6 & 0x100u) ? ages_ld8(g7 + 0x821u) : ages_ld8(g8 + 0x821u);
    if (((r6 | r7) & 0x100u) && kind == 5u)
        r12 = 0x3E99999Au;                          /* 0.3f */
    else if (ages_ld32(g7 + 0x1A4u) & 0x4000u)
        r12 = 0x3F59999Au;                          /* 0.85f */
    if (colig_cmpr(ages_ld32(g7 + 0x5F4u), ages_ld32(g7 + 0x7D8u)) != 4u)
        r12 = 0;
    return r12;
}

static uint32_t afn_decide_dir(int entry) {
    uint32_t g7 = ages_ld32(0x500804u), g8 = ages_ld32(0x500808u), g13 = AGES_G(13);
    uint32_t cc = 2u;
    AGES_G(7) = g7;
    AGES_G(8) = g8;
    if (((ages_ld32(g7 + 0x1A4u) | ages_ld32(g8 + 0x1A4u)) & 0x20040000u) == 0) {
        uint32_t r12 = colig_follow_rate(g7, g8);
        ages_st32(g13 + 0x270u, r12);
        cc = colig_cmpr(r12, ages_ld32(g13 + 0x168u));
        if (cc == 1u) goto out;
    }
    uint32_t v[3];
    v[0] = colig_subr(ages_ld32(g7 + 0x1F4u), ages_ld32(g8 + 0x1F4u));
    v[1] = 0;
    v[2] = colig_subr(ages_ld32(g7 + 0x1FCu), ages_ld32(g8 + 0x1FCu));
    if (((v[0] | v[2]) & 0x7FFFFFFFu) == 0) {
        v[0] = v[2] = 0;
        cc = 2u;
    } else {
        ages_cop_w(0x18003030u);
        for (int i = 0; i < 3; i++) ages_cop_w(v[i]);
        for (int i = 0; i < 3; i++) v[i] = ages_cop_r();
        cc = 4u;
    }
    ages_st32(g13 + 0xD4u, v[0]);
    ages_st32(g13 + 0xD8u, v[1]);
    ages_st32(g13 + 0xDCu, v[2]);
out:
    AGES_AC = (AGES_AC & ~7u) | cc;
    if (entry) ages_i960_ret();
    return 0;
}
