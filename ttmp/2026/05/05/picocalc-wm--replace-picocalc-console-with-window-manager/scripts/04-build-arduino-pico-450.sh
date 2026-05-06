#!/usr/bin/env bash
# Build uLisp PicoCalc with Earle Philhower arduino-pico core 4.5.0.
# This builds a normal BOOTSEL/UF2 image linked at 0x10000000.
set -euo pipefail

ROOT="${ROOT:-/home/manuel/code/wesen/2026-05-05--ulisp-picocalc}"
BUILD_DIR="$ROOT/build-4.5.0"
SKETCH="$ROOT/ulisp-picocalc-sketch"
FQBN="rp2040:rp2040:rpipico"
CORE="rp2040:rp2040@4.5.0"
INDEX_URL="https://github.com/earlephilhower/arduino-pico/releases/download/global/package_rp2040_index.json"

arduino-cli core update-index --additional-urls "$INDEX_URL"
arduino-cli core install "$CORE" --additional-urls "$INDEX_URL"

arduino-cli compile \
  --fqbn "$FQBN" \
  --build-path "$BUILD_DIR" \
  --warnings all \
  "$SKETCH"

ls -la "$BUILD_DIR"/*.uf2 "$BUILD_DIR"/*.bin
