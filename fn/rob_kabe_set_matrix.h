/* rob_kabe_set_matrix: i960 0x76A98, PS2 0x1CCF40 (helper of
 * rob_kabe_kage_disp).
 *
 * Sets up the two walls fighter g7 casts its shadow on. Flags at 0x50A3F8:
 * bit 0 / 1 = no z / x wall shadow (fighter beyond the wall distance at
 * 0x50A400, or the light parallel to the wall), bit 2 / 3 = camera beyond
 * that wall. Stores the signed walls (0x50A3E0 / E4), the shadow offsets
 * (0x50A3EC / F4) and stretches (0x50A3E8 / F0, at most 3), then COP
 * matrices 7 and 6 (x wall), 5 and 4 (z wall) and 3. g1 = the stage's
 * flags (bit 4: light angle from 0x50A3FC instead of the camera's). Board
 * quirk kept: with the z wall on the + side, its camera check compares
 * r13, which then holds the sign mask (-0.0), instead of loading the
 * camera's +0x18; the z wall's -1 / sin is 0
 * when the sine is 0 (the board's divr by 0). */
#pragma once

#define RKM_F 0x50A3F8u
#define RKM_CC(cc) (AGES_AC = (AGES_AC & ~7u) | (cc))

static void rkm_flag(uint32_t bit) {
    ages_st32(RKM_F, ages_ld32(RKM_F) | bit);
}

static uint32_t rkm_cmpr(float a, float b) {
    return a < b ? 4u : a == b ? 2u : a > b ? 1u : 0u;
}

/* Store the wall with the given sign at `at`; flag `bit` unless the
 * camera coordinate is on the fighter's side of it. */
static uint32_t rkm_wall(uint32_t sign, uint32_t at, uint32_t cam,
                         uint32_t bit) {
    uint32_t w = (ages_ld32(0x50A400u) & 0x7FFFFFFFu) | (sign & 0x80000000u);
    ages_st32(at, w);
    uint32_t cc = rkm_cmpr(ages_u2f(cam), ages_u2f(w));
    if (cc != ((w & 0x80000000u) ? 1u : 4u)) rkm_flag(bit);
    return w;
}

/* 1 / v at most 3 to `at`; v == +-0 sets flag `bit` instead. */
static void rkm_stretch(uint32_t v, uint32_t at, uint32_t bit) {
    if ((v & 0x7FFFFFFFu) == 0) { rkm_flag(bit); return; }
    float s = 1.0f / ages_u2f(v);
    if (!(3.0f >= s)) s = 3.0f;
    ages_stf(at, s);
}

/* divr: the board's x / 0 is 0 (m2-hle2's i960, fdivr). */
static float rkm_divr(float x, float y) {
    return y != 0.0f ? x / y : 0.0f;
}

static uint32_t rkm_cop2(uint32_t cmd, uint32_t a) {
    ages_cop_w(cmd);
    ages_cop_w(a);
    return ages_cop_r();
}

/* 0x76C1C-0x76D94: offsets and stretches. */
static void rkm_planes(uint32_t wx, uint32_t wz) {
    uint32_t p[3];
    ages_ldn(p, AGES_G(7) + 0x1F4u, 3);
    uint32_t a = (uint32_t)ages_ld16s(0x50A3FCu);
    float d = ages_ldf(0x50A400u) - ages_u2f(p[2] ^ (wx & 0x80000000u));
    float t = d * ages_u2f(rkm_cop2(0x11802323u, a) ^ 0x80000000u);
    uint32_t tb = ages_f2u(t) ^ (ages_ld32(0x50A3E4u) & 0x80000000u);
    ages_stf(0x50A3ECu, ages_u2f(p[0]) + ages_u2f(tb));
    rkm_stretch(rkm_cop2(0x11002222u, a), 0x50A3E8u, 2u);

    ages_ldn(p, AGES_G(7) + 0x1F4u, 3);
    a = (uint32_t)ages_ld16s(0x50A3FCu);
    d = ages_ldf(0x50A400u) - ages_u2f(p[0] ^ (wz & 0x80000000u));
    t = d * rkm_divr(-1.0f, ages_u2f(rkm_cop2(0x11802323u, a)));
    tb = ages_f2u(t) ^ (wz & 0x80000000u);
    ages_stf(0x50A3F4u, ages_u2f(p[2]) + ages_u2f(tb));
    rkm_stretch(rkm_cop2(0x10802121u, a + 0x8000u), 0x50A3F0u, 1u);
}

static void rkm_cop4(uint32_t cmd, uint32_t x, uint32_t y, uint32_t z) {
    ages_cop_w(cmd);
    ages_cop_w(x);
    ages_cop_w(y);
    ages_cop_w(z);
}

static void rkm_begin(void) {
    ages_cop_w(0x09801313u);
    ages_cop_w(0x09801313u);
    ages_cop_w(0x09801313u);
    (void)ages_cop_r();
    ages_cop_w(0x00800101u);
    ages_cop_w(0x01800303u);
}

static void rkm_end(uint32_t slot) {
    ages_cop_w(0x21804343u);
    ages_cop_w(slot);
    ages_cop_w(0x01000202u);
}

/* COP cmd (a, the fighter's word at off) -> result. */
static uint32_t rkm_turn(uint32_t cmd, uint32_t a, uint32_t off) {
    ages_cop_w(cmd);
    ages_cop_w(a);
    ages_cop_w(ages_ld32(AGES_G(7) + off));
    return ages_cop_r();
}

/* Matrices 7 and 6: the x wall. */
static void rkm_x_wall(void) {
    rkm_begin();
    rkm_cop4(0x03000606u, ages_ld32(0x50A3ECu), 0xBDA3D70Au,
             ages_ld32(0x50A3E4u));
    float s = ages_ldf(0x50A3E8u) * ages_ldf(0x50A3D0u);
    rkm_cop4(0x03800707u, ages_f2u(s), ages_ld32(0x50A3D4u), 0);
    rkm_end(7);
    rkm_begin();
    uint32_t a = 0u - (uint32_t)ages_ld16s(0x50A3FCu);
    uint32_t v = rkm_turn(0x12802525u, a, 0x1F4u);
    rkm_cop4(0x03000606u, v ^ 0x80000000u, 0x80000000u, 0x80000000u);
    v = rkm_turn(0x12002424u, a, 0x1FCu);
    rkm_cop4(0x03000606u, v, 0, 0);
    ages_cop_w(0x04800909u);
    ages_cop_w(a);
    rkm_end(6);
}

/* Matrices 5 and 4: the z wall. */
static void rkm_z_wall(void) {
    rkm_begin();
    rkm_cop4(0x03000606u, ages_ld32(0x50A3E0u), 0xBDA3D70Au,
             ages_ld32(0x50A3F4u));
    float s = ages_ldf(0x50A3F0u) * ages_ldf(0x50A3D8u);
    rkm_cop4(0x03800707u, 0, ages_ld32(0x50A3D4u), ages_f2u(s));
    rkm_end(5);
    rkm_begin();
    uint32_t a = 0xC000u - (uint32_t)ages_ld16s(0x50A3FCu);
    uint32_t v = rkm_turn(0x12002424u, a, 0x1F4u);
    rkm_cop4(0x03000606u, 0x80000000u, 0x80000000u, v ^ 0x80000000u);
    v = rkm_turn(0x12802525u, a, 0x1FCu);
    rkm_cop4(0x03000606u, 0x80000000u, 0x80000000u, v ^ 0x80000000u);
    ages_cop_w(0x04800909u);
    ages_cop_w(a);
    rkm_end(4);
}

/* 0x76B20-0x76C1C: the walls' signs from the light angle. */
static void rkm_walls(uint32_t *wx, uint32_t *wz) {
    uint32_t cam = ages_ld32(0x500814u);
    uint32_t a = (uint32_t)ages_ld16s(cam + 0x26u);
    if (AGES_G(1) & 0x10u) a = (uint32_t)ages_ld16s(0x50A3FCu);
    *wx = rkm_wall((a ^ (a << 1)) << 16, 0x50A3E4u,
                   ages_ld32(cam + 0x20u), 8u);
    uint32_t sz = ~a << 16;
    uint32_t c = 0x80000000u;   /* r13 still holds the sign mask */
    if (sz & 0x80000000u) c = ages_ld32(cam + 0x18u);
    *wz = rkm_wall(sz, 0x50A3E0u, c, 4u);
}

static uint32_t afn_rob_kabe_set_matrix(int entry) {
    ages_st32(RKM_F, 0);
    uint32_t p[3];
    ages_ldn(p, AGES_G(7) + 0x1F4u, 3);
    float w = ages_ldf(0x50A400u);
    uint32_t cc = rkm_cmpr(ages_u2f(p[0] & 0x7FFFFFFFu), w);
    if (cc == 4u) cc = rkm_cmpr(ages_u2f(p[2] & 0x7FFFFFFFu), w);
    if (cc != 4u) {
        rkm_flag(1u);
        rkm_flag(2u);
        RKM_CC(cc);
    } else {
        uint32_t wx, wz;
        rkm_walls(&wx, &wz);
        rkm_planes(wx, wz);
        if (!(ages_ld32(RKM_F) & 2u)) rkm_x_wall();
        uint32_t z = ages_ld32(RKM_F) & 1u;
        if (!z) rkm_z_wall();
        ages_cop_w(0x00800101u);
        rkm_end(3);
        RKM_CC(z ? 2u : 0u);
    }
    if (entry) ages_i960_ret();
    return 0;
}
