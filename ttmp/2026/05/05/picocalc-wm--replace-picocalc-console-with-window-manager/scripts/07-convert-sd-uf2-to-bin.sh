#!/usr/bin/env bash
# Pull a UF2 from the PicoCalc SD card, convert it to BIN, and upload the BIN back.
set -euo pipefail

ROOT="${ROOT:-/home/manuel/code/wesen/2026-05-05--ulisp-picocalc}"
TICKET="${TICKET:-$ROOT/ttmp/2026/05/05/picocalc-wm--replace-picocalc-console-with-window-manager}"
SD_HOST="${SD_HOST:-manuel@192.168.0.57}"
SD_DIR="${SD_DIR:-/Volumes/NO NAME/firmware}"
SRC_UF2_NAME="${SRC_UF2_NAME:-PicoCalc_uLisp_v1.1.uf2}"
DST_BIN_NAME="${DST_BIN_NAME:-uLisp_v1.1.bin}"
UF2="/tmp/$SRC_UF2_NAME"
BIN="/tmp/$DST_BIN_NAME"

scp "$SD_HOST:$SD_DIR/$SRC_UF2_NAME" "$UF2"
python3 "$TICKET/scripts/03-uf2-to-bin.py" "$UF2" "$BIN"
ssh "$SD_HOST" "touch '$SD_DIR/.write-test' && rm '$SD_DIR/.write-test'"
scp "$BIN" "$SD_HOST:$SD_DIR/$DST_BIN_NAME"
ssh "$SD_HOST" "ls -la '$SD_DIR/$DST_BIN_NAME'"
