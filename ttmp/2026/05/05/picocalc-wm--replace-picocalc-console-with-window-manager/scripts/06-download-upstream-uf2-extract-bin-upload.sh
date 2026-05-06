#!/usr/bin/env bash
# Download technoblogy upstream UF2, extract its raw BIN payload, and upload to SD.
set -euo pipefail

ROOT="${ROOT:-/home/manuel/code/wesen/2026-05-05--ulisp-picocalc}"
TICKET="${TICKET:-$ROOT/ttmp/2026/05/05/picocalc-wm--replace-picocalc-console-with-window-manager}"
SD_HOST="${SD_HOST:-manuel@192.168.0.57}"
SD_DIR="${SD_DIR:-/Volumes/NO NAME/firmware}"
URL="${URL:-https://github.com/technoblogy/ulisp-picocalc/raw/main/ulisp-picocalc.uf2}"
UF2="${UF2:-/tmp/ulisp-picocalc.upstream.uf2}"
BIN="${BIN:-/tmp/ulisp-picocalc.upstream.bin}"
BIN_NAME="${BIN_NAME:-uLisp_4.8f_upstream.bin}"

curl -L -o "$UF2" "$URL"
python3 "$TICKET/scripts/03-uf2-to-bin.py" "$UF2" "$BIN"
ssh "$SD_HOST" "touch '$SD_DIR/.write-test' && rm '$SD_DIR/.write-test'"
scp "$BIN" "$SD_HOST:$SD_DIR/$BIN_NAME"
ssh "$SD_HOST" "ls -la '$SD_DIR/$BIN_NAME'"
