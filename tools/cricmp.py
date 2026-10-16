"""Unpack a CRI 'CRICMP' file (CMP_DecSlideStd, as in the Sega Ages 2500 executables).
cricmp.py <in.CMP> <out>"""
import sys, struct

def unpack(src):
    if src[:6] == b'CRICMP':
        ver = float(src[8:12].rstrip(b'\0')); size = struct.unpack_from('<I', src, 0x14)[0]
        src = src[struct.unpack_from('<I', src, 0x18)[0]:]
    else:
        ver, size = 1.0, None
    p = 2
    if ver > 2.0: nblk = int.from_bytes(src[p:p+4], 'big'); p += 4
    else: nblk = int.from_bytes(src[p:p+2], 'big'); p += 2
    out = bytearray()
    def block(p, n):
        flags = int.from_bytes(src[p:p+2], 'big'); p += 2
        for i in range(n):
            a, b = src[p], src[p+1]; p += 2
            if flags & (1 << i):
                out.append(a); out.append(b)
                continue
            v = a << 8 | b
            if v & 0xf000 == 0:
                out.extend(bytes([out[-1]]) * (v + 3))
            else:
                dist, ln = v & 0xfff, (v >> 12) + 2
                for _ in range(ln): out.append(out[-dist])
        return p
    for _ in range(nblk): p = block(p, 16)
    pad, rest = src[p], src[p+1]; p += 2
    if rest > 0: p = block(p, rest)
    if pad: del out[-pad:]
    if size is not None and len(out) != size:
        print('warning: %d bytes, header says %d' % (len(out), size), file=sys.stderr)
    return bytes(out)

if __name__ == '__main__':
    open(sys.argv[2], 'wb').write(unpack(open(sys.argv[1], 'rb').read()))
