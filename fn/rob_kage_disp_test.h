/* rob_kage_disp_test: i960 0x60640, PS2 0x1F5580 (EXT 7e;
 * decomp/FV/FN/ext7e_rob_kage_disp_test_001f5580.c).
 *
 * Fighter g7's shadow, in GEO mode 2 under the kage matrix (COP 0x22):
 * the 15 parts of the fighter's part table (0x60BC8 by the byte at
 * g7+0x1B0; 0x18 bytes each: unit, model, kage arg, three words that go to
 * g0-g2) whose unit bit is set in g4, then the held item's shadow from the
 * table's trailer. g5 is the kage argument. The same as the PS2; shared
 * with rob_kage_disp_inside and rob_kabe_kage_disp (rkt_* helpers). */
#pragma once

static void rkt_sync(void) {
    ages_cop_w(0x09801313u);
    ages_cop_w(0x09801313u);
    ages_cop_w(0x09801313u);
    (void)ages_cop_r();
}

/* bbc/bbs rN,g4: the bit number mod 32 */
static int rkt_bit(uint32_t n) {
    return (AGES_G(4) >> (n & 31u)) & 1u;
}

static void rkt_set_obj(uint32_t model) {
    AGES_G(1) = ages_ld8(AGES_G(7) + 4u);
    AGES_G(0) = model;
    afn_set_obj(0);
}

/* set_obj(model) as its own display-list block, linked into the list whose
 * head and tail words are at list, list + 4: g14 is saved at 0x50EFFC
 * around a GEO port write, the block's start kept at 0x501800 (as the i960). */
static void rkt_linked(uint32_t list, uint32_t model) {
    uint32_t g10 = AGES_G(10);
    uint32_t dl = ages_ld32(g10 + 0x2008u);
    ages_st32(0x50EFFCu, AGES_G(14));
    ages_st32(g10, 0);
    ages_st32(0x501800u, dl);
    uint32_t tail = ages_ld32(list + 4u);
    uint32_t at = ages_ld32(g10 + 0x2008u);
    if (tail == 0) ages_st32(list, at);
    else ages_st32(0x900000u + tail, at | 0x80000000u);
    AGES_G(1) = 0;
    AGES_G(0) = model;
    afn_set_obj(0);
    uint32_t head = ages_ld32(list);
    ages_st32(list + 4u, ages_ld32(g10 + 0x2008u));
    ages_st32(g10, head | 0x80000000u);
    uint32_t end = ages_ld32(g10 + 0x2008u);
    ages_st32(0x900000u + ages_ld32(0x501800u), end | 0x80000000u);
}

static void rkt_kage_matrix(void) {
    ages_cop_w(0x00800101u);
    ages_cop_w(0x22004444u);
    ages_cop_w(0);
}

static void rkt_part(uint32_t part) {
    uint32_t rob = AGES_G(7);
    uint32_t unit = ages_ld32(part), arg = ages_ld32(part + 8u);
    ages_ldn(&AGES_G(0), part + 0xCu, 3);
    if (unit == 9) {
        uint32_t st = ages_ld32(rob + 0x1A4u);
        if ((st & 0x4000u) && (st & 0x10000u)) return;
    }
    if (!rkt_bit(unit)) return;
    ages_cop_w(0x3A007474u);
    ages_cop_w(ages_ld8(rob + 4u));
    ages_cop_w(unit * 12u);
    ages_cop_w(arg);
    ages_cop_w(AGES_G(0));
    ages_cop_w(AGES_G(1));
    ages_cop_w(AGES_G(2));
    ages_cop_w(AGES_G(5));
    rkt_sync();
    rkt_set_obj(ages_ld32(part + 4u));
    ages_cop_w(0x01000202u);
}

/* The held item under its own matrix (g7+0xF34, 0x34 bytes a slot). */
static void rkt_item(uint32_t model) {
    uint32_t rob = AGES_G(7);
    uint32_t mat = ages_ld32(rob + 0xF00u) * 0x34u + rob + 0xF34u;
    rkt_sync();
    if (!rkt_bit(ages_ld16(rob + 0x788u))) return;
    rkt_sync();
    ages_cop_w(0x05800B0Bu);
    for (uint32_t k = 0; k < 12; k++) ages_cop_w(ages_ld32(mat + k * 4u));
    rkt_sync();
    ages_cop_w(0x23004646u);
    ages_cop_w(AGES_G(5));
    rkt_sync();
    rkt_set_obj(model);
}

/* The item on a unit of the fighter (status bit 27). */
static void rkt_item_unit(uint32_t trailer) {
    uint32_t rob = AGES_G(7);
    uint32_t model = ages_ld16(trailer + 2u), alt = ages_ld16(trailer + 4u);
    if (model == 0 || ages_ld16(rob + 0x7A2u) == 0) return;
    uint32_t unit = ages_ld16(rob + 0x7A0u);
    if (!rkt_bit(unit)) return;
    ages_cop_w(0x1B803737u);
    ages_cop_w(ages_ld8(rob + 4u));
    ages_cop_w(unit * 12u);
    rkt_sync();
    ages_cop_w(0x23004646u);
    ages_cop_w(AGES_G(5));
    if (ages_ld32(rob) & 0x40u) model = alt;
    rkt_set_obj(model);
}

static uint32_t afn_rob_kage_disp_test(int entry) {
    uint32_t rob = AGES_G(7);
    AGES_G(0) = 2;
    afn_set_mmode(0);
    rkt_kage_matrix();
    uint32_t part = ages_ld32(0x60BC8u + ages_ld8(rob + 0x1B0u) * 4u);
    for (int i = 0; i < 15; i++, part += 0x18u) rkt_part(part);
    ages_cop_w(0x01000202u);
    rkt_kage_matrix();
    uint32_t model = ages_ld16(part);
    if (model != 0 && ages_ld16(rob + 0x78Au) != 0) {
        if (ages_ld32(rob) & (1u << 27))
            rkt_item_unit(part);
        else
            rkt_item(model);
    }
    ages_cop_w(0x01000202u);
    AGES_G(0) = 1;
    afn_set_mmode(0);
    if (entry) ages_i960_ret();
    return 0;
}
