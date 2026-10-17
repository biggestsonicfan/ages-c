# IDA (7.x, Python 2 or 3): dump each function's calls, in address order, for arcade_align.py.
# Lines: address, name, end, callee names (space separated; ? for an indirect call).
# Set OUT, then run with File > Script file.
# The calls come from following the code from the function's start (branches both ways,
# up to ret, bx or another function's start), not from IDA's function bounds: some of
# those end early. A jump table (ld table[rX*4], rN; bx (rN)) is followed through its
# entries, as long as they point into the function's first TABLE_SPAN bytes. Only decodes; nothing in the database is changed. End is one past
# the last instruction reached. A callee that is only an entry stub (at most STUB
# instructions, no branch or call, then falling into the next function, like
# get_fcurve_value: cvtir g6, g6, then get_fcurve_value_f) is written as the function
# it falls into: a compiler that inlines the stub calls that function.
# Named code labels that are not function starts (tou_disp is a label inside
# ground_upper_disp) get a line too: the PS2 patches some of them as functions.
import idaapi
import idautils
import idc

OUT = 'fv_ida_calls.tsv'
CALLS = ('call', 'callx', 'bal', 'balx')
STOPS = ('ret', 'bx', 'fault')
LIMIT = 20000
STUB = 2
TABLE_SPAN = 0x4000


def near_target(insn):
    for i in range(3):
        if insn.ops[i].type == idaapi.o_near:
            return insn.ops[i].addr
    return None


def decode(ea):
    insn = idaapi.insn_t()
    return insn, idaapi.decode_insn(insn, ea)


def jump_table(ea, insn, start):
    """The targets of a bx (rN) whose rN was just loaded from table[rX*4]."""
    reg = insn.ops[0].reg
    for back in (8, 4):
        ld, ln = decode(ea - back)
        if ln == back and ld.get_canon_mnem() == 'ld' and ld.ops[0].type == idaapi.o_displ \
                and ld.ops[0].addr and ld.ops[1].reg == reg:
            out, t = [], ld.ops[0].addr
            while len(out) < 256:
                v = idc.get_wide_dword(t + 4 * len(out))
                if not (start <= v < start + TABLE_SPAN and v % 4 == 0):
                    break
                out.append(v)
            return out
    return []


def step(ea, fstarts, calls, work, start):
    """Decode one instruction; return the next address in sequence, or None."""
    insn, ln = decode(ea)
    if not ln:
        return None
    m = insn.get_canon_mnem()
    tgt = near_target(insn)
    if m in CALLS:
        calls.append((ea, tgt))
    elif m == 'bx' and insn.ops[0].type == idaapi.o_phrase:
        work.extend(jump_table(ea, insn, start))
        return None
    elif m in STOPS:
        return None
    elif m == 'b':
        if tgt is not None and tgt not in fstarts:
            work.append(tgt)
        return None
    elif tgt is not None:
        work.append(tgt)
    return ea + ln


def walk(start, fstarts):
    seen, calls, work = set(), [], [start]
    while work and len(seen) < LIMIT:
        ea = work.pop()
        while ea is not None and ea not in seen and (ea == start or ea not in fstarts):
            seen.add(ea)
            ea = step(ea, fstarts, calls, work, start)
    end = max(seen) + 4 if seen else start
    return [t for _, t in sorted(calls)], end


def stub_target(ea, fstarts):
    """The function an entry stub at ea falls into, or None."""
    for _ in range(STUB):
        insn, ln = decode(ea)
        if not ln or near_target(insn) is not None or insn.get_canon_mnem() in STOPS:
            return None
        ea += ln
        if ea in fstarts:
            return ea
    return None


def callee_name(t, fstarts):
    if t is None:
        return '?'
    return idc.get_name(stub_target(t, fstarts) or t)


def code_labels(fstarts):
    for ea, _name in idautils.Names():
        if ea not in fstarts and idc.is_code(idc.get_full_flags(ea)):
            yield ea


def main():
    fstarts = set(idautils.Functions())
    lines = []
    for fs in sorted(fstarts | set(code_labels(fstarts))):
        calls, end = walk(fs, fstarts)
        names = ' '.join(callee_name(t, fstarts) for t in calls)
        lines.append('%x\t%s\t%x\t%s' % (fs, idc.get_name(fs), end, names))
    with open(OUT, 'w') as w:
        w.write('\n'.join(lines) + '\n')


main()
