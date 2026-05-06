#!/usr/bin/env bash
# Build uLisp as a legacy ClockworkPi pico_multi_booter v1.0 app.
#
# This is the important build for /firmware/*.bin on bootloader v1.0:
# - The app must be linked at 0x10000000 + 200 KiB (0x10032000).
# - The produced .bin must start with the vector table, not boot2/OTA/partition.
# - Normal UF2-to-BIN extraction is NOT compatible with the legacy multibooter.
set -euo pipefail

ROOT="${ROOT:-/home/manuel/code/wesen/2026-05-05--ulisp-picocalc}"
TICKET="${TICKET:-$ROOT/ttmp/2026/05/05/picocalc-wm--replace-picocalc-console-with-window-manager}"
BUILD_DIR="$ROOT/build-multiboot-v1"
SKETCH="$ROOT/ulisp-picocalc-sketch"
FQBN="rp2040:rp2040:rpipico"
LINKER="$TICKET/scripts/linker/memmap_default.ld.mp.rp2040"

rm -rf "$BUILD_DIR"
mkdir -p "$BUILD_DIR"

PRELINK='"{runtime.tools.pqt-python3.path}/python3" -I "{runtime.platform.path}/tools/simplesub.py" --input "'"$LINKER"'" --out "{build.path}/memmap_default.ld" --sub __FLASH_LENGTH__ {build.flash_length} --sub __RAM_LENGTH__ {build.ram_length}'

arduino-cli compile \
  --fqbn "$FQBN" \
  --build-path "$BUILD_DIR" \
  --warnings all \
  --build-property "recipe.hooks.linking.prelink.1.pattern=$PRELINK" \
  "$SKETCH"

python3 - <<PY
from pathlib import Path
import struct, sys
bin_path = Path('$BUILD_DIR/ulisp-picocalc-sketch.ino.bin')
data = bin_path.read_bytes()[:16]
sp, reset, nmi, hardfault = struct.unpack('<4I', data)
print(f'Header: SP={sp:#010x} Reset={reset:#010x} NMI={nmi:#010x} HardFault={hardfault:#010x}')
if not (0x20000000 <= sp <= 0x20042000):
    sys.exit(f'ERROR: first word is not a plausible RP2040 stack pointer: {sp:#x}')
if not (0x10032000 <= reset < 0x10200000):
    sys.exit(f'ERROR: reset vector is not in multiboot app flash range: {reset:#x}')
PY

ls -la "$BUILD_DIR"/*.bin "$BUILD_DIR"/*.uf2
