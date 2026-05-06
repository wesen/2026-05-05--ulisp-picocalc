#!/bin/bash
# 01-compile-ulisp-picocalc.sh
# Compile uLisp for PicoCalc using arduino-cli
set -euo pipefail

SKETCH="/home/manuel/code/wesen/2026-05-05--ulisp-picocalc/ulisp-picocalc-sketch"
FQBN="rp2040:rp2040:rpipico"
# Flash size: 2MB (Sketch: 1MB, FS: 1MB) for save-image support
BUILD_DIR="/home/manuel/code/wesen/2026-05-05--ulisp-picocalc/build"

mkdir -p "$BUILD_DIR"

echo "=== Compiling uLisp for PicoCalc ==="
echo "FQBN: $FQBN"
echo "Sketch: $SKETCH"
echo "Build dir: $BUILD_DIR"

arduino-cli compile \
    --fqbn "$FQBN" \
    --build-path "$BUILD_DIR" \
    --build-cache-path "$BUILD_DIR/cache" \
    --warnings all \
    "$SKETCH"

echo ""
echo "=== Build successful! ==="
echo "Output files in: $BUILD_DIR"
ls -la "$BUILD_DIR"/*.uf2 "$BUILD_DIR"/*.bin "$BUILD_DIR"/*.hex 2>/dev/null || echo "(checking for output files)"
