#!/usr/bin/env bash
# Upload a built UF2 and BIN pair to the PicoCalc SD card mounted on the Mac.
set -euo pipefail

ROOT="${ROOT:-/home/manuel/code/wesen/2026-05-05--ulisp-picocalc}"
BUILD_DIR="${BUILD_DIR:-$ROOT/build-4.5.0}"
SD_HOST="${SD_HOST:-manuel@192.168.0.57}"
SD_DIR="${SD_DIR:-/Volumes/NO NAME/firmware}"
UF2_NAME="${UF2_NAME:-PicoCalc_uLisp_4.8f_450.uf2}"
BIN_NAME="${BIN_NAME:-uLisp_4.8f_450.bin}"

UF2="$BUILD_DIR/ulisp-picocalc-sketch.ino.uf2"
BIN="$BUILD_DIR/ulisp-picocalc-sketch.ino.bin"

ssh "$SD_HOST" "touch '$SD_DIR/.write-test' && rm '$SD_DIR/.write-test'"
scp "$UF2" "$SD_HOST:$SD_DIR/$UF2_NAME"
scp "$BIN" "$SD_HOST:$SD_DIR/$BIN_NAME"
ssh "$SD_HOST" "ls -la '$SD_DIR/$UF2_NAME' '$SD_DIR/$BIN_NAME'"
