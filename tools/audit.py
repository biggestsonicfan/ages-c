#!/usr/bin/env python3
"""Write AUDIT.md for an ages-decomp tree: audit.py <tree> [lizard]"""
import csv
import io
import os
import subprocess
import sys

FOLDERS = ['VF2/FN', 'VF2/FN/helpers', 'VF2/COP', 'VF2/COP/helpers',
           'FV/FN', 'FV/FN/helpers', 'FV/COP', 'FV/COP/helpers']
CMD = 'lizard -w -C 15 -L 80 VF2 FV'


def measure(tree, lizard, folder):
    out = subprocess.run([lizard, '--csv', os.path.join(tree, folder)],
                         capture_output=True, text=True).stdout
    rows = []
    for r in csv.reader(io.StringIO(out)):
        if 'helpers' not in folder and '/helpers/' in r[6]:
            continue
        rows.append({'fn': r[7], 'file': os.path.relpath(r[6], tree), 'loc': int(r[0]),
                     'ccn': int(r[1]), 'tok': int(r[2]), 'len': int(r[4])})
    return rows


def flagged(r):
    return r['ccn'] > 15 or r['len'] > 80


def table(rows, key):
    yield '| function | file | CCN | lines | tokens |'
    yield '|---|---|---:|---:|---:|'
    for r in sorted(rows, key=lambda r: -r[key])[:15]:
        yield '| `%s` | `%s` | %d | %d | %d |' % (r['fn'], r['file'], r['ccn'], r['len'], r['tok'])


def main():
    tree = sys.argv[1]
    lizard = sys.argv[2] if len(sys.argv) > 2 else 'lizard'
    version = subprocess.run([lizard, '--version'], capture_output=True, text=True).stdout.strip()
    out = ['# Lizard audit', '',
           '`lizard %s`, run as `%s` (and `--csv` for the tables below). A function is flagged '
           'when its cyclomatic complexity (CCN) is over 15 or it is longer than 80 lines.' % (version, CMD), '',
           'This is Ghidra output, not hand-written code: the complexity is the shape of the original '
           'game code plus what the decompiler adds, so it is a map of where reading is hard, '
           'not a to-do list.', '', '## Summary', '',
           '| folder | functions | lines of code | avg CCN | max CCN | flagged |',
           '|---|---:|---:|---:|---:|---:|']
    every = []
    for folder in FOLDERS:
        rows = measure(tree, lizard, folder)
        every += rows
        ccn = [r['ccn'] for r in rows]
        out.append('| `%s` | %d | %d | %.1f | %d | %d |' % (
            folder, len(rows), sum(r['loc'] for r in rows), sum(ccn) / len(ccn), max(ccn),
            sum(map(flagged, rows))))
    ccn = [r['ccn'] for r in every]
    out.append('| total | %d | %d | %.1f | %d | %d |' % (
        len(every), sum(r['loc'] for r in every), sum(ccn) / len(ccn), max(ccn), sum(map(flagged, every))))
    out += ['', '## Most complex (top 15 by CCN)', ''] + list(table(every, 'ccn'))
    out += ['', '## Longest (top 15 by length)', ''] + list(table(every, 'len'))
    out += ['', '## Every flagged function', '', '| function | file | CCN | lines |', '|---|---|---:|---:|']
    out += ['| `%s` | `%s` | %d | %d |' % (r['fn'], r['file'], r['ccn'], r['len'])
            for r in sorted(every, key=lambda r: r['file']) if flagged(r)]
    with open(os.path.join(tree, 'AUDIT.md'), 'w') as w:
        w.write('\n'.join(out) + '\n')


main()
