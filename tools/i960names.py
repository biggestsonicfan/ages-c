#!/usr/bin/env python3
"""Name the native functions after the arcade i960 routine each one replaces.
usage: i960names.py [--keep] [--by-hand TSV] <names> <roots.tsv> > roots_named.tsv
names: ida_names.tsv (addr, name, F (function) or L (label), function end, comment;
dumped from an IDA database of the arcade program), or a .csv of address,name,...
(vf2-decomp's decomp/i960/original_symbols.csv, Sega's own symbols for VF2 2.1), where
each function runs to the next symbol. FN roots whose comment says "patched over i960
<site>" get the name at that site (function+offset inside one).
--keep: only add " (arcade X)" to the comment, keeping the root's name (VF2's are
Sega's own, from the PS2 symbol table).
--by-hand: native addr, arcade name, evidence; for roots whose first site is not in the
function they replace (VF2's SetObj is patched over its 140 call sites)."""
import bisect
import re
import sys


def load(path):
    exact, funcs = {}, []
    if path.endswith('.csv'):
        rows = []
        with open(path) as f:
            next(f)
            for line in f:
                addr, name = line.split(',')[:2]
                rows.append((int(addr, 16), name))
        rows.sort()
        for i, (addr, name) in enumerate(rows):
            exact[addr] = name
            end = rows[i + 1][0] if i + 1 < len(rows) else addr
            funcs.append((addr, end, name))
        return exact, funcs
    for line in open(path):
        addr, name, kind, end = line.rstrip('\n').split('\t')[:4]
        addr = int(addr, 16)
        exact[addr] = name
        if kind == 'F':
            funcs.append((addr, int(end, 16), name))
    funcs.sort()
    return exact, funcs


def load_by_hand(path):
    hand = {}
    for line in open(path):
        if line.startswith('#') or not line.strip():
            continue
        addr, name = line.rstrip('\n').split('\t')[:2]
        hand[addr] = name
    return hand


def name_at(addr, exact, funcs):
    if addr in exact and not re.match(r'a[A-Z0-9]', exact[addr]):
        return exact[addr]
    i = bisect.bisect_right(funcs, (addr, 1 << 32)) - 1
    if i >= 0 and funcs[i][0] <= addr < funcs[i][1]:
        return '%s+%x' % (funcs[i][2], addr - funcs[i][0])
    return ''


def rename(cols, exact, funcs, keep, hand):
    m = re.search(r'patched over i960 ([0-9a-f]+)', cols[4])
    if cols[2] != 'FN' or not m or 'arcade ' in cols[4]:
        return cols
    raw = hand.get(cols[0]) or name_at(int(m.group(1), 16), exact, funcs)
    if not raw:
        return cols
    if keep:
        cols[4] += ' (arcade %s)' % raw
        return cols
    name = re.sub(r'\+([0-9a-f]+)', r'_p\1', raw)
    cols[4] = cols[4].replace(cols[1] or 'FUN_' + cols[0], name) + ' (arcade %s)' % raw
    cols[1] = name
    cols[3] = re.sub(r'^(ext[0-9a-f]+)_', r'\1_%s_' % name, cols[3])
    return cols


def main():
    args = sys.argv[1:]
    keep = '--keep' in args
    if keep:
        args.remove('--keep')
    hand = {}
    if '--by-hand' in args:
        i = args.index('--by-hand')
        hand = load_by_hand(args[i + 1])
        del args[i:i + 2]
    exact, funcs = load(args[0])
    for line in open(args[1]):
        print('\t'.join(rename(line.rstrip('\n').split('\t'), exact, funcs, keep, hand)))


main()
