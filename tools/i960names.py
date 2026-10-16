#!/usr/bin/env python3
"""Name FV's native functions after the arcade i960 routine each one replaces.
usage: i960names.py <ida_names.tsv> <roots.tsv> > roots_named.tsv
ida_names.tsv: addr, name, F (function) or L (label), function end, comment;
dumped from an IDA database of the arcade program. FN roots whose comment says
"patched over i960 <site>" get the name at that site (function+offset inside one)."""
import bisect
import re
import sys


def load(path):
    exact, funcs = {}, []
    for line in open(path):
        addr, name, kind, end = line.rstrip('\n').split('\t')[:4]
        addr = int(addr, 16)
        exact[addr] = name
        if kind == 'F':
            funcs.append((addr, int(end, 16), name))
    funcs.sort()
    return exact, funcs


def name_at(addr, exact, funcs):
    if addr in exact and not re.match(r'a[A-Z0-9]', exact[addr]):
        return exact[addr]
    i = bisect.bisect_right(funcs, (addr, 1 << 32)) - 1
    if i >= 0 and funcs[i][0] <= addr < funcs[i][1]:
        return '%s+%x' % (funcs[i][2], addr - funcs[i][0])
    return ''


def rename(cols, exact, funcs):
    m = re.search(r'patched over i960 ([0-9a-f]+)', cols[4])
    if cols[2] != 'FN' or not m or 'arcade ' in cols[4]:
        return cols
    raw = name_at(int(m.group(1), 16), exact, funcs)
    if not raw:
        return cols
    name = re.sub(r'\+([0-9a-f]+)', r'_p\1', raw)
    cols[4] = cols[4].replace(cols[1] or 'FUN_' + cols[0], name) + ' (arcade %s)' % raw
    cols[1] = name
    cols[3] = re.sub(r'^(ext[0-9a-f]+)_', r'\1_%s_' % name, cols[3])
    return cols


def main():
    exact, funcs = load(sys.argv[1])
    for line in open(sys.argv[2]):
        print('\t'.join(rename(line.rstrip('\n').split('\t'), exact, funcs)))


main()
