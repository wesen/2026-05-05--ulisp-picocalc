# Changelog

## 2026-05-05

- Initial workspace created


## 2026-05-05

Step 1: Analyzed full console architecture — display pipeline (ScrollBuf, PlotChar, ScrollDisplay), keyboard pipeline (STM32→I2C→polling, gserial, ProcessKey), interrupt/escape handling, SPI/I2C bus topology, graphics coexistence, dual-core opportunities. Wrote 10-section design doc.

### Related Files

- /home/manuel/code/wesen/2026-05-05--ulisp-picocalc/ttmp/2026/05/05/picocalc-wm--replace-picocalc-console-with-window-manager/design/01-console-architecture.md — Console architecture analysis document


## 2026-05-05

Step 2: Wrote 10-chapter literate console walkthrough in Peter Norvig textbook style (12 small chunks). Covers boot, REPL, display pipeline, keyboard pipeline, escape handling, buses, dual-core, graphics, sound, WM requirements. Uploaded to reMarkable.

### Related Files

- /home/manuel/code/wesen/2026-05-05--ulisp-picocalc/ttmp/2026/05/05/picocalc-wm--replace-picocalc-console-with-window-manager/design/02-literate-console.md — Literate programming walkthrough of the PicoCalc console


## 2026-05-05

Step 3: Compiled uLisp (460KB uf2) and uploaded to PicoCalc SD card at /Volumes/NO NAME/firmware/PicoCalc_uLisp_4.8f_wm.uf2


## 2026-05-05

Step 4: Diagnosed stock PicoCalc bootloader v1.0 BIN issue, added uf2loader submodule, installed UF2 Loader menu assets to SD card, documented flashing instructions.

### Related Files

- /home/manuel/code/wesen/2026-05-05--ulisp-picocalc/ttmp/2026/05/05/picocalc-wm--replace-picocalc-console-with-window-manager/design/03-picocalc-bootloader-analysis.md — Bootloader analysis and flashing instructions
- /home/manuel/code/wesen/2026-05-05--ulisp-picocalc/uf2loader — UF2 Loader source submodule


## 2026-05-05

Step 5: Wrote Obsidian ARTICLE deep dive on UF2 Loader two-stage architecture, copied it into the ticket with cp, and recorded compile/install/runtime details.

### Related Files

- /home/manuel/code/wesen/2026-05-05--ulisp-picocalc/ttmp/2026/05/05/picocalc-wm--replace-picocalc-console-with-window-manager/reference/05-picocalc-uf2loader-two-stage-deep-dive.md — Ticket copy of vault report
- /home/manuel/code/wesen/obsidian-vault/Projects/2026/05/05/ARTICLE - PicoCalc UF2 Loader - Two-Stage Bootloader Deep Dive.md — Vault report


## 2026-05-05

Step 6: Downloaded official UF2 Loader Pico2 assets, uploaded BOOT2350.UF2 and Pico2W uLisp UF2 to SD card with explicit names, then unmounted SD cleanly.

### Related Files

- /home/manuel/code/wesen/2026-05-05--ulisp-picocalc/ttmp/2026/05/05/picocalc-wm--replace-picocalc-console-with-window-manager/reference/01-diary.md — Step 6 records Pico2W SD-card preparation

