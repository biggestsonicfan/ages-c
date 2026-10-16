# IDA (7.x, Python 2 or 3): dump the arcade program's names for i960names.py.
# Lines: address, name, F (function start) or L (label), function end, comment.
# Set OUT, then run with File > Script file.
import idautils
import idc

OUT = 'fv_ida_names.tsv'


def comment(a, is_func):
    if is_func:
        c = idc.get_func_cmt(a, 0) or idc.get_func_cmt(a, 1) or ''
    else:
        c = idc.get_cmt(a, 0) or idc.get_cmt(a, 1) or ''
    return c.replace('\n', ' ').replace('\t', ' ')


lines = []
for a, n in idautils.Names():
    is_func = idc.get_func_attr(a, idc.FUNCATTR_START) == a
    end = idc.get_func_attr(a, idc.FUNCATTR_END) if is_func else 0
    lines.append('%x\t%s\t%s\t%x\t%s' % (a, n, 'F' if is_func else 'L', end, comment(a, is_func)))
with open(OUT, 'w') as w:
    w.write('\n'.join(lines) + '\n')
