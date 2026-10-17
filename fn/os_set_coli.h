/* os_set_coli: i960 0x63454, PS2 0x1FCC80 (helper of osage_dsp, record
 * type 2).
 *
 * A collision body for a sway chain: on the first frame (g13 flag 0) the
 * radii and their squares and two limit planes (sin/cos of the angles
 * g9+0x20 - 0x4000 and g9+0x24 + 0x4000). Each frame the two ends (units
 * g9+0 and g9+0x28) go through the kage matrix 1 to g8+0x10/0x24, and both
 * limit points to g8+0x60/0x6C with their plane slopes at g8+0x58/0x5C;
 * g8 takes the chain origin g13+0xE8. Departs from the PS2 in one place:
 * the slope divide gives 0 on a zero divisor, as the board's fdivr does. */
#pragma once

static uint32_t osc_mul(uint32_t a, uint32_t b) {
    return ages_f2u(ages_u2f(a) * ages_u2f(b));
}

static uint32_t osc_sub(uint32_t a, uint32_t b) {
    return ages_f2u(ages_u2f(a) - ages_u2f(b));
}

static uint32_t osc_divr(uint32_t x, uint32_t y) {
    float d = ages_u2f(y);
    return d != 0.0f ? ages_f2u(ages_u2f(x) / d) : 0u;
}

static uint32_t osc_cop1(uint32_t cmd, uint32_t a) {
    ages_cop_w(cmd);
    ages_cop_w(a);
    return ages_cop_r();
}

static void osc_sq(uint32_t g8, uint32_t from, uint32_t to) {
    uint32_t v = ages_ld32(from);
    ages_st32(g8 + to, v);
    ages_st32(g8 + to + 4u, osc_mul(v, v));
}

static void osc_plane(uint32_t g8, uint32_t a, uint32_t to) {
    ages_st32(g8 + to, osc_cop1(0x11002222u, a));
    ages_st32(g8 + to + 4u, osc_cop1(0x10802121u, a));
    ages_st32(g8 + to + 8u,
              osc_mul(ages_ld32(g8 + to), ages_ld32(g8 + 0x38u)));
}

static void osc_first(uint32_t g8, uint32_t g9) {
    osc_sq(g8, g9 + 0xCu, 0x1Cu);
    osc_sq(g8, g9 + 0x18u, 0x30u);
    osc_sq(g8, g9 + 0x1Cu, 0x38u);
    osc_plane(g8, ages_ld32(g9 + 0x20u) - 0x4000u, 0x40u);
    osc_plane(g8, ages_ld32(g9 + 0x24u) + 0x4000u, 0x4Cu);
    ages_st32(g8 + 0x54u, ages_ld32(g8 + 0x54u) ^ 0x80000000u);
}

static void osc_unit(uint32_t cmd, uint32_t unit) {
    ages_cop_w(cmd);
    ages_cop_w(ages_ld8(AGES_G(7) + 4u));
    ages_cop_w(unit * 12u);
}

static void osc_xf(uint32_t a, uint32_t b, uint32_t c, uint32_t *o) {
    ages_cop_w(0x14802929u);
    ages_cop_w(a); ages_cop_w(b); ages_cop_w(c);
    for (int i = 0; i < 3; i++) o[i] = ages_cop_r();
}

static void osc_ends(uint32_t g8, uint32_t g9) {
    uint32_t *g = &AGES_G(0);
    osc_unit(0x1B003636u, ages_ld32(g9));
    osc_xf(ages_ld32(g9 + 4u), 0, ages_ld32(g9 + 8u), g + 4);
    osc_xf(ages_ld32(g9 + 0x10u), 0, ages_ld32(g9 + 0x14u), g);
    ages_cop_w(0x22004444u);
    ages_cop_w(1);
    osc_xf(g[4], g[5], g[6], g + 4);
    ages_stn(g8 + 0x10u, g + 4, 3);
    osc_xf(g[0], g[1], g[2], g);
    ages_stn(g8 + 0x24u, g, 3);
}

/* One limit point: k is the record offset, out the point, c the plane. */
static void osc_limit(uint32_t g8, uint32_t g9, uint32_t k, uint32_t out,
                      uint32_t c, uint32_t slope, const uint32_t *ax) {
    uint32_t *g = &AGES_G(0), p[3], q[3];
    ages_cop_w(0x2D805B5Bu);
    ages_cop_w(ages_ld32(g9 + k));
    ages_cop_w(g[4]);
    ages_cop_w(g[5]);
    p[0] = ages_cop_r();
    p[1] = ages_cop_r();
    ages_cop_w(0x2C805959u);
    ages_cop_w(p[0]); ages_cop_w(ax[0]);
    ages_cop_w(p[1]); ages_cop_w(ax[1]);
    p[2] = ages_f2u(ages_u2f(ages_cop_r()) + ages_u2f(ages_ld32(g9 + k + 4u)));
    ages_stn(g8 + out, p, 3);
    ages_ldn(q, g8 + c, 3);
    g[1] = osc_mul(p[2], q[0]);
    g[3] = osc_mul(p[1], q[0]);
    g[0] = osc_sub(g[1], osc_mul(p[0], q[2]));
    g[2] = osc_sub(g[3], osc_mul(p[0], q[1]));
    ages_st32(g8 + slope, osc_divr(g[0], g[2]));
}

static void osc_limits(uint32_t g8, uint32_t g9) {
    uint32_t *g = &AGES_G(0), ax[3];
    osc_unit(0x1B803737u, ages_ld32(g9 + 0x28u));
    ages_cop_w(0x2B005656u);
    g[4] = ages_cop_r();
    g[5] = ages_cop_r();
    (void)ages_cop_r();
    ages_cop_w(0x07800F0Fu);
    for (int i = 0; i < 3; i++) ax[i] = ages_cop_r();
    ages_cop_w(0x2D005A5Au);
    ages_cop_w(g[4]);
    ages_cop_w(g[5]);
    g[4] = ages_cop_r();
    g[5] = ages_cop_r();
    osc_limit(g8, g9, 0x2Cu, 0x60u, 0x40u, 0x58u, ax);
    osc_limit(g8, g9, 0x34u, 0x6Cu, 0x4Cu, 0x5Cu, ax);
}

static uint32_t afn_os_set_coli(int entry) {
    uint32_t g8 = AGES_G(8), g9 = AGES_G(9), g13 = AGES_G(13);
    uint32_t first = ages_ld32(g13) & 1u, v[3];
    if (first) osc_first(g8, g9);
    ages_cop_w(0x00800101u);
    osc_ends(g8, g9);
    osc_limits(g8, g9);
    ages_cop_w(0x01000202u);
    ages_ldn(v, g13 + 0xE8u, 3);
    ages_stn(g8, v, 3);
    ages_st32(g8 + 0xCu, ages_ld32(g13 + 0xF4u));
    AGES_AC = (AGES_AC & ~7u) | (first ? 2u : 0u);
    if (entry) ages_i960_ret();
    return 0;
}
