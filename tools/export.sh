#!/bin/bash
# Decompile both games into an ages-decomp tree: export.sh <ghidra project dir> <work dir> <out dir>
# The project (named "ages") holds SLPM_625.47 and SLPM_626.06, imported and analysed (see README).
# <work dir> has vf2_names.tsv, fv_names.tsv and scan/{vf2,fv}/{roots,cop}.tsv.
set -e
P=${1:?project dir}; W=${2:?work dir}; O=${3:?out dir}
D=$(cd "$(dirname "$0")" && pwd)
G=${GHIDRA:-$HOME/build/tools/ghidra}/support/analyzeHeadless
export MAXMEM=${MAXMEM:-3G}
run() { # program, names, scan, game folder, title
    rm -rf "$O/$4/FN" "$O/$4/COP"
    "$G" "$P" ages -process "$1" -noanalysis -scriptPath "$D/ghidra" \
        -postScript AgesTypes.java "$W/$2" \
        -postScript AgesExport.java "$W/$2" "$W/scan/$3/roots.tsv" "$O/$4" 3
    python3 "$D/make_index.py" "$W/scan/$3/roots.tsv" FN "$O/$4/FN" "$5: native C translations of i960 functions"
    python3 "$D/make_index.py" "$W/scan/$3/roots.tsv" COP "$O/$4/COP" "$5: the Model 2 coprocessor (COP) commands" "$W/scan/$3/cop.tsv"
    cp "$W/scan/$3/patches.tsv" "$W/scan/$3/ext.tsv" "$O/$4/"
}
run SLPM_625.47 vf2_names.tsv vf2 VF2 "Virtua Fighter 2 (SLPM-62547)"
run SLPM_626.06 fv_names.tsv fv FV "Fighting Vipers (SLPM-62606)"
