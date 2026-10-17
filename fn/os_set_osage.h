/* os_set_osage: i960 0x637B4, PS2 0x1FC380 (helper of osage_dsp, record
 * type 5).
 *
 * A sway segment's record. On the first frame (g13 flag 0) the rest point
 * (g13+0x114 offset by g9+0x28, through the COP's 0x18003030 and scaled by
 * g9+0x24) is added to the chain point g13+0x108, which moves on; the record
 * copies its constants. Later frames only store the gravity at g8+0x20:
 * zero when flag 7, else g9+0x28 (g9+0x34 with flag 1) scaled by the wind
 * g13+0x144 when that is set. As the PS2. */
#pragma once

static void oso_add(uint32_t a0, uint32_t a1, uint32_t a2,
                    uint32_t b0, uint32_t b1, uint32_t b2, uint32_t *o) {
    ages_cop_w(0x2E005C5Cu);
    ages_cop_w(a0); ages_cop_w(b0);
    ages_cop_w(a1); ages_cop_w(b1);
    ages_cop_w(a2); ages_cop_w(b2);
    for (int i = 0; i < 3; i++) o[i] = ages_cop_r();
}

static void oso_cmd3(uint32_t cmd, const uint32_t *pre, int npre,
                     uint32_t *v) {
    ages_cop_w(cmd);
    for (int i = 0; i < npre; i++) ages_cop_w(pre[i]);
    for (int i = 0; i < 3; i++) ages_cop_w(v[i]);
    for (int i = 0; i < 3; i++) v[i] = ages_cop_r();
}

static void oso_first(uint32_t g8, uint32_t g9, uint32_t g13) {
    uint32_t *g = &AGES_G(0), r[3], k[3];
    ages_ldn(g, g13 + 0x114u, 3);
    ages_ldn(r, g9 + 0x28u, 3);
    oso_add(r[0], r[1], r[2], g[0], g[1], g[2], g);
    oso_cmd3(0x18003030u, 0, 0, g);
    uint32_t s = ages_ld32(g9 + 0x24u);
    oso_cmd3(0x2F005E5Eu, &s, 1, g);
    ages_ldn(r, g13 + 0x108u, 3);
    oso_add(g[0], g[1], g[2], r[0], r[1], r[2], r);
    ages_stn(g13 + 0x108u, r, 3);
    ages_stn(g8, r, 3);
    ages_ldn(k, g9 + 0xCu, 3);
    ages_stn(g8 + 0xCu, k, 3);
    ages_st32(g8 + 0x18u, ages_ld32(g9 + 0x18u));
    ages_st32(g8 + 0x1Cu, s);
    ages_ldn(k, g9 + 0x28u, 3);
    ages_stn(g8 + 0x20u, k, 3);
}

static uint32_t oso_later(uint32_t g8, uint32_t g9, uint32_t g13) {
    uint32_t w = ages_ld32(g13), r3 = ages_ld32(g13 + 0x144u);
    uint32_t v[3] = { 0, 0, 0 }, cc = 2;
    if (!(w & 0x80u)) {
        ages_ldn(v, g9 + ((w & 2u) ? 0x34u : 0x28u), 3);
        cc = r3 ? 4u : 2u;
        if (r3) oso_cmd3(0x2F005E5Eu, &r3, 1, v);
    }
    ages_stn(g8 + 0x20u, v, 3);
    return cc;
}

static uint32_t afn_os_set_osage(int entry) {
    uint32_t g8 = AGES_G(8), g9 = AGES_G(9), g13 = AGES_G(13), cc = 2;
    if (ages_ld32(g13) & 1u) oso_first(g8, g9, g13);
    else cc = oso_later(g8, g9, g13);
    AGES_AC = (AGES_AC & ~7u) | cc;
    if (entry) ages_i960_ret();
    return 0;
}
