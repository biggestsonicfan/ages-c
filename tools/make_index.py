#!/usr/bin/env python3
"""Write <dir>/INDEX.md for one exported folder (FN or COP).
usage: make_index.py <roots.tsv> <group FN|COP> <dir> <title> [cop.tsv]"""
import os
import re
import sys


def lines(path):
    with open(path) as f:
        return sum(1 for _ in f)


def fn_rows(roots, d):
    yield '| EXT | native | name | i960 sites | arcade name | lines | file |'
    yield '|---|---|---|---|---|---:|---|'
    for addr, name, _g, stem, comment in roots:
        ext = re.match(r'EXT ([0-9a-f]+)', comment).group(1)
        sites = re.search(r'i960 (.*?)( \(arcade|$)', comment).group(1).split(', ')
        if len(sites) > 4:
            sites = sites[:3] + ['... (%d)' % len(sites)]
        arcade = re.search(r'\(arcade (.*)\)', comment)
        yield '| %s | %s | %s | %s | %s | %d | %s.c |' % (
            ext, addr, name or '-', ' '.join(sites), arcade.group(1) if arcade else '',
            lines(os.path.join(d, stem + '.c')), stem)


def cop_rows(roots, d, cop):
    yield '| op | firmware name | native | native name | args | reply bytes | lines | file |'
    yield '|---|---|---|---|---:|---:|---:|---|'
    for op, handler, name, firmware, argc, reply in cop:
        stem = '%s_%s' % (op, firmware)
        yield '| %s | %s | %s | %s | %s | %s | %d | %s.c |' % (
            op, firmware, handler, name, argc, reply, lines(os.path.join(d, stem + '.c')), stem)


def helper_rows(d):
    path = os.path.join(d, 'helpers.tsv')
    if not os.path.exists(path):
        return
    yield ''
    yield '## Helpers (`helpers/`)'
    yield ''
    yield 'Functions the above call, followed up to three calls deep. Runtime calls'
    yield '(`i960*`, `sysMem*`, libc, CRI and Sony libraries) are left out.'
    yield ''
    yield '| native | name | bytes | first caller | depth |'
    yield '|---|---|---:|---|---:|'
    for line in open(path):
        addr, name, size, caller, depth = line.rstrip('\n').split('\t')
        yield '| %s | %s | %s | %s | %s |' % (addr, name, size, caller, depth)


def main():
    roots_path, group, d, title = sys.argv[1:5]
    roots = [l.rstrip('\n').split('\t') for l in open(roots_path)]
    roots = [r for r in roots if r[2] == group]
    out = ['# ' + title, '']
    if group == 'FN':
        out += fn_rows(roots, d)
    else:
        with open(sys.argv[5]) as f:
            next(f)
            out += cop_rows(roots, d, [l.rstrip('\n').split('\t') for l in f])
    out += helper_rows(d)
    with open(os.path.join(d, 'INDEX.md'), 'w') as w:
        w.write('\n'.join(out) + '\n')


main()
