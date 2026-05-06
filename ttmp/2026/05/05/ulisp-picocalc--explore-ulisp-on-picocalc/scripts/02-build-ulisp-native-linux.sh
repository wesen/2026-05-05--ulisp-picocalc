#!/bin/bash
# 02-build-ulisp-native-linux.sh
# Build uLisp C99 port as a native Linux binary (no Arduino, no Docker)
set -euo pipefail

ULISP_WASM="/home/manuel/code/wesen/2026-05-05--ulisp-picocalc/ulisp-wasm"
BUILD_DIR="$ULISP_WASM/build"

echo "=== Building uLisp C99 native Linux binary ==="
echo "Source: $ULISP_WASM/c99/ulisp.c"

mkdir -p "$BUILD_DIR"

clang -std=c99 -lm -O3 \
    -D_DEFAULT_SOURCE -D_XOPEN_SOURCE -D__HAS_RANDOM__=1 \
    -o "$BUILD_DIR/ulisp-cli" \
    -I "$ULISP_WASM/c99" \
    "$ULISP_WASM/c99/ulisp.c" \
    "$ULISP_WASM/c99/bestline.c"

echo ""
echo "=== Build successful! ==="
ls -la "$BUILD_DIR/ulisp-cli"
echo ""
echo "Run: $BUILD_DIR/ulisp-cli"
