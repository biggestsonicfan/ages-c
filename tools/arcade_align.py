#!/usr/bin/env python3
"""Name FV's native helpers after the arcade i960 functions they translate.

arcade_align.py <fv elf> <functions.tsv> <names.tsv> <roots.tsv> <ida_calls.tsv> [LO HI [by_hand.tsv]] > arcade.tsv
Starts from the FN roots (named by i960names.py). For each native function with an
arcade name, its calls in code order (runtime calls dropped) are lined up against
the arcade function's calls (ida_calls.tsv: addr, name, end, callees in order).
Only native calls into [LO, HI), the game code, count: the rest are the runtime and
the VU0 matrix code that stands in for COP commands. Calls both sides already agree
on are anchors; between two anchors, a gap of the same length on both sides pairs
up in order. A native callee that every caller pairs with the same arcade name gets it; repeat until
nothing new. Output: native addr, arcade name, how (equal = the two call lists
had the same length, aligned = only the gap did, by hand = from by_hand.tsv, named
after reading the code where the alignment cannot pair them)."""
import re
import struct
import sys

RUNTIME = re.compile(r'^(i960|sysMem|sysCop(Read|Write)Fifo|sce|ADX|adx|CRS|CRI|cv|ROFS|SJ|mem|str|'
                     r'sprintf|printf|_|sqrtf|atan2?f?|sinf?|cosf?|floorf?|fabsf?|SQRT|abs|rand|srand|qsort)')


def native_calls(elf, tsv, names):
    d = open(elf, 'rb').read()
    ph = struct.unpack_from('<I', d, 0x1c)[0]
    off, va, fsz = (struct.unpack_from('<I', d, ph + o)[0] for o in (4, 8, 16))
    calls = {}
    for line in open(tsv):
        a, _name, size = line.rstrip('\n').split('\t')[:3]
        a, size = int(a, 16), int(size)
        if va <= a < va + fsz:
            words = struct.unpack_from('<%dI' % (size // 4), d, off + a - va)
            jal = [(w & 0x3ffffff) << 2 for w in words if w >> 26 == 3]
            calls[a] = [t for t in jal if not RUNTIME.match(names.get(t, ''))]
    return calls


def anchors(n, a, known):
    """LCS of the callees already named: index pairs where both lists call the same function."""
    dp = [[0] * (len(a) + 1) for _ in range(len(n) + 1)]
    for i in range(len(n) - 1, -1, -1):
        for j in range(len(a) - 1, -1, -1):
            same = known.get(n[i]) == a[j]
            dp[i][j] = max(dp[i + 1][j], dp[i][j + 1], dp[i + 1][j + 1] + 1 if same else 0)
    i = j = 0
    while i < len(n) and j < len(a):
        if known.get(n[i]) == a[j] and dp[i][j] == dp[i + 1][j + 1] + 1:
            yield i, j
            i, j = i + 1, j + 1
        elif dp[i + 1][j] >= dp[i][j + 1]:
            i += 1
        else:
            j += 1


def align(n, a, known):
    """Pair unnamed native callees with arcade callees between agreeing anchors, only
    where both gaps are the same length once the native side's named calls are dropped
(the PS2 adds calls of its own)."""
    marks = [(-1, -1)] + list(anchors(n, a, known)) + [(len(n), len(a))]
    for (i0, j0), (i1, j1) in zip(marks, marks[1:]):
        gap = [x for x in n[i0 + 1:i1] if x not in known]
        if len(gap) != j1 - j0 - 1:
            continue
        pairs = [(x, y) for x, y in zip(gap, a[j0 + 1:j1]) if y != '?' and not y.startswith('debug_')]
        yield from consistent(pairs)


def consistent(pairs):
    """Drop the pairs of a native callee that pairs with two arcade names in one gap, or of
    an arcade name paired with two native callees: the PS2 compiler moved a branch there."""
    xs, ys = {}, {}
    for x, y in pairs:
        xs.setdefault(x, set()).add(y)
        ys.setdefault(y, set()).add(x)
    return [(x, y) for x, y in pairs if len(xs[x]) == 1 and len(ys[y]) == 1]


def load_arcade(ida_tsv):
    arcade = {}
    for line in open(ida_tsv):
        p = line.rstrip('\n').split('\t')
        arcade[p[1]] = p[3].split() if len(p) > 3 else []
    return arcade


def load_roots(roots_tsv):
    known = {}
    for line in open(roots_tsv):
        p = line.rstrip('\n').split('\t')
        m = re.search(r'\(arcade ([A-Za-z0-9_]+)\)', p[4])
        if p[2] == 'FN' and m:
            known[int(p[0], 16)] = m.group(1)
    return known


def one_round(known, calls, arcade):
    """New names this round: callees every caller pairs with one arcade name, unique both ways."""
    votes = {}
    for fn, name in list(known.items()):
        n, a = calls.get(fn, []), arcade.get(name, [])
        kind = 'equal' if len(n) == len(a) else 'aligned'
        for x, y in align(n, a, known):
            votes.setdefault(x, set()).add((y, kind))
    new = {}
    for x, v in votes.items():
        ys = {y for y, _ in v}
        if len(ys) == 1:
            new[x] = (ys.pop(), 'equal' if any(k == 'equal' for _, k in v) else 'aligned')
    taken = {}
    for x, (y, _k) in new.items():
        taken.setdefault(y, []).append(x)
    return {x: v for x, v in new.items() if len(taken[v[0]]) == 1 and v[0] not in known.values()}


def load_by_hand(tsv):
    rows = [l.rstrip('\n').split('\t') for l in open(tsv) if l.strip() and not l.startswith('#')]
    return {int(p[0], 16): p[1] for p in rows}


def main():
    elf, tsv, names_tsv, roots_tsv, ida_tsv = sys.argv[1:6]
    lo, hi = (int(x, 16) for x in sys.argv[6:8]) if len(sys.argv) > 7 else (0, 1 << 32)
    names = {int(l.split('\t')[0], 16): l.split('\t')[1].strip() for l in open(names_tsv)}
    arcade = load_arcade(ida_tsv)
    calls = {f: [t for t in c if lo <= t < hi] for f, c in native_calls(elf, tsv, names).items()}
    known = load_roots(roots_tsv)
    how = dict.fromkeys(known, 'root')
    if len(sys.argv) > 8:
        hand = load_by_hand(sys.argv[8])
        known.update(hand)
        how.update(dict.fromkeys(hand, 'by hand'))
    new = one_round(known, calls, arcade)
    while new:
        for x, (y, k) in new.items():
            known[x], how[x] = y, k
        new = one_round(known, calls, arcade)
    for x in sorted(known):
        print('%08x\t%s\t%s' % (x, known[x], how[x]))


main()
