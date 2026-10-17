/* osage_dsp: i960 0x62854, PS2 0x1FA800 (EXT 6c).
 *
 * A fighter's sway chains (hair, cloth; g13 the chain block, g7 its
 * fighter): set the frame flags (frozen, low, hidden by the camera), run
 * calc_kaze, build the chain records (g13+0x44 script into g13+0x48, by
 * os_set_matrix/coli/etc/tsukene/osage), solve and draw them through
 * osage_copro in the fighter's display-list slot (0x501804 + id*8), then on
 * stages with flag 8 their shadows from the FIFO at 0x50E404 under the kage
 * matrix in list 0x50184C. Times itself with the board's timer (0xF00008)
 * into g13+0x120/0x124/0x128 on the first frame (ldl 0xF00008: both
 * words from the one register, as MMIO does not burst). Departs from the board in
 * one place: the debug print when the records overrun g13+0x48 + 0x800 is
 * left out. */
#pragma once

#include "calc_kaze.h"
#include "os_set_matrix.h"
#include "os_set_coli.h"
#include "os_set_etc.h"
#include "os_set_tsukene.h"
#include "os_set_osage.h"
#include "osage_copro.h"
#include "set_situation_flags.h"

static void osd_flag(uint32_t g13, uint32_t bit, int on) {
    uint32_t w = ages_ld32(g13);
    ages_st32(g13, on ? (w | bit) : (w & ~bit));
}

static uint32_t osd_timer(void) {
    (void)ages_ld32(0xF00008u);   /* ldl: the second word too */
    uint32_t t = ages_ld32(0xF00008u);
    return (0xFFFFFu - (t & 0xFFFFFu) - 0x12u) / 0x19u;
}

static void osd_open(uint32_t list) {
    uint32_t g10 = AGES_G(10);
    uint32_t dl = ages_ld32(g10 + 0x2008u);
    ages_st32(0x50EFFCu, AGES_G(14));
    ages_st32(g10, 0);
    AGES_G(14) = ages_ld32(0x50EFFCu);
    ages_st32(0x501800u, dl);
    uint32_t tail = ages_ld32(list + 4u);
    uint32_t at = ages_ld32(g10 + 0x2008u);
    if (tail == 0) ages_st32(list, at);
    else ages_st32(0x900000u + tail, at | 0x80000000u);
}

static void osd_close(uint32_t list) {
    uint32_t g10 = AGES_G(10);
    uint32_t head = ages_ld32(list);
    ages_st32(list + 4u, ages_ld32(g10 + 0x2008u));
    ages_st32(g10, head | 0x80000000u);
    uint32_t end = ages_ld32(g10 + 0x2008u);
    ages_st32(0x900000u + ages_ld32(0x501800u), end | 0x80000000u);
}

/* Returns 0 to go on, else the condition code to leave with. */
static uint32_t osd_enter(uint32_t g13) {
    uint32_t g7 = AGES_G(7);
    if (ages_ld8(g7 + 0x1B1u) == 1u) {
        ages_st32(g13 + 0x44u, 0x63FCCu);
        if (!(ages_ld32(g7 + 0x7F4u) & 4u)) ages_st32(g13 + 0x44u, 0x64154u);
    }
    uint32_t w = ages_ld32(g13) & ~0x10u;
    ages_st32(g13, w);
    if (((0x300u >> (w & 31u)) & 1u)
        && ((3u >> (ages_ld8(0x500031u) & 31u)) & 1u)) return 2;
    if (!(ages_ld32(g7) & 0x80u)) return 0x10;
    osd_flag(g13, 0x100u, 0);
    uint32_t cam = ages_ld32(ages_ld32(0x500814u));
    uint32_t bit = ages_ld8(g7 + 4u) == 0 ? 2u : 4u;
    osd_flag(g13, 0x20u, !(cam & bit));
    return 0;
}

static void osd_lift(uint32_t g13) {
    uint32_t g7 = AGES_G(7), lift;
    if (ages_ld32(g13) & 0x10u) {
        lift = 0xC479F99Au;
    } else {
        int32_t x = (int32_t)(ages_ld32(g7 + 0x20Cu) & 0x7FFFFFFFu);
        int32_t z = (int32_t)(ages_ld32(g7 + 0x214u) & 0x7FFFFFFFu);
        int32_t m = x > z ? x : z;
        lift = m > (int32_t)ages_ld32(0x50A00Cu) ? 0xBF000000u : 0u;
    }
    ages_st32(g13 + 0xE4u, lift);
    float level = ages_u2f(lift) + 0.5f;
    if (ages_u2f(ages_ld32(g7 + 0x210u)) < level) {
        osd_flag(g13, 0x40u, 1);
        osd_flag(g13, 0x80u, ages_ld8(g7 + 0x1B1u) != 5u);
    } else {
        osd_flag(g13, 0x40u, 0);
        osd_flag(g13, 0x80u, 0);
    }
}

static void osd_records(uint32_t g13) {
    ages_st32(g13 + 0x144u, 0);
    ages_st32(g13 + 0x50u, 0x54u);
    AGES_G(9) = ages_ld32(g13 + 0x44u);
    AGES_G(8) = ages_ld32(g13 + 0x48u);
    for (;;) {
        uint32_t type = ages_ld32(AGES_G(9));
        AGES_G(9) += 4;
        ages_st32(AGES_G(8), type);
        AGES_G(8) += 4;
        if (type == 0) break;
        switch (type) {
        case 1: afn_os_set_matrix(0); break;
        case 2: afn_os_set_coli(0); break;
        case 3: afn_os_set_etc(0); break;
        case 4: afn_os_set_tsukene(0); break;
        case 5: afn_os_set_osage(0); break;
        }
        uint32_t step[2];
        ages_ldn(step, 0x62734u + type * 8u, 2);
        AGES_G(9) += step[0];
        AGES_G(8) += step[1];
    }
}

static void osd_draw(void) {
    uint32_t g7 = AGES_G(7);
    uint32_t slot = 0x501804u + ages_ld8(g7 + 4u) * 8u;
    osd_open(slot);
    ages_st32(0x50E400u, 0x50E404u);
    osd_lift(AGES_G(13));
    afn_calc_kaze(0);
    osd_records(AGES_G(13));
    ages_cop_w(0x00800101u);
    afn_osage_copro(0);
    ages_cop_w(0x01000202u);
    afn_set_situation_flags(0);
    osd_close(slot);
}

static void osd_shadow_obj(uint32_t model) {
    uint32_t g7 = AGES_G(7);
    AGES_G(1) = ages_ld8(g7 + 4u);
    AGES_G(0) = model;
    uint32_t v = ages_ld16(g7 + 0x1AAu);
    if ((ages_ld32(g7 + 0x1A4u) & 0x04000000u) && !(4u < v)) {
        AGES_G(2) = 0x800000u;
        afn_set_obj_thd(0);
    } else if ((ages_ld32(g7 + 0x720u) & 4u) && !(4u < v) && (v & 1u)) {
        AGES_G(2) = 0x802000u;
        afn_set_obj_thd(0);
    } else {
        afn_set_obj(0);
    }
}

static void osd_shadow_setup(void) {
    ages_cop_w(0x00800101u);
    ages_cop_w(0x06000C0Cu);
    ages_cop_w(0x09801313u);
    ages_cop_w(0x09801313u);
    ages_cop_w(0x09801313u);
    (void)ages_cop_r();
    ages_cop_w(0x21804343u);
    ages_cop_w(0);
    ages_cop_w(0x01000202u);
    ages_cop_w(0x00800101u);
    ages_cop_w(0x03000606u);
    ages_cop_w(0);
    ages_cop_w(0);
    ages_cop_w(0x41400000u);
    ages_cop_w(0x03800707u);
    ages_cop_w(0x3F800000u);
    ages_cop_w(0x3F800000u);
    ages_cop_w(0xBF800000u);
    ages_cop_w(0x22804545u);
    ages_cop_w(0);
}

static void osd_shadows(void) {
    osd_open(0x50184Cu);
    osd_shadow_setup();
    uint32_t r4 = 0x3F800000u, r5 = 0x50E404u;
    for (;;) {
        uint32_t r6 = ages_ld32(0x50E400u);
        ages_cop_w(0x09801313u);
        ages_cop_w(r4);
        ages_cop_w(r4);
        r4 = ages_cop_r();
        if (r5 == r6) break;
        ages_cop_w(0x00800101u);
        ages_cop_w(0x05800B0Bu);
        for (int i = 0; i < 12; i++) ages_cop_w(ages_ld32(r5 + i * 4u));
        r5 += 0x30u;
        uint32_t model = ages_ld32(r5);
        r5 += 4;
        if (model) osd_shadow_obj(model);
        ages_cop_w(0x01000202u);
    }
    ages_cop_w(0x01000202u);
    osd_close(0x50184Cu);
}

static uint32_t osd_leave(uint32_t g13) {
    uint32_t cc = 2;
    uint32_t w = ages_ld32(g13);
    if (!(w & 5u)) {
        if (ages_ld8(0x500000u) != 0) {
            cc = 4;
        } else {
            uint32_t t = osd_timer();
            ages_st32(g13 + 0x124u, t);
            ages_st32(g13 + 0x128u, t - ages_ld32(g13 + 0x120u));
            osd_flag(g13, 4u, 1);
        }
    }
    osd_flag(g13, 1u, 0);
    return cc;
}

static uint32_t osd_run(uint32_t g13) {
    AGES_G(7) = ages_ld32(g13 + 0x40u);
    uint32_t cc = osd_enter(g13);
    if (cc) return cc & 7u;
    if (!(ages_ld32(g13) & 5u)) ages_st32(g13 + 0x120u, osd_timer());
    osd_draw();
    uint32_t st = ages_ld8(0x500064u);
    if (ages_ld32(0x6442BA8u + st * 12u) & 0x100u) osd_shadows();
    uint32_t n = ages_ld16(g13 + 6u) + 1u;
    ages_st16(g13 + 6u, n);
    return osd_leave(g13);
}

static uint32_t afn_osage_dsp(int entry) {
    uint32_t g13 = AGES_G(13), cc = 2;
    if (!(ages_ld32(g13) & 8u)) cc = osd_run(g13);
    AGES_AC = (AGES_AC & ~7u) | cc;
    if (entry) ages_i960_ret();
    return 0;
}
