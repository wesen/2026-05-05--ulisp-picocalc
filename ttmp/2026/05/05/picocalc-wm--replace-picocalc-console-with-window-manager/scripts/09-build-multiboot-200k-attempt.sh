#!/usr/bin/env bash
# ATTEMPT / WORK IN PROGRESS:
# Build uLisp linked for ClockworkPi pico_multi_booter, which loads .bin apps at
# SD_BOOT_FLASH_OFFSET = 200 KiB (0x32000). Normal UF2-to-BIN extraction is not
# enough for the legacy multibooter, because the vector table/reset vector remain
# linked for 0x10000000.
#
# Status: this documents the attempted arduino-cli route. It may still need a
# proper platform recipe override or a copied/modified arduino-pico platform.
set -euo pipefail

ROOT="${ROOT:-/home/manuel/code/wesen/2026-05-05--ulisp-picocalc}"
TICKET="${TICKET:-$ROOT/ttmp/2026/05/05/picocalc-wm--replace-picocalc-console-with-window-manager}"
BUILD_DIR="$ROOT/build-multiboot-200k"
SKETCH="$ROOT/ulisp-picocalc-sketch"
FQBN="rp2040:rp2040:rpipico"
LINKER="$TICKET/scripts/linker/memmap_rp2040_multiboot_200k.ld"

mkdir -p "$BUILD_DIR"

PRELINK='"{runtime.tools.pqt-python3.path}/python3" -I "{runtime.platform.path}/tools/simplesub.py" --input "'"$LINKER"'" --out "{build.path}/memmap_default.ld" --sub __FLASH_LENGTH__ {build.flash_length} --sub __EEPROM_START__ {build.eeprom_start} --sub __FS_START__ {build.fs_start} --sub __FS_END__ {build.fs_end} --sub __RAM_LENGTH__ {build.ram_length} --sub __PSRAM_LENGTH__ {build.psram_length}'

arduino-cli compile \
  --fqbn "$FQBN" \
  --build-path "$BUILD_DIR" \
  --warnings all \
  --build-property "recipe.hooks.linking.prelink.1.pattern=$PRELINK" \
  "$SKETCH"

ls -la "$BUILD_DIR"/*.uf2 "$BUILD_DIR"/*.bin
