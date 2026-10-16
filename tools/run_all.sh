#!/bin/bash
# Rebuild an ages-decomp tree from the two discs:
#   run_all.sh <VF2 disc dir> <FV disc dir> <work dir> <out dir>
# <work dir> gets the Ghidra project, the unpacked i960 programs and the tables. It must
# already hold fv_ida_names.tsv and fv_ida_calls.tsv, dumped with ida/dump_*.py from a
# labelled IDA database of the arcade fvipers program (MAME set fvipers, Revision D).
# Needs Ghidra 12.1.x with ghidra-emotionengine-reloaded ($GHIDRA, default ~/build/tools/ghidra),
# python3 with capstone, nm and lizard.
set -e
VD=${1:?VF2 disc}; FD=${2:?FV disc}; W=${3:?work dir}; O=${4:?out dir}
D=$(cd "$(dirname "$0")" && pwd)
G=${GHIDRA:-$HOME/build/tools/ghidra}/support/analyzeHeadless
export MAXMEM=${MAXMEM:-3G}
VE="$VD/SLPM_625.47"; FE="$FD/SLPM_626.06"
mkdir -p "$W/gproj" "$W/cvm/vf2" "$W/cvm/fv"

# The i960 programs (byte-identical to the arcade ROMs; only needed to check that).
python3 "$D/cvm.py" "$VD/BIN.CVM" "$W/cvm/vf2" > /dev/null
python3 "$D/cvm.py" "$FD/BIN.CVM" "$W/cvm/fv" > /dev/null

# Import and analyse both executables; list their functions.
for e in "$VE" "$FE"; do
    "$G" "$W/gproj" ages -import "$e" -overwrite -scriptPath "$D/ghidra" -postScript ExportFuncs.java "$W/out"
done

# Names: VF2 from its symbol table, FV by matching against VF2, then from the arcade.
nm -n "$VE" | awk 'NF==3 && $1>="00100000" {print $1"\t"$3}' > "$W/vf2_names.tsv"
python3 "$D/match.py" "$VE" "$W/out/SLPM_625.47/functions.tsv" "$FE" "$W/out/SLPM_626.06/functions.tsv" > "$W/fv_matches.tsv"
cut -f1,2 "$W/fv_matches.tsv" > "$W/fv_vf2_names.tsv"

# The patch table, the extension switch and the COP table (addresses found by hand; see README).
python3 "$D/ages_scan.py" "$VE" "$W/vf2_names.tsv" "$W/scan/vf2" --patch 20ab70 --ext 20b950 6a --cop 207a50
python3 "$D/ages_scan.py" "$FE" "$W/fv_vf2_names.tsv" "$W/scan/fv" --patch 288fe0 --ext 2898d0 8f --cop 23fc20
python3 "$D/ages_roots.py" "$W/scan/vf2" --fn-regex '^sysCop(Patch|Set)' --names "$W/vf2_names.tsv" > "$W/scan/vf2/roots.tsv"
python3 "$D/ages_roots.py" "$W/scan/fv" --fn-range 1c8000 200000 > "$W/scan/fv/roots0.tsv"
python3 "$D/i960names.py" "$W/fv_ida_names.tsv" "$W/scan/fv/roots0.tsv" > "$W/scan/fv/roots.tsv"
python3 "$D/arcade_align.py" "$FE" "$W/out/SLPM_626.06/functions.tsv" "$W/fv_vf2_names.tsv" \
    "$W/scan/fv/roots.tsv" "$W/fv_ida_calls.tsv" 1c8000 200000 > "$W/fv_arcade.tsv"
{ grep -v -P '\troot$' "$W/fv_arcade.tsv" | cut -f1,2; cat "$W/fv_vf2_names.tsv"; } > "$W/fv_names.tsv"

# Decompile, index, audit.
"$D/export.sh" "$W/gproj" "$W" "$O"
{ printf 'fv\tname\tvf2\tfv_bytes\tvf2_bytes\thow\n'; cat "$W/fv_matches.tsv"; } > "$O/FV/names_from_vf2.tsv"
{ printf 'fv\tarcade name\thow\n'; cat "$W/fv_arcade.tsv"; } > "$O/FV/names_from_arcade.tsv"
python3 "$D/audit.py" "$O" "${LIZARD:-lizard}"
