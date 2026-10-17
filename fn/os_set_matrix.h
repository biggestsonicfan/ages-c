/* os_set_matrix: i960 0x62EE0, PS2 0x1FD640 (helper of osage_dsp, record
 * type 1).
 *
 * A sway chain's root: under the fighter's unit matrix (unit at g9) with the
 * record's rotation (g9+4..) and offset (g9+0x10..), the root's height
 * above the floor lift goes to g13+0xF4; the wind (g13+0x138, y lowered by
 * the record's weight g9+0x1C times [0x50A000]) turned into the root's space
 * gives g4-g6 (kept at g13+0x114 on the first frame), COP 0x57's three
 * words g13+0xE8. Then the chain's matrices: g8+0x30 (root, from last
 * frame's at g13 + [g13+0x50] or identity on the first frame), g8+0x60
 * (moved by the wind), the new one back to g13 + [g13+0x50] (which steps
 * 0x30) and g8 (camera). The board's order; no departures. */
#pragma once

/* COP 0x2800505: the current matrix's 12 words to at. */
static void osm_get(uint32_t at) {
    ages_cop_w(0x02800505u);
    for (uint32_t i = 0; i < 12; i++) ages_st32(at + i * 4u, ages_cop_r());
}

static void osm_w3(uint32_t cmd, uint32_t a, uint32_t b, uint32_t c) {
    ages_cop_w(cmd);
    ages_cop_w(a);
    ages_cop_w(b);
    ages_cop_w(c);
}

/* 0x62EE0-0x62F94: the root's place; its height to g13+0xF4. */
static void osm_root(void) {
    uint32_t g9 = AGES_G(9), v[3];
    ages_cop_w(0x00800101u);
    ages_cop_w(0x1B003636u);
    ages_cop_w(ages_ld8(AGES_G(7) + 4u));
    ages_cop_w(ages_ld32(g9) * 12u);
    ages_ldn(v, g9 + 4u, 3);
    ages_cop_w(0x05000A0Au);
    ages_cop_w(v[0]);
    ages_cop_w(0x04800909u);
    ages_cop_w(v[1]);
    ages_cop_w(0x04000808u);
    ages_cop_w(v[2]);
    ages_ldn(v, g9 + 0x10u, 3);
    osm_w3(0x03000606u, v[0], v[1], v[2]);
    ages_cop_w(0x21804343u);
    ages_cop_w(0);
    ages_cop_w(0x07800F0Fu);
    (void)ages_cop_r();
    uint32_t y = ages_cop_r() ^ 0x80000000u;
    AGES_G(2) = ages_cop_r();
    AGES_G(0) = ages_ld32(AGES_G(13) + 0xE4u);
    AGES_G(1) = ages_f2u(ages_u2f(AGES_G(0)) + ages_u2f(y));
    ages_st32(AGES_G(13) + 0xF4u, AGES_G(1));
}

/* 0x62F98-0x63058: the wind in the root's space (g4-g6), COP 0x57. */
static void osm_wind(void) {
    uint32_t g13 = AGES_G(13);
    ages_cop_w(0x06000C0Cu);
    ages_cop_w(0x09801313u);
    ages_cop_w(0x09801313u);
    ages_cop_w(0x09801313u);
    (void)ages_cop_r();
    ages_cop_w(0x21804343u);
    ages_cop_w(1);
    uint32_t w[3];
    ages_ldn(w, g13 + 0x138u, 3);
    float d = ages_u2f(ages_ld32(0x50A000u) ^ 0x80000000u)
              * ages_ldf(AGES_G(9) + 0x1Cu);
    w[1] = ages_f2u(d + ages_u2f(w[1]));
    ages_cop_w(0x00800101u);
    ages_cop_w(0x06800D0Du);
    osm_w3(0x14802929u, w[0], w[1], w[2]);
    for (int i = 0; i < 3; i++) AGES_G(4 + i) = ages_cop_r();
    if (ages_ld32(g13) & 1u) ages_stn(g13 + 0x114u, &AGES_G(4), 3);
    ages_cop_w(0x2B805757u);
    for (int i = 0; i < 3; i++) AGES_G(i) = ages_cop_r();
    ages_stn(g13 + 0xE8u, &AGES_G(0), 3);
    ages_cop_w(0x01000202u);
}

static uint32_t afn_os_set_matrix(int entry) {
    osm_root();
    osm_wind();
    uint32_t g13 = AGES_G(13), g8 = AGES_G(8);
    uint32_t off = ages_ld32(g13 + 0x50u);
    uint32_t first = ages_ld32(g13) & 1u;
    if (first) {
        ages_cop_w(0x01800303u);
    } else {
        ages_cop_w(0x05800B0Bu);
        for (uint32_t i = 0; i < 12; i++)
            ages_cop_w(ages_ld32(g13 + off + i * 4u));
    }
    osm_get(g8 + 0x30u);
    osm_w3(0x07000E0Eu, AGES_G(4), AGES_G(5), AGES_G(6));
    osm_get(g8 + 0x60u);
    ages_cop_w(0x22004444u);
    ages_cop_w(0);
    osm_get(g13 + off);
    ages_st32(g13 + 0x50u, off + 0x30u);
    ages_cop_w(0x01000202u);
    ages_cop_w(0x00800101u);
    ages_cop_w(0x22804545u);
    ages_cop_w(0);
    osm_get(g8);
    ages_cop_w(0x01000202u);
    AGES_AC = (AGES_AC & ~7u) | (first ? 2u : 0u);
    if (entry) ages_i960_ret();
    return 0;
}
