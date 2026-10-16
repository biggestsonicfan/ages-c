"""List / extract a CRI CVM (ISO9660 at offset 0x1800). cvm.py <cvm> [outdir]"""
import sys, struct, os
f = open(sys.argv[1], 'rb'); base = 0x1800
def rd(lba, n): f.seek(base + lba*2048); return f.read(n)
pvd = rd(16, 2048); assert pvd[1:6] == b'CD001', pvd[:8]
root = pvd[156:156+34]
out = sys.argv[2] if len(sys.argv) > 2 else None
def walk(rec, path):
    lba, size = struct.unpack_from('<I', rec, 2)[0], struct.unpack_from('<I', rec, 10)[0]
    data = rd(lba, size); i = 0
    while i < size:
        l = data[i]
        if l == 0: i = (i//2048+1)*2048; continue
        r = data[i:i+l]; nl = r[32]; name = r[33:33+nl]
        if name not in (b'\0', b'\1'):
            nm = name.decode('ascii', 'replace').split(';')[0]
            p = path + '/' + nm
            if r[25] & 2: walk(r, p)
            else:
                fl, fs = struct.unpack_from('<I', r, 2)[0], struct.unpack_from('<I', r, 10)[0]
                print(f'{fl:8x} {fs:10d} {p}')
                if out:
                    os.makedirs(os.path.dirname(out + p), exist_ok=True)
                    open(out + p, 'wb').write(rd(fl, fs))
        i += l
walk(root, '')
