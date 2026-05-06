# Changelog

## 2026-05-05

- Initial workspace created


## 2026-05-05

Step 1-2: Researched uLisp PicoCalc ecosystem, set up arduino-cli toolchain (RP2040 core 5.6.0, TFT_eSPI 2.5.34, arduino_picocalc_kbd), compiled successfully to 460KB UF2

### Related Files

- /home/manuel/code/wesen/2026-05-05--ulisp-picocalc/build/ulisp-picocalc-sketch.ino.uf2 — Compiled UF2 image


## 2026-05-05

Step 3: Wrote textbook-style guide 'Building uLisp for the PicoCalc' covering hardware, toolchain, compilation, flashing, and troubleshooting

### Related Files

- /home/manuel/code/wesen/2026-05-05--ulisp-picocalc/ttmp/2026/05/05/ulisp-picocalc--explore-ulisp-on-picocalc/reference/02-building-ulisp-picocalc-guide.md — Full textbook-style build guide


## 2026-05-05

Step 4: Researched native Linux uLisp. Found ulisp-wasm C99 port by Eliot Akira. Built 349KB native binary with clang. Running in tmux session. Wrote design doc.

### Related Files

- /home/manuel/code/wesen/2026-05-05--ulisp-picocalc/ulisp-wasm/build/ulisp-cli — Native Linux uLisp binary (349KB)


## 2026-05-05

Step 5: Wrote 'Questions and Deep Answers' — 5 textbook-style deep dives on UF2 format, benchmarks, BIOS compatibility, memory layout, and display patching


## 2026-05-05

Step 6: Saved I2C example, defuddled 6 more sources (19 total), wrote full postmortem in textbook style


## 2026-05-05

Step 5: Generated .idea/c_cpp_properties.json with 181 include paths and 105 defines for CLion IntelliSense. Wrote scripts/03-generate-c_cpp_properties.py to extract from compile_commands.json.

### Related Files

- /home/manuel/code/wesen/2026-05-05--ulisp-picocalc/.idea/c_cpp_properties.json — CLion IntelliSense config for Arduino/RP2040/pico-sdk headers
- /home/manuel/code/wesen/2026-05-05--ulisp-picocalc/ttmp/2026/05/05/ulisp-picocalc--explore-ulisp-on-picocalc/scripts/03-generate-c_cpp_properties.py — Script to regenerate c_cpp_properties.json from arduino-cli build output


## 2026-05-05

Step 5b: Fixed forward declaration resolution. Extracted 462 Arduino-generated prototypes into _ulisp_fwd_decls.h, added as forcedInclude in c_cpp_properties.json.

### Related Files

- /home/manuel/code/wesen/2026-05-05--ulisp-picocalc/build/sketch/_ulisp_fwd_decls.h — Auto-generated forward declarations for CLion IntelliSense
- /home/manuel/code/wesen/2026-05-05--ulisp-picocalc/ttmp/2026/05/05/ulisp-picocalc--explore-ulisp-on-picocalc/scripts/04-extract-forward-declarations.py — Standalone forward-decl extraction script


## 2026-05-05

Added reference document: Understanding WITH-OUTPUT-TO-STRING in uLisp PicoCalc — detailed textbook-style explanation for new team members

### Related Files

- /home/manuel/code/wesen/2026-05-05--ulisp-picocalc/ulisp-picocalc/ulisp-picocalc.ino — Covers sp_withoutputtostring

