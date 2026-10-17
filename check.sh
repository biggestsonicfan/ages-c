#!/bin/bash
# Syntax-check the converted C inside m2-hle2: check.sh <m2-hle2 tree>
# $MINIZ_GEN: a configured m2-hle2 build's miniz-gen (default <tree>/build/miniz-gen).
# Generates its own ages_all.h in a temp dir, so parallel runs do not race.
D=$(cd "$(dirname "$0")" && pwd); T=${1:?m2-hle2 tree}
TMP=$(mktemp -d "${TMPDIR:-/tmp}/ageschk.XXXXXX") || exit 1
trap 'find "$TMP" -delete' EXIT
python3 "$D/gen_all.py" "$TMP/ages_all.h" >/dev/null || exit 1
printf '#include "emu_thread.h"\n#include "ages.h"\nint main(void){return ages_apply("fvipers");}\n' > "$TMP/check.c"
gcc -std=gnu11 -fsyntax-only -Wall -Wno-unused-function -Wno-unused-variable -Wno-unused-but-set-variable \
    -DM2HLE_AGES -DM2HLE_DEV_TOOLS=1 -I"$TMP" -I"$D" -I"$T/src" -I"$T/src/board" -I"$T/src/core" \
    -I"$T/src/ui" -I"$T/src/profiles" -I"$T/src/net" -I"$T/vendor/sokol" -I"$T/vendor/miniz" -I"${MINIZ_GEN:-$T/build/miniz-gen}" -DM2HLE_VERSION='"ages"' "$TMP/check.c" "${@:2}"
