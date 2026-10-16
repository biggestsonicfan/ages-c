#!/usr/bin/env python3
"""List the functions to export (roots.tsv) from ages_scan.py's tables.

ages_roots.py <scan dir> (--fn-regex RE | --fn-range LO HI) > roots.tsv
FN roots: EXT cases that call a game function: one whose name matches RE, or
(for a stripped executable) a case whose only call is to an address in [LO, HI).
COP roots: every command handler. Columns: address, name (blank if
unknown), group, file name, header comment."""
import argparse
import os
import re


def read(scan, name):
    with open(os.path.join(scan, name)) as f:
        next(f)
        return [line.rstrip('\n').split('\t') for line in f]


def is_game(fn, args):
    if args.fn_regex:
        return re.match(args.fn_regex, fn) is not None
    m = re.match(r'FUN_([0-9a-f]{8})$', fn)
    return m is not None and args.lo <= int(m.group(1), 16) < args.hi


def fn_roots(scan, args, addr_of):
    sites = {}
    for i960, _w0, _w1, kind, _calls in read(scan, 'patches.tsv'):
        sites.setdefault(kind, []).append(i960)
    for ext, _case, calls in read(scan, 'ext.tsv'):
        calls = calls.split()
        game = [c for c in calls if is_game(c, args)]
        if not game or (args.fn_range and len(calls) > 1):
            continue
        fn = game[0]
        addr = addr_of(fn)
        name = '' if fn.startswith('FUN_') else fn
        where = ', '.join(sites.get('ext ' + ext, [])) or '(no site)'
        stem = '_'.join(p for p in ('ext' + ext, name, addr) if p)
        print('%s\t%s\tFN\t%s\tEXT %s: %s @ %s; patched over i960 %s'
              % (addr, name, stem, ext, fn, addr, where))


def cop_roots(scan):
    for op, handler, name, firmware, argc, reply in read(scan, 'cop.tsv'):
        if name.startswith('FUN_'):
            name = firmware
        print('%s\t%s\tCOP\t%s_%s\tCOP %s %s: %s @ %s, %s argument words, %s reply bytes'
              % (handler, name, op, firmware, op, firmware, name, handler, argc, reply))


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('scan')
    ap.add_argument('--fn-regex')
    ap.add_argument('--fn-range', nargs=2)
    ap.add_argument('--names', help='names.tsv, to find the address of a named FN root')
    args = ap.parse_args()
    if args.fn_range:
        args.lo, args.hi = (int(x, 16) for x in args.fn_range)
    addrs = {}
    if args.names:
        for line in open(args.names):
            a, n = line.rstrip('\n').split('\t')[:2]
            addrs.setdefault(n, a.zfill(8))
    fn_roots(args.scan, args, lambda fn: fn[4:] if fn.startswith('FUN_') else addrs[fn])
    cop_roots(args.scan)


main()
