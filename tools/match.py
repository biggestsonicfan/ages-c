"""Name FV's functions from VF2's: hash each function's instructions with addresses and
immediates masked; a hash unique in both programs is a match. Then follow calls: when a
matched pair's call lists line up, the callees match too.
match.py <vf2 elf> <vf2 functions.tsv> <fv elf> <fv functions.tsv> > matches.tsv"""
import sys, struct, hashlib, collections

def load(elf, tsv):
    d = open(elf, 'rb').read()
    ph = struct.unpack_from('<I', d, 0x1c)[0]
    off, va, fsz = (struct.unpack_from('<I', d, ph + o)[0] for o in (4, 8, 16))
    fns = []
    for line in open(tsv):
        a, name, size, calls = line.rstrip('\n').split('\t')
        a = int(a, 16); size = int(size)
        if not (va <= a < va + fsz): continue
        words = struct.unpack_from('<%dI' % (size // 4), d, off + a - va)
        jals = [(w & 0x3ffffff) << 2 for w in words if w >> 26 == 3]
        fns.append((a, name, size, jals, words))
    return fns

def mask(w):
    op = w >> 26
    if op in (2, 3): return w & 0xfc000000                 # j / jal
    if op in (4, 5, 6, 7, 20, 21, 22, 23, 1): return w     # branches: relative, keep
    if op == 0 or op == 0x1c or op == 0x12 or op == 0x10 or op == 0x11: return w  # R-type, MMI, COP
    return w & 0xffff0000                                  # imm16: lui/addiu/lw/sw/...

def h(fn): return hashlib.sha1(struct.pack('<%dI' % len(fn[4]), *map(mask, fn[4]))).hexdigest()

def gapfill(A, B, match, used):
    """Between two consecutive anchors (in both programs), equal-length runs of unmatched
    functions with sizes within 25% pair up in order: the link order is the same."""
    ia = {f[0]: i for i, f in enumerate(A)}
    anchors = sorted((ia[m[0][0]], j) for j, f in enumerate(B) if f[0] in match for m in [match[f[0]]])
    for (a0, b0), (a1, b1) in zip(anchors, anchors[1:]):
        if a1 <= a0 or a1 - a0 != b1 - b0 or a1 - a0 < 2: continue
        pa, pb = A[a0+1:a1], B[b0+1:b1]
        if all(abs(x[2] - y[2]) <= max(8, x[2] // 4) for x, y in zip(pa, pb)):
            for x, y in zip(pa, pb):
                if y[0] not in match and x[0] not in used:
                    match[y[0]] = (x, 'gap'); used.add(x[0])

def lcs(x, y):
    n, m = len(x), len(y); t = [[0] * (m + 1) for _ in range(n + 1)]
    for i in range(n - 1, -1, -1):
        for j in range(m - 1, -1, -1):
            t[i][j] = t[i+1][j+1] + 1 if x[i] == y[j] else max(t[i+1][j], t[i][j+1])
    i = j = 0; out = []
    while i < n and j < m:
        if x[i] == y[j]: out.append((i, j)); i += 1; j += 1
        elif t[i+1][j] >= t[i][j+1]: i += 1
        else: j += 1
    return out

def align_callees(fa, fb, byA, byB):
    """Callee pairs of one matched pair whose masked hashes line up."""
    ca = [c for c in fa[3] if c in byA]
    cb = [c for c in fb[3] if c in byB]
    if not ca or not cb or len(ca) > 200 or len(cb) > 200:
        return []
    return [(ca[i], cb[j]) for i, j in lcs([h(byA[c]) for c in ca], [h(byB[c]) for c in cb])]

def callalign(byA, byB, match, used):
    """For each matched pair, line up the callees by masked hash; equal-hash callees match."""
    changed = True
    while changed:
        changed = False
        for b, (fa, _how) in list(match.items()):
            for x, y in align_callees(fa, byB[b], byA, byB):
                if y not in match and x not in used:
                    match[y] = (byA[x], 'callhash'); used.add(x); changed = True

def hash_match(A, B):
    """Unique masked hashes match; same-shaped groups of equal size pair in address order."""
    ha = collections.defaultdict(list); hb = collections.defaultdict(list)
    for f in A: ha[h(f)].append(f)
    for f in B: hb[h(f)].append(f)
    match = {}  # fv addr -> (vf2 fn, how)
    for k, fs in ha.items():
        if len(fs) == 1 and len(hb.get(k, [])) == 1 and fs[0][2] >= 16:
            match[hb[k][0][0]] = (fs[0], 'hash')
    for k, fs in ha.items():
        gb = hb.get(k, [])
        if 1 < len(fs) == len(gb) and fs[0][2] >= 16:
            for x, y in zip(fs, gb): match.setdefault(y[0], (x, 'order'))
    return match

def similar(x, y):
    return abs(x[2] - y[2]) <= max(16, x[2] // 4)

def callprop(byA, byB, match, used):
    """A matched pair with call lists of equal length: callees of similar size match in order."""
    changed = True
    while changed:
        changed = False
        for b, (fa, _how) in list(match.items()):
            ca, cb = fa[3], byB[b][3]
            if len(ca) != len(cb): continue
            for x, y in zip(ca, cb):
                if y in match or x in used or x not in byA or y not in byB: continue
                if not similar(byA[x], byB[y]): continue
                match[y] = (byA[x], 'call'); used.add(x); changed = True

def main():
    A = load(sys.argv[1], sys.argv[2]); B = load(sys.argv[3], sys.argv[4])
    match = hash_match(A, B)
    byA = {f[0]: f for f in A}; byB = {f[0]: f for f in B}
    used = {m[0][0] for m in match.values()}
    gapfill(A, B, match, used)
    callprop(byA, byB, match, used)
    gapfill(A, B, match, used)
    callalign(byA, byB, match, used)
    gapfill(A, B, match, used)
    for b in sorted(match):
        fa, how = match[b]
        print('%08x\t%s\t%08x\t%d\t%d\t%s' % (b, fa[1], fa[0], byB[b][2], fa[2], how))
    print('matched %d of %d FV functions' % (len(match), len(B)), file=sys.stderr)

main()
