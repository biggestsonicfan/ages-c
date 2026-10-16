"""Dump a Sega Ages 2500 executable's i960 patch table, EXT switch and COP command table.

ages_scan.py <elf> <names.tsv> <outdir> --patch A --ext A N --cop A
names.tsv: address<TAB>name lines (VF2: from its symbol table; FV: from match.py).
Writes patches.tsv, ext.tsv, cop.tsv in outdir."""
import sys, os, struct, argparse, capstone

OFF, VA = 0x80, 0x100000
ap = argparse.ArgumentParser()
ap.add_argument('elf'); ap.add_argument('names'); ap.add_argument('out')
ap.add_argument('--patch', type=lambda s: int(s, 16), required=True)
ap.add_argument('--ext', nargs=2, required=True)
ap.add_argument('--cop', type=lambda s: int(s, 16), required=True)
a = ap.parse_args()
d = open(a.elf, 'rb').read()
names = {}
for l in open(a.names):
    p = l.rstrip('\n').split('\t')
    names.setdefault(int(p[0], 16), p[1])
def u32(x): return struct.unpack_from('<I', d, OFF + x - VA)[0]
def cstr(x):
    o = OFF + x - VA; return d[o:d.index(b'\0', o)].decode('ascii', 'replace')
def nm(x): return names.get(x, 'FUN_%08x' % x)
md = capstone.Cs(capstone.CS_ARCH_MIPS, capstone.CS_MODE_MIPS64 + capstone.CS_MODE_LITTLE_ENDIAN)
os.makedirs(a.out, exist_ok=True)

ext_base, ext_n = int(a.ext[0], 16), int(a.ext[1], 16)
ext_calls = {}
with open(os.path.join(a.out, 'ext.tsv'), 'w') as w:
    w.write('ext\tcase\tcalls\n')
    for i in range(ext_n):
        t = u32(ext_base + i * 4); calls = []
        for ins in md.disasm(d[OFF + t - VA: OFF + t - VA + 0x100], t):
            if ins.mnemonic == 'jal': calls.append(nm(int(ins.op_str, 16)))
            if ins.mnemonic in ('j', 'b', 'jr'): break
        ext_calls[i] = calls
        w.write('%02x\t%08x\t%s\n' % (i, t, ' '.join(calls)))

with open(os.path.join(a.out, 'patches.tsv'), 'w') as w:
    w.write('i960\tword0\tword1\tkind\tcalls\n')
    x = a.patch
    while True:
        i9, w0, w1 = struct.unpack_from('<iII', d, OFF + x - VA)
        if i9 < 0: break
        op = w0 & 0xffff
        kind = {0: 'ext %02x' % (w1 >> 2), 0x200: 'b'}.get(op, 'op %04x' % op)
        calls = ' '.join(ext_calls.get(w1 >> 2, [])) if op == 0 else ''
        w.write('%06x\t%08x\t%08x\t%s\t%s\n' % (i9, w0, w1, kind, calls))
        x += 12

with open(os.path.join(a.out, 'cop.tsv'), 'w') as w:
    w.write('op\thandler\tname\tfirmware\targs\treply_bytes\n')
    for i in range(256):
        h, argc, reply, s = struct.unpack_from('<IIII', d, OFF + a.cop - VA + i * 16)
        if not (VA <= h < VA + len(d) and VA <= s < VA + len(d)) or argc > 64: break
        w.write('%02x\t%08x\t%s\t%s\t%d\t%d\n' % (i, h, nm(h), cstr(s), argc, reply))
