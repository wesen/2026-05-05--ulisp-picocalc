## Lisp for microcontrollers

Lisp for Arduino, Adafruit M0/M4, Micro:bit, ESP32, RISC-V, and Teensy 4.x boards.

uLisp® (pronounced "You-Lisp") is a version of the Lisp programming language specifically designed to run on microcontrollers with a limited amount of RAM, from the Arduino Uno based on the ATmega328 up to the Teensy 4.0/4.1. You can use exactly the same uLisp program, irrespective of the platform. For the performance of each platform see [Performance](http://www.ulisp.com/show?36M3).

Because uLisp is an interpreter you can type commands in, and see the effect immediately, without having to compile and upload your program. This makes it an ideal environment for learning to program, or for setting up simple electronic devices.

Lisp is also an ideal language for learning about fundamental programming concepts. It incorporates string handling, list processing, and garbage collection, and so is also an excellent language for expressing complex ideas, such as teaching a robot to solve mazes or finding the shortest route on a map. As well as supporting a core set of Lisp functions uLisp includes Arduino extensions, making it ideal as a control language for the Arduino.

You can download and install the current version of uLisp free from the [Download and install uLisp](http://www.ulisp.com/show?1AA0) page.

### uLisp projects

 [![FruitJamProjects.jpg](http://www.ulisp.com/pictures/3j/fruitjamprojects.jpg) Adafruit Fruit Jam](http://www.ulisp.com/show?5CSS)

 [![PyPortalProjects.jpg](http://www.ulisp.com/pictures/3j/pyportalprojects.jpg) Adafruit Py Portal](http://www.ulisp.com/show?5IIC)

 [![CardputerHome.jpg](http://www.ulisp.com/pictures/3j/cardputerhome.jpg) Cardputer uLisp Machine](http://www.ulisp.com/show?52G4)

 [![PyGamerMaze2.jpg](http://www.ulisp.com/pictures/3j/pygamermaze2.jpg) Simple maze game](http://www.ulisp.com/show?3ONH)

 [![MessageBoardLipo.jpg](http://www.ulisp.com/pictures/3j/messageboardlipo.jpg) Wireless message display](http://www.ulisp.com/show?40X8)

 [![Snake2.jpg](http://www.ulisp.com/pictures/3j/snake2.jpg) Simple arcade game](http://www.ulisp.com/show?3OAZ)

 [![BarnsleyFern2.jpg](http://www.ulisp.com/pictures/3j/barnsleyfern2.jpg) Barnsley Fern](http://www.ulisp.com/show?44WD)

 [![ClueRayTrace2.jpg](http://www.ulisp.com/pictures/3j/clueraytrace2.jpg) Ray tracing with uLisp](http://www.ulisp.com/show?2NWA)

### Versions

There are five versions of uLisp to cater for different platforms. The following table describes the capabilities of each version:

| **Version** | **AVR-Nano** | **AVR** | **ARM** | **ESP** | **RISC-V** |
| --- | --- | --- | --- | --- | --- |
| Garbage Collection | Yes | Yes | Yes | Yes | Yes |
| Integers | Yes | Yes | Yes | Yes | Yes |
| Save Workspace | Yes | Yes | Yes | Yes | No |
| Floating Point | No | No | Yes | Yes | Yes |
| Arrays | No | Yes | Yes | Yes | Yes |
| Built-in Documentation | No | Yes | Yes | Yes | Yes |
| Machine Code | No | Yes | Yes | No | Yes |
| Graphics Extensions | No | No | Yes | Yes | Yes |
| Wi-Fi Extensions | No | No | Yes | Yes | No |

#### AVR-Nano version

The AVR-Nano version of uLisp supports boards with as little as 2 Kbytes of RAM and 32 Kbytes of program memory. All the **Simple examples** will run on these platforms:

**[Arduino Uno and Nano](http://www.ulisp.com/show?1LG8)** or other ATmega328-based cards. These will give you enough memory for a simple uLisp application.

**[ATmega4809 boards](http://www.ulisp.com/show?33LO)**. The Arduino Nano Every and Microchip Curiosity Nano evaluation board are low-cost platforms based on the ATmega4809.

#### AVR version

The AVR version of uLisp supports the following boards:

**[Arduino Mega 2560](http://www.ulisp.com/show?1LGA)** or other ATmega2560-based boards. These will give you enough memory for a fairly complex application; for examples see [Animals](http://www.ulisp.com/show?1LKX), [Tweetmaze](http://www.ulisp.com/show?1ANR), [Route finder](http://www.ulisp.com/show?1BM4), and [Infinite precision arithmetic](http://www.ulisp.com/show?1FHA).

**[ATmega1284](http://www.ulisp.com/show?1LGC)**. Although there isn't an official Arduino board based on it, the ATmega1284 is easy to wire up on a prototyping board, and provides a generous 16 Kbytes RAM.

**[AVR DA and DB series boards](http://www.ulisp.com/show?3BMI)**. The AVR128DA48 and AVR128DB48 Curiosity Nano boards are based on the Microchip AVR128DA48 and AVR128DB48 running at 24 MHz, and they have 128 Kbytes of flash and 16 Kbytes of RAM.

#### ARM version

The ARM version of uLisp supports the following boards:

**[Arduino M0 boards](http://www.ulisp.com/show?29RK)**. The **Arduino Zero** and **Arduino MKRZero** are based on the SAMD21 ARM Cortex-M0+ core and provide 256 Kbytes of flash and 32 Kbytes of RAM. They save images to the program flash. The **Arduino MKRZero** incorporates an SD-card socket which allows you to use an SD card for saving and loading uLisp images and files.

**[Adafruit M0 boards](http://www.ulisp.com/show?2OFG)**. The **Adafruit Feather M0 Express**, **Adafruit Feather M0**, **Adafruit ItsyBitsy M0,** **Adafruit Gemma M0**, and **Adafruit QT-Py SAMD21** are each based on the ATSAMD21 48 MHz ARM Cortex M0+ microcontroller. They have similar features and performance. The **Adafruit Feather M0 Express** uses the on-board DataFlash to save Lisp images; the other boards save images to the program flash.

**[QT Py SAMD21 and XIAO SAMD21](http://www.ulisp.com/show?3NRF)**. The **Adafruit QT-Py SAMD21** and **Seeduino XIAO SAMD21** are each based on the ATSAMD21 48 MHz ARM Cortex M0+ microcontroller, with 256 Kbytes of flash and 32 Kbytes of RAM. They save images to the program flash.

**[Adafruit Neo Trinkey](http://www.ulisp.com/show?49K5)**. This is a USB-key sized board containing an ATSAMD21 ARM M0+ chip running at 48MHz, with 256KB Flash and 32 KB RAM, and four NeoPixel WS2812 serial addressable RGB displays.

**[Adafruit M4 boards](http://www.ulisp.com/show?2OH5)**. The **Adafruit Metro M4 Grand Central**, **Adafruit Metro M4**, **Adafruit Feather M4**, and **Adafruit ItsyBitsy M4** are each based on the ATSAMD51 120 MHz ARM Cortex M4 microcontroller. They all include on-board DataFlash which is used to save Lisp images.

**[Adafruit PyGamer and PyBadge](http://www.ulisp.com/show?33OF)**. The **Adafruit PyGamer** and **Adafruit PyBadge** are handheld game platforms based on the ATSAMD51 120 MHz ARM Cortex M4 microcontroller and incorporating a 160x128 colour TFT display. They include on-board DataFlash which is used to save Lisp images.

**[Adafruit nRF52840 boards](http://www.ulisp.com/show?2ZD1)**, **[Circuit Playground Bluefruit](http://www.ulisp.com/show?1TZD)**, and **[Seeed Studio XIAO nRF52840](http://www.ulisp.com/show?4AT6)**. These are based on the Nordic Semiconductor nRF52840 64 MHz ARM Cortex-M4 microcontroller, with 1 Mbyte of flash program memory and 256 Kbytes of RAM. They include on-board DataFlash which is used to save Lisp images.

**[BBC Micro:bit and Calliope Mini](http://www.ulisp.com/show?3CXJ)**. The **BBC Micro:bit** and **Calliope Mini** are is based on a Nordic Semiconductor nRF51822 ARM Cortex-M0 microcontroller. They run at 16 MHz and provide 256 Kbytes of flash program memory and 16 Kbytes of RAM. The **BBC Micro:bit V2** is based on a Nordic Semiconductor nRF52833 ARM Cortex-M4 microcontroller, running at 64 MHz, and provides 512 Kbytes of flash program memory, and 128 Kbytes of RAM. They don't support **save-image**.

**[Maxim MAX32620FTHR](http://www.ulisp.com/show?2IN9).** This is based on a Maxim MAX32620 ARM Cortex-M4F microcontroller running at 96 MHz, with 2048 Kbytes of flash memory and 256 Kbytes of RAM. It doesn't support **save-image**.

**[Teensy 4.0 and 4.1](http://www.ulisp.com/show?36VJ)**. These are based on the NXP iMXRT1062 ARM M7 processor running at 600 MHz, and with 1 Mbytes of RAM.

**[Raspberry Pi RP2040 boards](http://www.ulisp.com/show?3KN3)**, **[Adafruit RP2040 Feather boards](http://www.ulisp.com/show?4UIB)**, **[QT Py RP2040 and XIAO RP2040](http://www.ulisp.com/show?4B16)**, and **[LILYGO T-Display RP2040](http://www.ulisp.com/show?3JOX)**. These are based on the Raspberry Pi RP2040 microcontroller, which is a dual-core ARM Cortex M0+ running at 125 MHz which provides up to 16 MB of off-chip flash and 246 KB on-chip RAM.

**[Raspberry Pi RP2350 boards](http://www.ulisp.com/show?4X21)**, **[Adafruit Feather RP2350 HSTX](http://www.ulisp.com/show?58C2)**, and **[Pimoroni RP2350 boards](http://www.ulisp.com/show?4YCF)**. These are based on the Raspberry Pi RP2350 microcontroller, which is a dual core ARM Cortex-M33 or dual core RISC-V Hazard3 processor running at 150 MHz which provides up to 16 MB of off-chip flash and 520 KB on-chip RAM.

**[Arduino Uno R4 boards](http://www.ulisp.com/show?4GBA)**. These are based on the Renesas RA4M1 ARM Cortex M4 CPU, with 256 KB flash memory, 32 KB RAM, and a 48 MHz clock, and are unusual in operating from 5 V.

#### ESP version

The ESP version of uLisp supports the following ESP32 boards:

**[Adafruit ESP32 Feather boards](http://www.plasticki.com/show?4UGT)**, **[Adafruit QT Py ESP32 Pico](http://www.plasticki.com/show?4FKT)**, and **[Other ESP32 boards](http://www.ulisp.com/show?2AJI)**. These boards are based on the 32-bit Tensilica Xtensa LX6 microprocessor running at 160 or 240 MHz, with 4 Mbytes of flash and 520 Kbytes of RAM. They include integrated Wi-Fi and dual-mode Bluetooth.

**[ESP32-S2 and ESP-C3 boards](http://www.ulisp.com/show?3TQF)**. These boards are based on Tensilica LX7 and RISC-V microprocessors running at 240 MHz or 160 MHz respectively. They include integrated Wi-Fi and native USB, and some include Bluetooth.

**[ESP32 boards with a TFT display](http://www.ulisp.com/show?49WX)**. These ESP32 and ESP32-S2 boards include an integrated colour TFT display that can take advantage of uLisp's [Graphics extensions](http://www.ulisp.com/show?31GT).

#### RISC-V version

The RISC-V version of uLisp supports the following boards:

**[Sipeed MAiX RISC-V boards](http://www.ulisp.com/show?30X8)**. These boards are based on the Kendryte K210 RISC-V Dual Core 64 bit 400 MHz processor and provide 8 Mbytes RAM and 16 Mbytes flash. They are similar in performance.

### Other platforms

For additional platforms supported by earlier releases of uLisp see: [Older releases](http://www.ulisp.com/show?26CP#other-platforms).

### Performance

See [Performance](http://www.ulisp.com/show?36M3).

### Specification

The language is generally a subset of Common Lisp, and uLisp programs should also run under Common Lisp. But note that there is one namespace for functions and variables; in other words, you cannot use the same name for a function and a variable.

#### Features

All platforms provide lists, symbols, integers, characters, strings, streams, and arrays, including bit-arrays (not available in the AVR-Nano version). In addition the 32-bit platforms provide floating-point numbers.

All platforms provide Arduino extensions for accessing ports, running timers, and serial I/O.

Some platforms provide additional Wi-Fi extensions, graphics extensions, and an assembler.

#### Integers

An integer is a sequence of digits, optionally prefixed with "+" or "-".

- On the 8/16-bit platforms integers can be between -32768 and 32767.
- On the 32-bit platforms integers can be between 2147483647 and -2147483648.

You can enter integers in hexadecimal, octal, or binary with the notations #x2A, #o52, or #b101010, all of which represent 42.

#### Floating-point numbers

The 32-bit platforms (ARM, ESP, and RISC-V) also support floating-point numbers with full 32-bit precision.

#### Symbols

All platforms support long user-defined symbol names. Almost any sequence of characters that doesn't represent a number can be used as a symbol; so, for example, 12a and \*2 are valid symbols.

Symbols consisting of up to three of the characters 0-9, a-z, -, \*, and $ are represented more efficiently, and occupy less workspace.

#### Strings

Strings can consist of an arbitrary sequence of ASCII characters. Strings can be unlimited length, and are automatically garbage-collected.

#### Arrays

All platforms apart from the AVR-Nano version support arrays with an arbitrary number of dimensions. In addition, uLisp includes bit arrays which provide an extremely compact representation for bit values.

#### Garbage collection

uLisp includes a mark and sweep garbage collector. Garbage collection takes under 1 msec on an Arduino Uno or under 3 msec on an Arduino Mega 2560 (see [Performance](http://www.ulisp.com/show?36M3)).

uLisp provides tail-call optimization, so applications written using recursive functions can be as efficient as using iteration.

#### Debugging features

uLisp includes a simple program editor (see [Using the program editor](http://www.ulisp.com/show?1J1Z)), a trace facility, a pretty printer (see [Debugging in uLisp](http://www.ulisp.com/show?19X5)), and built-in documentation (not available in the AVR-Nano version) (see [Integral documentation](http://www.ulisp.com/show?41E0)).

#### Saving the workspace

On most platforms you can save the workspace, containing the functions and variables you have defined, using the **save-image** function, and then reload it with **load-image**. These functions take advantage of whatever non-volatile storage is available on each platform:

- On platforms with a DataFlash chip, such as Adafruit's ATSAMD21, ATSAMD51, and nRF52840 Express boards, you can save the entire workspace to DataFlash.
- On the ATmega1284P, AVR128DA48, AVR128DB48, the Raspberry Pi Pico, the ESP8266 and ESP32, and platforms based on the ATSAMD21 such as the Arduino Zero and Seeeduino XIAO, you can save the workspace to the flash program memory.
- On older AVR-based platforms, such as the Arduino Uno and Arduino Mega 2560, you can save the workspace to EEPROM.
- On any platform you can add an SD-card interface, and then save the workspace to an SD card.

#### Loading Lisp functions

On any platform you can also extend uLisp with your own library of function or variable definitions written in Lisp. They are stored as text in flash memory along with the uLisp code, and get loaded when you run uLisp. For more information see [Lisp Library](http://www.ulisp.com/show?27OV).

### Example

The following example illustrates how you might use uLisp.

After uploading uLisp to your microcontroller board you can communicate it via by typing or pasting commands into the Serial Monitor, or using a serial terminal. For more information see [Using uLisp](http://www.ulisp.com/show?19XT).

Suppose you have a red LED connected to the analogue output pin 9 on an Arduino Uno. Then you can type in the Lisp command:

```
(analogwrite 9 128)
```

to set the LED to 128, which corresponds to half brightness.

To save having to write this command every time you want to set the red LED you can define a function called **red**:

```
(defun red (x) (analogwrite 9 x))
```

Now you can achieve the same effect simply by writing:

```
(red 128)
```

In each case the LED changes immediately, as soon as you type in the command.

Suppose you've got a potentiometer connected to vary the voltage on the analogue input A0. You could define a function **dim** to make the potentiometer adjust the brightness of the LED with:

```
(defun dim () (loop (red (/ (analogread 0) 4)))
```

and run it by typing:

```
(dim)
```

Finally, you could save the uLisp image to EEPROM, and specify that **dim** should be run on load, by entering:

```
(save-image 'dim)
```

When you reset the Arduino **dim** will now load and run automatically.

This is a simple example showing how uLisp allows you to build up complex programs from simpler components, testing each of the components as you go along.

---

Next: [Performance](http://www.ulisp.com/show?36M3)