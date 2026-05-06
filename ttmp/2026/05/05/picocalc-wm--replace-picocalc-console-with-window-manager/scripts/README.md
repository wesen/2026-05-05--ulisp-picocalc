# Scripts for PicoCalc uLisp firmware / multibooter investigation

These scripts preserve the exact operational steps used during the SD-card boot investigation.

- `03-uf2-to-bin.py`: Extract raw payload bytes from UF2 blocks.
- `04-build-arduino-pico-450.sh`: Build uLisp with arduino-pico core 4.5.0.
- `05-upload-build-to-sd.sh`: Upload a built UF2/BIN pair to `/Volumes/NO NAME/firmware` on the Mac at `192.168.0.57`.
- `06-download-upstream-uf2-extract-bin-upload.sh`: Download upstream `technoblogy/ulisp-picocalc.uf2`, extract `.bin`, upload to SD.
- `07-convert-sd-uf2-to-bin.sh`: Convert an existing UF2 already on the SD card into a BIN and upload it back.
- `08-unmount-sd-card.sh`: Flush and unmount the SD card on the Mac.
- `09-build-multiboot-200k-attempt.sh`: Records the work-in-progress attempt to build a legacy `pico_multi_booter` compatible BIN linked at flash offset 200 KiB.
- `linker/memmap_rp2040_multiboot_200k.ld`: arduino-pico linker template modified so `FLASH` begins at `0x10000000 + 200k`.

Important finding: the legacy ClockworkPi `pico_multi_booter` does not want a normal UF2 converted to BIN. It writes the BIN to flash at `SD_BOOT_FLASH_OFFSET = 200 * 1024`, so the application must be linked for `0x10032000`. The newer `pelrun/uf2loader` is different: it can load normal UF2 files and avoids this custom-linker requirement.
