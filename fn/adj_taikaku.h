/* adj_taikaku: i960 0x2BF4C, PS2 0x1E3340. For the two big fighters
 * (g7+0x1B1 = 9 or 10) sets the height offset g7+0xC50 = y*s - y and, in
 * the 0x1A4 bit 19 move, grows s and lowers 0x18A/0x17C over frames 11-20
 * (or by 0x800 to frame 50). Only kinds 9 and 10 remain of the PS2's table. */
#pragma once

static void m1_tai_cc(uint32_t cc)
{
    AGES_AC = (AGES_AC & ~7u) | cc;
}

static uint32_t m1_tai_cmpo(uint32_t a, uint32_t b)
{
    return a < b ? 4u : a == b ? 2u : 1u;
}

static void m1_tai_lower(uint32_t rob, uint32_t d)
{
    ages_st16(rob + 0x18A, (uint32_t)ages_ld16s(rob + 0x18A) + d);
    ages_st16(rob + 0x17C, (uint32_t)ages_ld16s(rob + 0x17C) + d);
}

/* Frames past 20: lower by 0x800 to frame 50 when 0x614 & 0x9000. */
static void m1_tai_late(uint32_t rob, uint32_t frame)
{
    if (ages_ld32(0x508000) & 0x20u) { m1_tai_cc(2); return; }
    if (!(ages_ld16(rob + 0x614) & 0x9000u)) { m1_tai_cc(2); return; }
    m1_tai_cc(m1_tai_cmpo(50, frame));
    if (frame > 50)
        return;
    m1_tai_lower(rob, 0xFFFFF800u);
}

static uint32_t afn_adj_taikaku(int entry)
{
    uint32_t rob = AGES_G(7), kind = ages_ld8(rob + 0x1B1);
    if (kind != 9 && kind != 10) {
        ages_st32(rob + 0xC50, 0);
        m1_tai_cc(m1_tai_cmpo(10, kind));
        if (entry) ages_i960_ret();
        return 0;
    }
    float s = ages_u2f(kind == 9 ? 0x3F8CCCCDu : 0x3F0F5C29u);
    uint32_t frame = ages_ld16(rob + 0x1AA);
    if (!(ages_ld32(rob + 0x1A4) & 0x80000u)) {
        m1_tai_cc(0);
    } else if (frame > 20) {
        m1_tai_late(rob, frame);
        if (entry) ages_i960_ret();
        return 0;
    } else if (frame <= 10) {
        m1_tai_cc(m1_tai_cmpo(10, frame));
    } else {
        uint32_t n = frame - 10;
        ages_cop_w(0x0B801717);
        ages_cop_w(n);
        float k = ages_u2f(ages_cop_r()) * ages_u2f(0x3B23D70Au);
        s = s + k;
        int pause = (ages_ld32(0x508000) & 0x20u) != 0;
        m1_tai_cc(pause ? 2u : 0u);
        if (!pause)
            m1_tai_lower(rob, n * 0xFFFFFF00u);
    }
    float y = ages_u2f(ages_ld32(rob + 0x84));
    float sy = y * s;
    ages_st32(rob + 0xC50, ages_f2u(sy - y));
    if (entry) ages_i960_ret();
    return 0;
}
