# IDA (7.x, Python 2 or 3): dump each function's calls, in address order, for arcade_align.py.
# Lines: address, name, end, callee names (space separated; ? for an indirect call).
# Set OUT, then run with File > Script file.
import idaapi
import idautils
import idc

OUT = 'fv_ida_calls.tsv'
CALLS = ('call', 'callx', 'bal', 'balx')


def callees(start):
    out = []
    for h in idautils.FuncItems(start):
        if idc.print_insn_mnem(h) in CALLS:
            t = list(idautils.CodeRefsFrom(h, 0))
            out.append(idc.get_name(t[0]) if t else '?')
    return out


lines = []
for fs in idautils.Functions():
    f = idaapi.get_func(fs)
    lines.append('%x\t%s\t%x\t%s' % (fs, idc.get_func_name(fs), f.end_ea, ' '.join(callees(fs))))
with open(OUT, 'w') as w:
    w.write('\n'.join(lines) + '\n')
