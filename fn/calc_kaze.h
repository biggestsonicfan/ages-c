/* calc_kaze: i960 0x63B80, PS2 0x1FBAC0 (helper of osage_dsp).
 *
 * The wind on a fighter's sway chains. Stages 0, 4, 5, 1 and 8 always blow
 * (direction g13+0x134, g13 flag 10 picking the strong or weak entry of the
 * table at 0x63A58 by character); elsewhere only the winner on some game
 * modes. The phase g13+0x130 steps; g13+0x138 takes the wind vector, its
 * middle word capped at the word 0x50A000. No wind: flag 9 and all zero. */
#pragma once

static void kz_flag10(uint32_t g13, uint32_t dir, int on) {
    ages_st32(g13 + 0x134u, dir);
    uint32_t w = ages_ld32(g13);
    ages_st32(g13, on ? (w | 0x400u) : (w & ~0x400u));
}

static uint32_t kz_cmp(uint32_t a, uint32_t b) {
    return a < b ? 4u : a == b ? 2u : 1u;
}

/* Returns -1 to blow, else the condition code of the calm path. */
static int kz_stage(uint32_t g13) {
    uint32_t w = ages_ld32(g13);
    if (w & 0x50u) return 2;
    kz_flag10(g13, 0xF000u, 1);
    uint32_t st = ages_ld8(0x500064u);
    if (st == 8) { kz_flag10(g13, 0x7000u, 1); return -1; }
    if (st == 0 || st == 4) { kz_flag10(g13, 0xF000u, 1); return -1; }
    if (st == 5) { kz_flag10(g13, 0xC000u, 0); return -1; }
    if (st == 1) { kz_flag10(g13, 0x8000u, 0); return -1; }
    if (!((0x30000u >> (ages_ld8(0x500031u) & 31u)) & 1u)) return 0;
    uint32_t win = ages_ld8(0x500065u), me = ages_ld8(AGES_G(7) + 4u);
    if (win != me) return (int)kz_cmp(win, me);
    if (ages_ld8(AGES_G(7) + 0x1B1u) == 1u) return 2;
    return -1;
}

static uint32_t kz_blow(uint32_t g13) {
    ages_st32(g13, ages_ld32(g13) | 0x200u);
    uint32_t t = ages_ld32(0x63A58u + ages_ld8(AGES_G(7) + 0x1B0u) * 4u);
    if (!(ages_ld32(g13) & 0x400u)) t += 8;
    AGES_G(0) = ages_ld16(t);
    AGES_G(2) = ages_ld32(t + 4u);
    uint32_t ph = ages_ld32(g13 + 0x130u) + AGES_G(0);
    ages_st32(g13 + 0x130u, ph);
    ages_cop_w(0x10802121u);
    ages_cop_w(ph);
    float c = ages_u2f(ages_cop_r()) + 2.0f;
    float s = c * ages_u2f(AGES_G(2));
    s = s * ages_u2f(ages_ld32(0x50A000u));
    uint32_t a = ages_ld32(g13 + 0x134u), v[3], r9 = ages_f2u(s);
    ages_cop_w(0x12802525u); ages_cop_w(a); ages_cop_w(r9);
    v[0] = ages_cop_r();
    ages_cop_w(0x12002424u); ages_cop_w(a); ages_cop_w(r9);
    v[2] = ages_cop_r();
    uint32_t wu = ages_ld32(0x50A000u);
    float w = ages_u2f(wu);
    v[1] = s > w ? wu : r9;
    ages_stn(g13 + 0x138u, v, 3);
    return s < w ? 4u : s == w ? 2u : s > w ? 1u : 0u;
}

static uint32_t afn_calc_kaze(int entry) {
    uint32_t g13 = AGES_G(13);
    int cc = kz_stage(g13);
    if (cc < 0) {
        cc = (int)kz_blow(g13);
    } else {
        static const uint32_t zero[3] = { 0, 0, 0 };
        ages_st32(g13, ages_ld32(g13) & ~0x200u);
        ages_st32(g13 + 0x130u, 0);
        ages_stn(g13 + 0x138u, zero, 3);
    }
    AGES_AC = (AGES_AC & ~7u) | (uint32_t)cc;
    if (entry) ages_i960_ret();
    return 0;
}
