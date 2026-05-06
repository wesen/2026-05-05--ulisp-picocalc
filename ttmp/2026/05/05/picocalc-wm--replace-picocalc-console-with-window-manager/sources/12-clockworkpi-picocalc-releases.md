01 Mar 07:13

[cuu](https://github.com/cuu)[multi\_uf2\_bootloader\_0.6](https://github.com/clockworkpi/PicoCalc/tree/multi_uf2_bootloader_0.6)

[`c1b8f3f`](https://github.com/clockworkpi/PicoCalc/commit/c1b8f3fc3e75a37ae04a36e1cd87f98b6177ad5e)

This release contains all files in PicoCalc SD card.

## PicoCalc SD v0.6

```
PicoCalc SD/
├── BellLabs_Fine.mp3
├── bifdiag.bas
├── BOOT2040.uf2
├── bootloader_pico.uf2
├── cc
│   ├── edit.lua
│   ├── expect.lua
│   ├── internal
│   │   ├── menu.lua
│   │   └── syntax
│   │       ├── errors.lua
│   │       ├── init.lua
│   │       ├── lexer.lua
│   │       └── parser.lua
│   └── pretty.lua
├── Chessnovice_johnybot.nes
├── fonts
│   ├── 6x10.fnt
│   ├── Acer8x8.fnt
│   ├── Haxor12.fnt
│   ├── HP6x8.fnt
│   ├── HP8x8.fnt
│   └── ProggyClean.fnt
├── lorenz.bas
├── lua
│   ├── asteroids.lua
│   ├── boxworld.bmp
│   ├── boxworld.lua
│   ├── browser.lua
│   ├── bubble.lua
│   ├── mandelbrot.lua
│   └── piano.lua
├── MACHIKAP.INI
├── main.lua
├── mand.bas
├── pico1-apps
│   ├── MicroPython_fa8b24c.uf2
│   ├── phyllosoma_kb.uf2
│   ├── PicoCalc_Fuzix_v1.0.uf2
│   ├── PicoCalc_MP3Player_v0.5.uf2
│   ├── PicoCalc_NES_v1.0.uf2
│   ├── PicoCalc_uLisp_v1.1.uf2
│   ├── picolua_daf20a2.uf2
│   ├── PicoMite_v6.02.01b4_beta.uf2
│   └── Picoware_v1.6.9.uf2
├── picocalc.bmp
├── picoware
│   ├── apps
│   │   ├── Calculator.mpy
│   │   ├── cat-fact.mpy
│   │   ├── counter.mpy
│   │   ├── flip_social
│   │   │   ├── __init__.py
│   │   │   ├── password.mpy
│   │   │   ├── run.mpy
│   │   │   ├── settings.mpy
│   │   │   └── username.mpy
│   │   ├── FlipSocial.mpy
│   │   ├── games
│   │   │   ├── 2048.mpy
│   │   │   ├── Breakout.mpy
│   │   │   ├── example.mpy
│   │   │   ├── Flappy Bird.mpy
│   │   │   ├── flip_world
│   │   │   │   ├── assets.mpy
│   │   │   │   ├── general.mpy
│   │   │   │   ├── __init__.py
│   │   │   │   ├── player.mpy
│   │   │   │   ├── run.mpy
│   │   │   │   └── sprite.mpy
│   │   │   ├── FlipWorld.mpy
│   │   │   ├── free_roam
│   │   │   │   ├── dynamic_map.mpy
│   │   │   │   ├── game.mpy
│   │   │   │   ├── __init__.py
│   │   │   │   ├── maps.mpy
│   │   │   │   ├── player.mpy
│   │   │   │   └── sprite.mpy
│   │   │   ├── Free Roam.mpy
│   │   │   ├── game_of_life.mpy
│   │   │   ├── Maze Runner.mpy
│   │   │   ├── Minesweeper.mpy
│   │   │   ├── Pong.mpy
│   │   │   ├── Snake.mpy
│   │   │   ├── Soduko.mpy
│   │   │   ├── Space Invaders.mpy
│   │   │   ├── Tetris.mpy
│   │   │   └── Tower Defense.mpy
│   │   ├── Graph.mpy
│   │   ├── hello_color.mpy
│   │   ├── keyboard-simple.mpy
│   │   ├── loading-simple.mpy
│   │   ├── menu-simple.mpy
│   │   ├── random-object.mpy
│   │   ├── screensavers
│   │   │   ├── Bubble Universe.mpy
│   │   │   ├── Clock.mpy
│   │   │   ├── Cube.mpy
│   │   │   ├── DVI Bounce.mpy
│   │   │   ├── Fire Effect.mpy
│   │   │   ├── Matrix Rain.mpy
│   │   │   ├── Patterns.mpy
│   │   │   ├── PicoFlower.mpy
│   │   │   ├── Plasma Wave.mpy
│   │   │   └── Yin-Yang.mpy
│   │   ├── Serial Terminal.mpy
│   │   ├── storage-simple.mpy
│   │   ├── textbox-simple.mpy
│   │   ├── Text Editor.mpy
│   │   └── Weather.mpy
│   └── settings
└── README.md
```

Since PicoCalc SD v0.6 we used [uf2loader](https://github.com/pelrun/uf2loader.git) as main loader.

All uf2 files in folder **pico1-apps** will be showed up in menu.

Once a uf2 got flashed and ran, next time you can use menu item **\[Default App\]** to directly run it without flashing it again.

Important files:

- bootloader\_pico.uf2  
	uf2loader main program, should be flashed into pico.
- BOOT2040.uf2  
	uf2loader Menu UI, it is a very important file, do not delete or edit it unless you know what you are doing.

## PicoCalc\_Fuzix\_v1.0.uf2

[Fuzix](https://github.com/EtchedPixels/FUZIX.git) is an open-source, lightweight Unix-like operating system specifically designed for 8-bit and other resource-constrained processors.

Patches for PicoCalc are [here](https://github.com/clockworkpi/PicoCalc/tree/master/Code/FUZIX)  
UF2 Path: /pico1-apps/PicoCalc\_Fuzix\_v1.0.uf2 ([Download](https://github.com/clockworkpi/PicoCalc/tree/master/Bin/PicoCalc%20SD/pico1-apps))

## PicoCalc\_NES\_v1.0.uf2

A simple NES emulator for PicoCalc, it will scan all nes files in the root of SD card.  
Given the resource constraints of the Pico, it is recommended to only run NES games that are less than **44KB** in size.

UF2 Path: /pico1-apps/PicoCalc\_NES\_v1.0.uf2 ([Download](https://github.com/clockworkpi/PicoCalc/tree/master/Bin/PicoCalc%20SD/pico1-apps))

## picolua\_daf20a2.uf2

[https://github.com/Lana-chan/picocalc\_lua](https://github.com/Lana-chan/picocalc_lua)  
A Lua interpreter for PicoCalc. It contains a REPL, basic API to draw graphics, read keys and access the SD filesystem.

UF2 Path: /pico1-apps/picolua\_daf20a2.uf2 ([Download](https://github.com/clockworkpi/PicoCalc/tree/master/Bin/PicoCalc%20SD/pico1-apps))

## Picoware\_v1.6.9.uf2

[https://github.com/jblanked/Picoware](https://github.com/jblanked/Picoware)

An Open-source custom firmware for the PicoCalc, Video Game Module, and other Raspberry Pi Pico devices.

Here is the version based on MicroPython.

UF2 Path: /pico1-apps/Picoware\_v1.6.9.uf2 ([Download](https://github.com/clockworkpi/PicoCalc/tree/master/Bin/PicoCalc%20SD/pico1-apps))

## phyllosoma\_kb.uf2

[MachiKania Phyllosoma](https://github.com/machikania/phyllosoma/releases) is a BASIC compiler for ARMv6-M with excellent performance., especially for Raspberry Pi Pico.

UF2 Path: /pico1-apps/phyllosoma\_kb.uf2 ([Download](https://github.com/clockworkpi/PicoCalc/tree/master/Bin/PicoCalc%20SD/pico1-apps))

## PicoCalc\_MP3Player\_v0.5.uf2

[PicoCalc\_MP3Player](https://github.com/clockworkpi/PicoCalc/tree/master/Code/MP3Player) is a simple MP3 playback program based on the \[YAHAL\]([https://git.fh-aachen.de/Terstegge/YAHAL](https://git.fh-aachen.de/Terstegge/YAHAL)

) framework.

UF2 Path: /pico1-apps/PicoCalc\_MP3Player\_v0.5.uf2 ([Download](https://github.com/clockworkpi/PicoCalc/tree/master/Bin/PicoCalc%20SD/pico1-apps))

## PicoCalc\_uLisp\_v1.1.uf2

[http://www.ulisp.com/show?56ZO](http://www.ulisp.com/show?56ZO)

A self-contained Lisp computer for PicoCalc.  
UF2 Path: /pico1-apps/PicoCalc\_uLisp\_v1.1.uf2 ([Download](https://github.com/clockworkpi/PicoCalc/tree/master/Bin/PicoCalc%20SD/pico1-apps))

## PicoMite\_v6.02.01b4\_beta.uf2

[The PicoMite](https://geoffg.net/picomite.html) is a complete operating system with a Microsoft BASIC compatible interpreter and extensive hardware support including touch sensitive LCD panels, SD Cards, WiFi/Internet and much more.

UF2 Path: /pico1-apps/PicoMite\_v6.02.01b4\_beta.uf2 ([Download](https://github.com/clockworkpi/PicoCalc/tree/master/Bin/PicoCalc%20SD/pico1-apps))

```
Copyright and Acknowledgments

The PicoMite firmware and MMBasic is copyright 2011-2025 by Geoff Graham and Peter Mather 2016-2025.
1-Wire Support is copyright 1999-2006 Dallas Semiconductor Corporation and 2012 Gerard Sexton.
FatFs (SD Card) driver is copyright 2014, ChaN.
WAV, MP3, and FLAC file support is copyright 2019 David Reid.
JPG support is thanks to Rich Geldreich
The pico-sdk is copyright 2021 Raspberry Pi (Trading) Ltd.
TinyUSB is copyright tinyusb.org
LittleFS is copyright Christopher Haster
Thomas Williams and Gerry Allardice for MMBasic enhancements
The VGA driver code was derived from work by Miroslav Nemecek
The CRC calculations are copyright Rob Tillaart
The compiled object code (the .uf2 file) for the PicoMite firmware is free software: you can use or redistribute
it as you please. The source code is on GitHub ( https://github.com/UKTailwind/PicoMiteAllVersions ) and
can be freely used subject to some conditions (see the header in the source files).
This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY, without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
```

## MicroPython\_fa8b24c.uf2

[https://github.com/zenodante/PicoCalc-micropython-driver](https://github.com/zenodante/PicoCalc-micropython-driver)

[MicroPython](https://micropython.org/) is a lean and efficient implementation of the Python 3 programming language that includes a small subset of the Python standard library and is optimised to run on microcontrollers and in constrained environments.

Here is the MicroPython Drivers compatible with PicoCalc.

UF2 Path: /pico1-apps/MicroPython\_fa8b24c.uf2 ([Download](https://github.com/clockworkpi/PicoCalc/tree/master/Bin/PicoCalc%20SD/pico1-apps))

03 Jun 15:34

[cuu](https://github.com/cuu)[multi\_booter\_v0.5](https://github.com/clockworkpi/PicoCalc/tree/multi_booter_v0.5)

[`2826755`](https://github.com/clockworkpi/PicoCalc/commit/2826755bcbcfff39efcec522f214bddd66f74048)

```
├── firmware
│   ├── Lua_180a58e.bin
│   ├── MicroPython_fa8b24c.bin
│   ├── MP3player_v0.5.bin
│   ├── PicoCalc_NES_v1.0.bin
│   ├── PicoMite_cbf6d71.bin
│   └── uLisp_4.8f.bin
└── PicoCalc_Bootloader_v0.5.uf2
```

## Lua\_180a58e.bin

[https://github.com/benob/picocalc\_lua](https://github.com/benob/picocalc_lua)  
180a58e6eff857f2106b8ce3fdfa2c3715b9e58c

```
diff --git a/CMakeLists.txt b/CMakeLists.txt
index 87f319a..8914485 100644
--- a/CMakeLists.txt
+++ b/CMakeLists.txt
@@ -18,7 +18,7 @@ add_executable(picolua
 )
 
 pico_enable_stdio_usb(picolua 0)
-pico_enable_stdio_uart(picolua 0)
+pico_enable_stdio_uart(picolua 1)
 
 target_link_libraries(picolua 
   pico_stdlib 
@@ -39,6 +39,17 @@ target_link_libraries(picolua
 
 pico_add_extra_outputs(picolua)
 
+function(enable_sdcard_app target)
+  #pico_set_linker_script(${target} ${CMAKE_SOURCE_DIR}/memmap_sdcard_app.ld)
+  if(${PICO_PLATFORM} STREQUAL "rp2040")
+    pico_set_linker_script(${CMAKE_PROJECT_NAME} ${CMAKE_SOURCE_DIR}/memmap_default_rp2040.ld)
+  elseif(${PICO_PLATFORM} MATCHES "rp2350")
+    pico_set_linker_script(${CMAKE_PROJECT_NAME} ${CMAKE_SOURCE_DIR}/memmap_default_rp2350.ld)
+  endif()
+endfunction()
+
+enable_sdcard_app(${CMAKE_PROJECT_NAME})
+
 add_custom_target(flash
     COMMAND cp picolua.uf2 /run/media/favre/RP2350/
     DEPENDS picolua
diff --git a/memmap_default_rp2040.ld b/memmap_default_rp2040.ld
new file mode 100644
index 0000000..31efe94
--- /dev/null
+++ b/memmap_default_rp2040.ld
@@ -0,0 +1,219 @@
+MEMORY
+{
+    FLASH(rx) : ORIGIN = 0x10000000 + 200k, LENGTH = 2048k - 200k
+    RAM(rwx) : ORIGIN =  0x20000000, LENGTH = 256k
+    SCRATCH_X(rwx) : ORIGIN = 0x20040000, LENGTH = 4k
+    SCRATCH_Y(rwx) : ORIGIN = 0x20041000, LENGTH = 4k
+}
+
+ENTRY(_entry_point)
+
+SECTIONS
+{
+    /* Second stage bootloader is prepended to the image. It must be 256 bytes big
+       and checksummed. It is usually built by the boot_stage2 target
+       in the Raspberry Pi Pico SDK
+    */
+
+    .flash_begin : {
+        __flash_binary_start = .;
+    } > FLASH
+
+    /* The second stage will always enter the image at the start of .text.
+       The debugger will use the ELF entry point, which is the _entry_point
+       symbol if present, otherwise defaults to start of .text.
+       This can be used to transfer control back to the bootrom on debugger
+       launches only, to perform proper flash setup.
+    */
+
+    .text : {
+        __logical_binary_start = .;
+        KEEP (*(.vectors))
+        KEEP (*(.binary_info_header))
+        __binary_info_header_end = .;
+        KEEP (*(.reset))
+        /* TODO revisit this now memset/memcpy/float in ROM */
+        /* bit of a hack right now to exclude all floating point and time critical (e.g. memset, memcpy) code from
+         * FLASH ... we will include any thing excluded here in .data below by default */
+        *(.init)
+        *(EXCLUDE_FILE(*libgcc.a: *libc.a:*lib_a-mem*.o *libm.a:) .text*)
+        *(.fini)
+        /* Pull all c'tors into .text */
+        *crtbegin.o(.ctors)
+        *crtbegin?.o(.ctors)
+        *(EXCLUDE_FILE(*crtend?.o *crtend.o) .ctors)
+        *(SORT(.ctors.*))
+        *(.ctors)
+        /* Followed by destructors */
+        *crtbegin.o(.dtors)
+        *crtbegin?.o(.dtors)
+        *(EXCLUDE_FILE(*crtend?.o *crtend.o) .dtors)
+        *(SORT(.dtors.*))
+        *(.dtors)
+
+        *(.eh_frame*)
+        . = ALIGN(4);
+    } > FLASH
+
+    .rodata : {
+        *(EXCLUDE_FILE(*libgcc.a: *libc.a:*lib_a-mem*.o *libm.a:) .rodata*)
+        . = ALIGN(4);
+        *(SORT_BY_ALIGNMENT(SORT_BY_NAME(.flashdata*)))
+        . = ALIGN(4);
+    } > FLASH
+
+    .ARM.extab :
+    {
+        *(.ARM.extab* .gnu.linkonce.armextab.*)
+    } > FLASH
+
+    __exidx_start = .;
+    .ARM.exidx :
+    {
+        *(.ARM.exidx* .gnu.linkonce.armexidx.*)
+    } > FLASH
+    __exidx_end = .;
+
+    /* Machine inspectable binary information */
+    . = ALIGN(4);
+    __binary_info_start = .;
+    .binary_info :
+    {
+        KEEP(*(.binary_info.keep.*))
+        *(.binary_info.*)
+    } > FLASH
+    __binary_info_end = .;
+    . = ALIGN(4);
+
+   .ram_vector_table (NOLOAD): {
+        *(.ram_vector_table)
+    } > RAM
+
+    .data : {
+        __data_start__ = .;
+        *(vtable)
+
+        *(.time_critical*)
+
+        /* remaining .text and .rodata; i.e. stuff we exclude above because we want it in RAM */
+        *(.text*)
+        . = ALIGN(4);
+        *(.rodata*)
+        . = ALIGN(4);
+
+        *(.data*)
+
+        . = ALIGN(4);
+        *(.after_data.*)
+        . = ALIGN(4);
+        /* preinit data */
+        PROVIDE_HIDDEN (__mutex_array_start = .);
+        KEEP(*(SORT(.mutex_array.*)))
+        KEEP(*(.mutex_array))
+        PROVIDE_HIDDEN (__mutex_array_end = .);
+
+        . = ALIGN(4);
+        /* preinit data */
+        PROVIDE_HIDDEN (__preinit_array_start = .);
+        KEEP(*(SORT(.preinit_array.*)))
+        KEEP(*(.preinit_array))
+        PROVIDE_HIDDEN (__preinit_array_end = .);
+
+        . = ALIGN(4);
+        /* init data */
+        PROVIDE_HIDDEN (__init_array_start = .);
+        KEEP(*(SORT(.init_array.*)))
+        KEEP(*(.init_array))
+        PROVIDE_HIDDEN (__init_array_end = .);
+
+        . = ALIGN(4);
+        /* finit data */
+        PROVIDE_HIDDEN (__fini_array_start = .);
+        *(SORT(.fini_array.*))
+        *(.fini_array)
+        PROVIDE_HIDDEN (__fini_array_end = .);
+
+        *(.jcr)
+        . = ALIGN(4);
+        /* All data end */
+        __data_end__ = .;
+    } > RAM AT> FLASH
+    /* __etext is (for backwards compatibility) the name of the .data init source pointer (...) */
+    __etext = LOADADDR(.data);
+
+    .uninitialized_data (NOLOAD): {
+        . = ALIGN(4);
+        *(.uninitialized_data*)
+    } > RAM
+
+    /* Start and end symbols must be word-aligned */
+    .scratch_x : {
+        __scratch_x_start__ = .;
+        *(.scratch_x.*)
+        . = ALIGN(4);
+        __scratch_x_end__ = .;
+    } > SCRATCH_X AT > FLASH
+    __scratch_x_source__ = LOADADDR(.scratch_x);
+
+    .scratch_y : {
+        __scratch_y_start__ = .;
+        *(.scratch_y.*)
+        . = ALIGN(4);
+        __scratch_y_end__ = .;
+    } > SCRATCH_Y AT > FLASH
+    __scratch_y_source__ = LOADADDR(.scratch_y);
+
+    .bss  : {
+        . = ALIGN(4);
+        __bss_start__ = .;
+        *(SORT_BY_ALIGNMENT(SORT_BY_NAME(.bss*)))
+        *(COMMON)
+        . = ALIGN(4);
+        __bss_end__ = .;
+    } > RAM
+
+    .heap (NOLOAD):
+    {
+        __end__ = .;
+        end = __end__;
+        KEEP(*(.heap*))
+        __HeapLimit = .;
+    } > RAM
+
+    /* .stack*_dummy section doesn't contains any symbols. It is only
+     * used for linker to calculate size of stack sections, and assign
+     * values to stack symbols later
+     *
+     * stack1 section may be empty/missing if platform_launch_core1 is not used */
+
+    /* by default we put core 0 stack at the end of scratch Y, so that if core 1
+     * stack is not used then all of SCRATCH_X is free.
+     */
+    .stack1_dummy (NOLOAD):
+    {
+        *(.stack1*)
+    } > SCRATCH_X
+    .stack_dummy (NOLOAD):
+    {
+        KEEP(*(.stack*))
+    } > SCRATCH_Y
+
+    .flash_end : {
+        PROVIDE(__flash_binary_end = .);
+    } > FLASH
+
+    /* stack limit is poorly named, but historically is maximum heap ptr */
+    __StackLimit = ORIGIN(RAM) + LENGTH(RAM);
+    __StackOneTop = ORIGIN(SCRATCH_X) + LENGTH(SCRATCH_X);
+    __StackTop = ORIGIN(SCRATCH_Y) + LENGTH(SCRATCH_Y);
+    __StackOneBottom = __StackOneTop - SIZEOF(.stack1_dummy);
+    __StackBottom = __StackTop - SIZEOF(.stack_dummy);
+    PROVIDE(__stack = __StackTop);
+
+    /* Check if data + heap + stack exceeds RAM limit */
+    ASSERT(__StackLimit >= __HeapLimit, "region RAM overflowed")
+
+    ASSERT( __binary_info_header_end - __logical_binary_start <= 256, "Binary info must be in first 256 bytes of the binary")
+    /* todo assert on extra code */
+}
+
```

## MicroPython\_fa8b24c.bin

[https://github.com/zenodante/PicoCalc-micropython-driver.git](https://github.com/zenodante/PicoCalc-micropython-driver.git)  
commit fa8b24c3d7b4b1b6d621b46ceced787dce69f4c1

in micropython/micropython/ports/rp2

```
diff --git a/ports/rp2/memmap_mp_rp2040.ld b/ports/rp2/memmap_mp_rp2040.ld
index 5c8d9f471..c9a33230f 100644
--- a/ports/rp2/memmap_mp_rp2040.ld
+++ b/ports/rp2/memmap_mp_rp2040.ld
@@ -23,7 +23,7 @@
 
 MEMORY
 {
-    FLASH(rx) : ORIGIN = 0x10000000, LENGTH = __micropy_flash_size__
+    FLASH(rx) : ORIGIN = 0x10000000 + 200k, LENGTH = __micropy_flash_size__ - 200k
     RAM(rwx) : ORIGIN =  0x20000000, LENGTH = 256k
     SCRATCH_X(rwx) : ORIGIN = 0x20040000, LENGTH = 4k
     SCRATCH_Y(rwx) : ORIGIN = 0x20041000, LENGTH = 4k
@@ -42,14 +42,6 @@ SECTIONS
         __flash_binary_start = .;
     } > FLASH
 
-    .boot2 : {
-        __boot2_start__ = .;
-        KEEP (*(.boot2))
-        __boot2_end__ = .;
-    } > FLASH
-
-    ASSERT(__boot2_end__ - __boot2_start__ == 256,
-        "ERROR: Pico second stage bootloader must be 256 bytes in size")
 
     /* The second stage will always enter the image at the start of .text.
        The debugger will use the ELF entry point, which is the _entry_point
```

## MP3player\_v0.5.bin

apply the [https://github.com/clockworkpi/PicoCalc/blob/master/Code/MP3Player/320dcf025478994c10930c5d6ffccd511d456b0a.patch](https://github.com/clockworkpi/PicoCalc/blob/master/Code/MP3Player/320dcf025478994c10930c5d6ffccd511d456b0a.patch)  
based on current code in

[https://github.com/clockworkpi/PicoCalc/blob/master/Code/MP3Player](https://github.com/clockworkpi/PicoCalc/blob/master/Code/MP3Player)

in YAHAL\_DIR  
replace ${YAHAL\_DIR}/cmake/boards/rpi-pico.ld with [https://github.com/clockworkpi/PicoCalc/blob/master/Code/pico\_multi\_booter/linker\_scripts/rpi-pico.ld](https://github.com/clockworkpi/PicoCalc/blob/master/Code/pico_multi_booter/linker_scripts/rpi-pico.ld)

## PicoCalc\_NES\_v1.0.bin

[https://github.com/cuu/shapones/tree/for\_multi\_boot](https://github.com/cuu/shapones/tree/for_multi_boot)

## PicoMite\_cbf6d71.bin

https://...

31 May 12:48

[cuu](https://github.com/cuu)[multi\_booter\_v0.4](https://github.com/clockworkpi/PicoCalc/tree/multi_booter_v0.4)

[`e403bc0`](https://github.com/clockworkpi/PicoCalc/commit/e403bc04d8f1f1ee6617bf1484250731db319fa3)

```
├── firmware
│   ├── lua_180a58e.bin
│   ├── micropython_1aa4069.bin
│   ├── mp3-player_v0.5.bin
│   ├── picocalc_nes_v1.0.bin
│   ├── PicoMite_abe6566.bin
│   └── ulisp_4.7d.bin
└── PicoCalc_Bootloader_v0.4.uf2
```

26 May 07:03

[cuu](https://github.com/cuu)[multi\_booter\_v0.3](https://github.com/clockworkpi/PicoCalc/tree/multi_booter_v0.3)

[`0090c02`](https://github.com/clockworkpi/PicoCalc/commit/0090c0237a1796054cd1a74f58985dab71897f5f)

In this release, flash offset changed from 940k to 152k

re-complied all \*mp.bin files

add Default app and Last app

In first boot up,Default app is PicoMite, if no sd card or no firmware bin files, user can still launch PicoMite as Default app

If other bin has ran, user can launch it as the last ran app from menu \[Last app\] to avoid repeated flash read and write.

put all bin files into **firmware** folder on SD Card

## changelog:

## PicoMite\_mp.bin

```
diff --git a/CMakeLists.txt b/CMakeLists.txt
index d063c10..3deadc0 100644
--- a/CMakeLists.txt
+++ b/CMakeLists.txt
@@ -7,14 +7,14 @@ set(PICOCALC true)
 # PicoMite will need to be shifted by this amount, and all references to
 # positions in flash updated to accomodate. THIS OPTION IS NOT READY YET.
 # details: https://github.com/adwuard/Picocalc_SD_Boot
-set(SDBOOT true)
+#set(SDBOOT true)
 
 # Compile for PICO 1 Board
-set(COMPILE PICO)
+#set(COMPILE PICO)
 
 # Compile for PICO 2 Board
 #set(COMPILE PICORP2350)
-#set(COMPILE WEBRP2350)
+set(COMPILE WEBRP2350)
 
 if (COMPILE STREQUAL "HDMI" OR COMPILE STREQUAL "WEBRP2350" OR COMPILE STREQUAL "HDMIUSB"  OR COMPILE STREQUAL "VGARP2350"  OR COMPILE STREQUAL "VGAUSBRP2350"  OR COMPILE STREQUAL "PICORP2350"  OR COMPILE STREQUAL "PICOUSBRP2350" )
     set(PICO_PLATFORM rp2350)
diff --git a/configuration.h b/configuration.h
index 08b7dac..5b4e809 100644
--- a/configuration.h
+++ b/configuration.h
@@ -162,7 +162,7 @@ extern "C" {
     #ifdef rp2350
         #define HEAP_MEMORY_SIZE (288*1024) 
         #define MAXVARS             768                     // 8 + MAXVARLEN + MAXDIM * 4  (ie, 64 bytes) - these do not incl array members
-        #define FLASH_TARGET_OFFSET (920 * 1024) 
+        #define FLASH_TARGET_OFFSET (832 * 1024) 
         #define MAX_CPU     (rp2350a ? 396000 : 378000)
         #define MAXSUBFUN           512                     // each entry takes up 4 bytes
         #ifdef USBKEYBOARD
@@ -175,7 +175,7 @@ extern "C" {
     #else
         #define HEAP_MEMORY_SIZE (128*1024) 
         #define MAXVARS             512                     // 8 + MAXVARLEN + MAXDIM * 2  (ie, 56 bytes) - these do not incl array members
-        #define FLASH_TARGET_OFFSET (920 * 1024) 
+        #define FLASH_TARGET_OFFSET (832 * 1024) 
         #define MAX_CPU     420000
         #define MAXSUBFUN           256                     // each entry takes up 4 bytes
         #ifdef USBKEYBOARD
diff --git a/memmap_default_rp2040.ld b/memmap_default_rp2040.ld
index 3c5ab0c..a0e1357 100644
--- a/memmap_default_rp2040.ld
+++ b/memmap_default_rp2040.ld
@@ -1,6 +1,6 @@
 MEMORY
 {
-    FLASH(rx) : ORIGIN = 0x10000000 + 152k, LENGTH = 2048k - 152k
+    FLASH(rx) : ORIGIN = 0x10000000 + 256k, LENGTH = 2048k - 256k
     RAM(rwx) : ORIGIN =  0x20000000, LENGTH = 256k
     SCRATCH_X(rwx) : ORIGIN = 0x20040000, LENGTH = 4k
     SCRATCH_Y(rwx) : ORIGIN = 0x20041000, LENGTH = 4k
```

## mp3-player\_mp.bin

apply the [https://github.com/clockworkpi/PicoCalc/blob/master/Code/MP3Player/320dcf025478994c10930c5d6ffccd511d456b0a.patch](https://github.com/clockworkpi/PicoCalc/blob/master/Code/MP3Player/320dcf025478994c10930c5d6ffccd511d456b0a.patch)  
based on current code

in YAHAL\_DIR  
replace ${YAHAL\_DIR}/cmake/boards/rpi-pico.ld with [https://github.com/clockworkpi/PicoCalc/blob/master/Code/pico\_multi\_booter/linker\_scripts/rpi-pico.ld](https://github.com/clockworkpi/PicoCalc/blob/master/Code/pico_multi_booter/linker_scripts/rpi-pico.ld)

## micropython\_mp.bin

in `micropython/micropython/ports/rp2`

```
diff --git a/ports/rp2/memmap_mp_rp2040.ld b/ports/rp2/memmap_mp_rp2040.ld
index a5799cd88..29ca60f69 100644
--- a/ports/rp2/memmap_mp_rp2040.ld
+++ b/ports/rp2/memmap_mp_rp2040.ld
@@ -23,7 +23,7 @@
 
 MEMORY
 {
-    FLASH(rx) : ORIGIN = 0x10000000, LENGTH = 2048k
+    FLASH(rx) : ORIGIN = 0x10000000 + 152k, LENGTH = 2048k - 152k
     RAM(rwx) : ORIGIN =  0x20000000, LENGTH = 256k
     SCRATCH_X(rwx) : ORIGIN = 0x20040000, LENGTH = 4k
     SCRATCH_Y(rwx) : ORIGIN = 0x20041000, LENGTH = 4k
@@ -42,15 +42,6 @@ SECTIONS
         __flash_binary_start = .;
     } > FLASH
 
-    .boot2 : {
-        __boot2_start__ = .;
-        KEEP (*(.boot2))
-        __boot2_end__ = .;
-    } > FLASH
-
-    ASSERT(__boot2_end__ - __boot2_start__ == 256,
-        "ERROR: Pico second stage bootloader must be 256 bytes in size")
-
     /* The second stage will always enter the image at the start of .text.
        The debugger will use the ELF entry point, which is the _entry_point
        symbol if present, otherwise defaults to start of .text.
```

## ulisp-picocalc.mp.bin

replace `~/.arduino15/packages/rp2040/hardware/rp2040/4.4.1/lib/rp2040/memmap_default.ld`  
with [https://github.com/clockworkpi/PicoCalc/blob/master/Code/pico\_multi\_booter/linker\_scripts/memmap\_default.ld.mp.rp2040](https://github.com/clockworkpi/PicoCalc/blob/master/Code/pico_multi_booter/linker_scripts/memmap_default.ld.mp.rp2040)  
The version of arduino-pico may be different, just need to find the corresponding memmap\_default.ld,replace it and re-compile ulisp

## mp3-player\_mp.bin

Just clone [https://github.com/cuu/shapones/tree/for\_multi\_boot](https://github.com/cuu/shapones/tree/for_multi_boot), all patched

23 May 07:09

[cuu](https://github.com/cuu)[pico\_multi\_booter\_v0.2](https://github.com/clockworkpi/PicoCalc/tree/pico_multi_booter_v0.2)

[`3009ce7`](https://github.com/clockworkpi/PicoCalc/commit/3009ce75ba7bcd52316e7046b9c96ff9f6044a89)

Pre-release

Pre-release

Add more bin files for multi booter

Flash combined.uf2 into PicoCalc

Put these bin files into sd/ folder on SD card

19 May 09:56

[cuu](https://github.com/cuu)[pico\_multi\_booter\_v0.1](https://github.com/clockworkpi/PicoCalc/tree/pico_multi_booter_v0.1)

[`b57f1f9`](https://github.com/clockworkpi/PicoCalc/commit/b57f1f916e2c22889fe5087a424a81b7fd2f7365)

Pre-release

Pre-release

Here is a bootloader for PicoCalc combined slightly modified [PicoMite](https://github.com/madcock/PicoMiteAllVersions) and [SD boot](https://github.com/adwuard/Picocalc_SD_Boot)

- Pico1
- No sdcard inserted,PicoMite will show up.
- Sdcard inserted, SD boot menu will show up, load third pico app bin to run at FLASH TARGET OFFSET 2048k-940k