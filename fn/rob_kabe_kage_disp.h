/* rob_kabe_kage_disp: i960 0x76944, PS2 0x1CC540 (EXT 6d;
 * decomp/FV/FN/ext6d_rob_kabe_kage_disp_001cc540.c).
 *
 * Fighter g7's shadow on the walls of a walled stage (stage flags at
 * 0x6442BA8 + stage * 12, bit 31; bit 7 only while 0x574004 & 15 == 0):
 * wall distance (0x50A400) and light angle (0x50A3FC, never exactly 0x4000
 * or 0xC000) from the stage record or the defaults, then the wall matrices,
 * the x and z wall shadows and the floor blobs. As the board: with bit 7
 * the record pointer is the (zero) 0x574004 & 15, so the record words are
 * read from addresses 4 and 8. */
#pragma once

#include "rob_kabe_set_matrix.h"
#include "rob_kage_disp_kabe.h"
#include "rob_kage_disp_ground.h"

/* 0x76974-0x769FC: distance and angle; rec = the stage record. */
static void rkb_light(uint32_t rec) {
    uint32_t g1 = AGES_G(1);
    ages_st32(0x50A400u, ages_ld32(rec + 4u));
    if (!(g1 & 0x20u)) ages_st32(0x50A400u, ages_ld32(0x50A00Cu));
    ages_st16(0x50A3FCu, (uint32_t)ages_ld16s(rec + 8u));
    if (!(g1 & 0x10u))
        ages_st16(0x50A3FCu,
                  (uint32_t)ages_ld16s(ages_ld32(0x500814u) + 0x26u));
    uint32_t a = ages_ld16(0x50A3FCu);
    if (a == 0x4000u) ages_st16(0x50A3FCu, 0x4001u);
    else if (a == 0xC000u) ages_st16(0x50A3FCu, 0xC001u);
}

static uint32_t afn_rob_kabe_kage_disp(int entry) {
    uint32_t rec = 0x6442BA8u + ages_ld8(0x500064u) * 12u;
    AGES_G(1) = ages_ld32(rec);
    int on = (AGES_G(1) & 0x80000000u) != 0;
    if (on && (AGES_G(1) & 0x80u)) {
        rec = ages_ld32(0x574004u) & 15u;
        on = rec == 0;
    }
    if (on) {
        rkb_light(rec);
        afn_rob_kabe_set_matrix(0);
        AGES_G(6) = 0;
        if (!(ages_ld32(0x50A3F8u) & 2u)) {
            AGES_G(5) = 6;
            afn_rob_kage_disp_kabe(0);
        }
        if (!(ages_ld32(0x50A3F8u) & 1u)) {
            AGES_G(6) |= 0x80000000u;
            AGES_G(5) = 4;
            afn_rob_kage_disp_kabe(0);
        }
        afn_rob_kage_disp_ground(0);
    }
    if (entry) ages_i960_ret();
    return 0;
}
