/* set_mirror: i960 0x2B77C, PS2 0x1E8DC0. Mirrors the motion values at g5
 * left for right: angles negated or taken from 0x8000, float signs flipped,
 * the left/right word triples swapped (some negated). As the PS2. */
#pragma once

enum { M1_MIRROR_KEEP, M1_MIRROR_NEG, M1_MIRROR_SIGN };

static uint32_t m1_mirror_op(uint32_t v, int op)
{
    if (op == M1_MIRROR_NEG) return 0u - v;
    if (op == M1_MIRROR_SIGN) return v ^ 0x80000000u;
    return v;
}

/* Swap the words at +a and +b, applying op to both. */
static void m1_mirror_swap(uint32_t rec, uint32_t a, uint32_t b, int op)
{
    uint32_t va = m1_mirror_op(ages_ld32(rec + a), op);
    uint32_t vb = m1_mirror_op(ages_ld32(rec + b), op);
    ages_st32(rec + a, vb);
    ages_st32(rec + b, va);
}

static void m1_mirror_flip(uint32_t rec, uint32_t a)
{
    ages_st32(rec + a, ages_ld32(rec + a) ^ 0x80000000u);
}

static void m1_mirror_half(uint32_t rec, uint32_t a)
{
    ages_st16(rec + a, 0x8000u - ages_ld16(rec + a));
}

static const uint8_t m1_mirror_tab[][3] = {
    {0x60, 0x54, M1_MIRROR_NEG},  {0x64, 0x58, M1_MIRROR_NEG},
    {0x68, 0x5C, M1_MIRROR_KEEP}, {0xC0, 0xB4, M1_MIRROR_SIGN},
    {0xC4, 0xB8, M1_MIRROR_KEEP}, {0xC8, 0xBC, M1_MIRROR_KEEP},
    {0x0C, 0x00, M1_MIRROR_NEG},  {0x10, 0x04, M1_MIRROR_NEG},
    {0x14, 0x08, M1_MIRROR_KEEP}, {0, 0, 0xFF},
    {0x84, 0x78, M1_MIRROR_NEG},  {0x88, 0x7C, M1_MIRROR_NEG},
    {0x8C, 0x80, M1_MIRROR_KEEP}, {0xE4, 0xD8, M1_MIRROR_SIGN},
    {0xE8, 0xDC, M1_MIRROR_KEEP}, {0xEC, 0xE0, M1_MIRROR_KEEP},
    {0x24, 0x18, M1_MIRROR_KEEP}, {0x28, 0x1C, M1_MIRROR_NEG},
    {0x2C, 0x20, M1_MIRROR_NEG},
};

static uint32_t afn_set_mirror(int entry)
{
    uint32_t rec = AGES_G(5);
    ages_st16(rec + 0x34, 0u - (uint32_t)ages_ld16s(rec + 0x34));
    ages_st16(rec + 0x38, 0u - (uint32_t)ages_ld16s(rec + 0x38));
    m1_mirror_flip(rec, 0x90);
    m1_mirror_half(rec, 0x3C);
    m1_mirror_half(rec, 0x44);
    m1_mirror_flip(rec, 0x9C);
    m1_mirror_flip(rec, 0xA8);
    for (size_t i = 0; i < sizeof m1_mirror_tab / sizeof m1_mirror_tab[0]; i++) {
        const uint8_t *t = m1_mirror_tab[i];
        if (t[2] == 0xFF) {
            m1_mirror_half(rec, 0x6C);
            m1_mirror_half(rec, 0x74);
        } else {
            m1_mirror_swap(rec, t[0], t[1], t[2]);
        }
    }
    if (entry) ages_i960_ret();
    return 0;
}
