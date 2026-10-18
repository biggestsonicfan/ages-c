#!/bin/bash
# Run m2hle headless on fvipers (or $SET) for SECONDS of attract with the given options,
# then ask the bridge for get_status and quit, so --ages-verify writes its
# report at exit (gems-c's tools/run/runverify.sh, for FV and VF2).
# Usage: runverify.sh SECONDS [m2hle args...]   e.g. runverify.sh 300 --ages-verify
# Env: M2HLE_EXE (the m2hle built with -DM2HLE_AGES_DIR), RUN (scratch dir, default
#      /dev/shm/ages-run), PORT (MCP port, default: a free one), ROMS_DIR (the zip's
#      folder), SET (fvipers or vf2, default fvipers).
#      M2HLE_GEMS_VERIFY_ALL=1 is passed through: verify every call.
secs=${1:?seconds}; shift
EXE=${M2HLE_EXE:?set M2HLE_EXE to an ages-enabled m2hle}
RUN=${RUN:-/dev/shm/ages-run}
PORT=${PORT:-$(python3 -c 'import socket; s=socket.socket(); s.bind(("127.0.0.1",0)); print(s.getsockname()[1])')}
export PORT
mkdir -p "$RUN"; cd "$RUN" || exit 1      # never the ROM folder: m2hle writes beside its cwd
"$EXE" --headless --mcp --mcp-port "$PORT" --rom "${ROMS_DIR:?set ROMS_DIR to the folder holding the set}/${SET:-fvipers}.zip" \
  --run --log "$RUN/m2hle.log" "$@" > out.txt 2>&1 &
pid=$!
trap 'kill $pid 2>/dev/null' EXIT
python3 - "$secs" <<'PY'
import os, socket, sys, time
end = time.time() + float(sys.argv[1]); port = int(os.environ['PORT'])
while time.time() < end: time.sleep(5)
for _ in range(10):
    try:
        s = socket.create_connection(('127.0.0.1', port), 5)
        s.sendall(b'{"cmd":"get_status"}\n'); print(s.recv(4000)[:600]); s.close()
        s = socket.create_connection(('127.0.0.1', port), 5)
        s.sendall(b'{"cmd":"quit"}\n'); print(s.recv(200)); break
    except Exception as e: print(e); time.sleep(1)
PY
for i in $(seq 30); do kill -0 $pid 2>/dev/null || break; python3 -c 'import time; time.sleep(1)'; done
echo "log: $RUN/m2hle.log  (grep ages)"
