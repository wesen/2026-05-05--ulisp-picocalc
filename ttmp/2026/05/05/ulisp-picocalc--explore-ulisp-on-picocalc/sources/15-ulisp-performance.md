## Performance

### Overview

![uLispInfograph.gif](http://www.ulisp.com/pictures/3j/ulispinfograph.gif)

                

Other SAMD21-based ARM M0 boards, such as the MKRZero, Adafruit Gemma M0, Adafruit ItsyBitsy M0, and Adafruit Feather M0 Adalogger are similar in performance to the Arduino Zero.

Other SAMD51-based ARM M4 boards, such as the Adafruit ItsyBitsy M4 and Adafruit Feather M4, are similar in performance to the Adafruit Metro M4.

Other RP2040-based boards, such as the Raspberry Pi Pico W and Adafruit Feather RP2040, are similar in performance to the Raspberry Pi Pico.

Other K210-based RISC-V boards, such as the Sipeed MAiX One Dock and Sipeed MAiX BiT, are similar in performance to the Sipeed Maixduino.

### Platforms

The following table gives a summary of the performance of the different boards supported by the latest release of uLisp for each platform:

#### AVR-Nano version

| **Platform** | **Processor** | **Clock** | **Current** | **Objects** | **Image** | **Code** | **GC time** | **Tak** | **Q2** |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| [Arduino Uno](http://www.ulisp.com/show?1LG8#uno) | ATmega328 | 16 MHz | 54 mA | 320 | 256 | † | 0.5 ms | 64.6 s |  |
| [Arduino Nano](http://www.ulisp.com/show?1LG8#nano) | ATmega328 | 16 MHz |  | 319 | 256 | † | 0.5 ms | 63.3 s |  |
| [Arduino Nano Every](http://www.ulisp.com/show?33LO#arduino-nano-every) | ATmega4809 | 20 MHz |  | 1066 | 64 | † | 1.4 ms | 53.3 s | 105 s |
| [ATmega4809 Curiosity Nano](http://www.ulisp.com/show?33LO#atmega4809-curiosity-nano) | ATmega4809 | 20 MHz |  | 1066 | 64 | † | 1.4 ms | 49 s | 112 s |

#### AVR version

| **Platform** | **Processor** | **Clock** | **Current** | **Objects** | **Image** | **Code** | **GC time** | **Tak** | **Q2** |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| [Arduino Mega 2560](http://www.ulisp.com/show?1LGA) ‡ | ATmega2560 | 16 MHz | 90 mA | 1340 | 1024 | † | 2.1 ms | 53.5 s |  |
| [Lisp Badge](http://www.ulisp.com/show?2L0C) | ATmega1284 | 16 MHz |  | 2944 | All | 96 | 4.7 ms | 54.6 s | 109 s |
| [Lisp Badge LE](http://www.ulisp.com/show?4JUS) ‡ | AVR128DB48 | 24 MHz |  | 2795 | All | 96 | 2.5 ms | 33.3 s | 68.5 s |
| [AVR128DA48 Curiosity Nano](http://www.ulisp.com/show?3BMI#avr128da48-curiosity-nano) ‡ | AVR128DA48 | 24 MHz | 22 mA | 2900 | All | 96 | 2.5 ms | 33.5 s | 69.5 s |
| [AVR128DB48 Curiosity Nano](http://www.ulisp.com/show?3BMI#avr128db48-curiosity-nano) ‡ | AVR128DB48 | 24 MHz | 22 mA | 2872 | All | 96 | 2.6 ms | 34.3 s | 71.4 s |
| [AVR128DA32 Feather board](http://www.ulisp.com/show?5B7I) ‡ | AVR128DA32 | 24 MHz | 22 mA | 2900 | All § | 96 | 2.6 ms | 33.4 s | 69.1 s |

#### ARM version

| **Platform** | **Processor** | **Clock** | **Current** | **Objects** | **Image** | **Code** | **GC time** | **Tak** | **Q2** |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| [Arduino Zero](http://www.ulisp.com/show?29RK#zero) | ATSAMD21 | 48 MHz | 13 mA | 2500 | All | 128 | 2.1 ms | 11.4 s | 24.3 s |
| [Arduino MKRZero](http://www.ulisp.com/show?29RK#mkrzero) | ATSAMD21 | 48 MHz | 23 mA | 2640 | All § | 128 | 2.2 ms | 14.2 s | 32.8 s |
| [Adafruit Gemma M0](http://www.ulisp.com/show?2OFG#adafruit-gemma-m0) | ATSAMD21 | 48 MHz |  | 2816 | All | 128 | 2.4 ms | 14.9 s | 33.8 s |
| [Adafruit QT-Py SAMD21](http://www.ulisp.com/show?3NRF#qt-py-samd21) ‡ | ATSAMD21 | 48 MHz |  | 2816 | All | 128 | 1.2 ms | 11.5 s | 25.3 s |
| [Adafruit ItsyBitsy M0](http://www.ulisp.com/show?2OFG#adafruit-itsybitsy-m0) | ATSAMD21 | 48 MHz |  | 2816 | All | 128 | 1.3 ms | 14.9 s | 33.8 s |
| [Adafruit Feather M0](http://www.ulisp.com/show?2OFG#adafruit-feather-m0) | ATSAMD21 | 48 MHz |  | 2816 | All | 128 | 1.3 ms | 14.9 s | 34.3 s |
| [Adafruit Neo Trinkey](http://www.ulisp.com/show?49K5) | ATSAMD21 | 48 MHz | 14 mA | 2816 | All | 128 | 1.2 ms | 11.6 s | 25.4 s |
| [Seeed Studio XIAO SAMD21](http://www.ulisp.com/show?3NRF#xiao-samd21) | ATSAMD21 | 48 MHz |  | 2816 | All | 128 | 2.4 ms | 14 s | 41 s |
| [Adafruit Grand Central M4](http://www.ulisp.com/show?2OH5#grand-central-m4) ‡ | ATSAMD51 | 120 MHz | 33 mA | 28800 | All § | 256 | 4.6 ms | 3.4 s | 7.4 s |
| [Adafruit Metro M4](http://www.ulisp.com/show?2OH5#metro-m4) ‡ | ATSAMD51 | 120 MHz |  | 20608 | All | 256 | 3.3 ms | 3.4 s | 7.4 s |
| [Adafruit ItsyBitsy M4](http://www.ulisp.com/show?2OH5#itsybitsy-m4) | ATSAMD51 | 120 MHz |  | 20608 | All | 256 | 3.3 ms | 4.4 s | 9.9 s |
| [Adafruit Feather M4](http://www.ulisp.com/show?2OH5#feather-m4) | ATSAMD51 | 120 MHz | 24 mA | 20608 | All | 256 | 3.0 ms | 3.4 s | 7.4 s |
| [Adafruit PyGamer/PyBadge](http://www.ulisp.com/show?33OF) ‡ | ATSAMD51 | 120 MHz |  | 20608 | All § | 256 | 3.3 ms | 3.6 s | 7.6 s |
| [Adafruit PyPortal](http://www.ulisp.com/show?5IIC) ‡ | ATSAMD51 | 120 MHz |  | 20608 | All § | 256 | 3.3 ms | 3.5 s | 7.6 s |
| [Seeed Studio Wio Terminal](http://www.ulisp.com/show?38J4) ‡ | ATSAMD51 | 120 MHz | 90 mA | 20480 | All § | 256 | 3.2 ms | 3.3 s | 6.8 s |
| [BBC Micro:bit](http://www.ulisp.com/show?3CXJ#bbc-microbit) | nRF51822 | 16 MHz | 21 mA | 1344 | \* | 64 | 2.2 ms | 36.8 s | 79.5 s |
| [Calliope mini](http://www.ulisp.com/show?35MB) | nRF51822 | 16 MHz |  | 3328 | \* | 64 | 5.3 ms | 34 s | 82 s |
| [BBC Micro:bit V2](http://www.ulisp.com/show?3CXJ#bbc-microbit-v2) | nRF52833 | 64 MHz |  | 12927 | \* | 128 | 6.7 ms | 8.6 s | 18.9 s |
| [Adafruit CLUE](http://www.ulisp.com/show?2ZD1#adafruit-clue) | nRF52840 | 64 MHz |  | 21120 | All | 256 | 7.8 ms | 12.7 s | 31.8 s |
| [Adafruit ItsyBitsy nRF52840](http://www.ulisp.com/show?2ZD1#itsybitsy-nrf52840) | nRF52840 | 64 MHz |  | 21120 | All | 256 | 6.8 ms | 12.6 s | 31.4 s |
| [Seeed Studio XIAO nRF52840](http://www.ulisp.com/show?4AT6) | nRF52840 | 64 MHz | 11 mA | 21120 | All | 256 | 5.9 ms | 10.7 s | 26.3 s |
| [Circuit Playground Bluefruit](http://www.ulisp.com/show?1TZD) ‡ | nRF52840 | 64 MHz | 19 mA | 21120 | All | 256 | 5.9 ms | 6.1 s | 13.0 s |
| [Raspberry Pi Pico](http://www.ulisp.com/show?3KN3#raspberry-pi-pico) ‡ | RP2040 | 200 MHz | 33 mA | 23000 | All | 256 | 2.7 ms | 2.6 s | 5.7 s |
| [Raspberry Pi Pico W](http://www.ulisp.com/show?3KN3#raspberry-pi-pico-w) | RP2040 | 200 MHz | 32 mA | 15232 | All | 256 | 1.8 ms | 3.7 s | 9.5 s |
| [Adafruit Feather RP2040](http://www.ulisp.com/show?4UIB#adafruit-feather-rp2040) ‡ | RP2040 | 200 MHz | 41 mA | 23000 | All | 256 | 2.7 ms | 4.0 s | 9.1 s |
| [Feather RP2040 Adalogger](http://www.ulisp.com/show?4UIB#adafruit-feather-rp2040-adalogger) ‡ | RP2040 | 200 MHz | 44 mA | 23000 | All § | 256 | 2.7 ms | 3.0 s | 6.8 s |
| [Adafruit Qt-Py RP2040](http://www.ulisp.com/show?4B16#qt-py-rp2040) ‡ | RP2040 | 200 MHz | 42 mA | 23000 | All | 256 | 2.7 ms | 2.9 s | 6.6 s |
| [Seed Studio XIAO RP2040](http://www.ulisp.com/show?4B16#xiao-rp2040) | RP2040 | 200 MHz | 43 mA | 23000 | All | 256 | 2.7 ms | 3.8 s | 9.7 s |
| [Raspberry Pi Pico 2](http://www.ulisp.com/show?4X21) ARM   [Raspberry Pi Pico 2](http://www.ulisp.com/show?4X21) RISC-V | RP2350   RP2350 | 150 MHz   150 MHz | 20 mA   19 mA | 47000   42500 | All   All | 256   256 | 5.0 ms   4.3 ms | 2.5 s   4.0 s | 5.4 s   9.3 s |
| [Raspberry Pi Pico 2W](http://www.ulisp.com/show?4X21#raspberry-pi-pico-2w) ARM   [Raspberry Pi Pico 2W](http://www.ulisp.com/show?4X21#raspberry-pi-pico-2w) RISC-V | RP2350   RP2350 | 150 MHz   150 MHz | 20 mA   19 mA | 39200   34850 | All   All | 256   256 | 4.3 ms   3.5 ms | 4.4 s   4.4 s | 10.8 s   10.8 s |
| [Feather RP2350 HSTX](http://www.ulisp.com/show?58C2) ARM ‡   [Feather RP2350 HSTX](http://www.ulisp.com/show?58C2) RISC-V ‡   [Feather RP2350 HSTX](http://www.ulisp.com/show?58C2) ARM ¶‡   [Feather RP2350 HSTX](http://www.ulisp.com/show?58C2) RISC-V ¶‡ | RP2350   RP2350   RP2350   RP2350 | 150 MHz   150 MHz   150 MHz   150 MHz | 21 mA   21 mA   25 mA   25 mA | 46500   42000   1000000   1000000 | All   All   All   All | 256   256   256   256 | 5.0 ms   4.2 ms   1.1 s   1.0 s | 2.5 s   2.5 s   6.6 s   6.5 s | 5.4 s   5.5 s   14.0 s   13.7 s |
| [Pimoroni Tiny 2350](http://www.ulisp.com/show?4YCF#pimoroni-tiny-2350) ARM   [Pimoroni Tiny 2350](http://www.ulisp.com/show?4YCF#pimoroni-tiny-2350) RISC-V | RP2350   RP2350 | 150 MHz   150 MHz | 23 mA   23 mA | 46500   42500 | All   All | 256   256 | 5.0 ms   4.3 ms | 4.5 s   5.0 s | 11.5 s   11.8 s |
| [Pimoroni Pico Plus 2](http://www.ulisp.com/show?4YCF#pimoroni-pico-plus-2) ARM   [Pimoroni Pico Plus 2](http://www.ulisp.com/show?4YCF#pimoroni-pico-plus-2) RISC-V   [Pimoroni Pico Plus 2](http://www.ulisp.com/show?4YCF#pimoroni-pico-plus-2) ARM ¶   [Pimoroni Pico Plus 2](http://www.ulisp.com/show?4YCF#pimoroni-pico-plus-2) RISC-V ¶ | RP2350   RP2350   RP2350   RP2350 | 150 MHz   150 MHz   150 MHz   150 MHz | 21 mA   20 mA   24 mA   23 mA | 46500   42000   1000000   1000000 | All   All   All   All | 256   256   256   256 | 5.0 ms   4.3 ms   1.0 s   1.0 s | 3.8 s   3.6 s   7.8 s   7.8 s | 9.2 s   8.3 s   18.7 s   17.2 s |
| [Adafruit Fruit Jam](http://www.ulisp.com/show?5CSS) ARM ‡   [Adafruit Fruit Jam](http://www.ulisp.com/show?5CSS) RISC-V ‡   [Adafruit Fruit Jam](http://www.ulisp.com/show?5CSS) ARM ¶‡   [Adafruit Fruit Jam](http://www.ulisp.com/show?5CSS) RISC-V ¶‡ | RP2350   RP2350   RP2350   RP2350 | 150 MHz   150 MHz   150 MHz   150 MHz | 73 mA   73 mA   73 mA   73 mA | 46500   42500   1000000   1000000 | All §   All §   All §   All § | 256   256   256   256 | 5.0 ms   4.3 ms   1.0 s   1.0 s | 2.5 s   2.5 s   6.9 s   6.2 s | 5.4 s   5.5 s   14.7 s   13.9 s |
| [Maxim MAX32620FTHR](http://www.ulisp.com/show?2IN9) | MAX32620 | 96 MHz | 6 mA | 24704 | \* | 256 | 5.4 ms | 6.1 s | 13.5 s |
| [Teensy 4.0](http://www.ulisp.com/show?36VJ) | iMXRT1062 | 600 MHz | 101 mA | 60000 | All | 256 | 1.3 ms | 0.4 s | 0.87 s |
| [Teensy 4.1](http://www.ulisp.com/show?36VJ) | iMXRT1062 | 600 MHz | 110 mA | 60000 | All | 256 | 1.3 ms | 0.4 s | 0.88 s |
| [Arduino UNO R4 Minima](http://www.ulisp.com/show?4GBA#uno-r4-minima) | RA4M1 | 48 MHz | 34 mA | 2032 | 1024 | 128 | 0.9 ms | 10.1 s | 22.5 s |
| [Arduino UNO R4 WiFi](http://www.ulisp.com/show?4GBA#uno-r4-wifi) | RA4M1 | 48 MHz |  | 1610 | 1024 | 128 | 0.8 ms | 11.0 s | 22.8 s |

#### ESP version

| **Platform** | **Processor** | **Clock** | **Current** | **Objects** | **Image** | **Code** | **GC time** | **Tak** | **Q2** |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| [Adafruit ESP32 Feather](http://www.ulisp.com/show?4UGT#adafruit-esp32-feather) | ESP32 | 240 MHz |  | 9500 | All | † | 0.5 ms | 2.6 s | 6.0 s |
| [Adafruit ESP32 Feather V2](http://www.ulisp.com/show?4UGT#adafruit-esp32-feather-v2)[   Adafruit ESP32 Feather V2](http://www.ulisp.com/show?4UGT#adafruit-esp32-feather-v2) ¶‡ | ESP32   ESP32 | 240 MHz   240 MHz | 60 mA   63 mA | 9500   250000 | All   All | †   † | 0.4 ms   156 ms | 2.6 s   5.6 s | 6.0 s   12.4 s |
| [Adafruit QT Py ESP32 Pico](http://www.ulisp.com/show?4FKT) | ESP32 | 240 MHz |  | 260000 | All | † | 156 ms | 7.9 s | 20.9 s |
| [Muse Lab ESP32 Key](http://www.ulisp.com/show?523F#esp32-key) | ESP32 | 240 MHz | 59 mA | 9500 | All | † | 0.5 ms | 6.3 s | 16.8 s |
| [LILYGO LoRa32](http://www.ulisp.com/show?583D) ‡ | ESP32 | 240 MHz | 63 mA | 9800 | All § | † | 0.5 ms | 2.6 s | 6.0 s |
| [Adafruit ESP32-S2 Feather](http://www.ulisp.com/show?5L74#adafruit-esp32-s2-feather) ‡   [Adafruit ESP32-S2 Feather](http://www.ulisp.com/show?5L74#adafruit-esp32-s2-feather) ¶‡ | ESP32-S2   ESP32-S2 | 240 MHz   240 MHz | 45 mA   47 mA | 4150   250000 | All   All | †   † | 0.3 ms   132 ms | 2.8 s   4.1 s | 7.4 s   10.5 s |
| [Adafruit ESP32-S2 TFT Feather   Adafruit ESP32-S2 TFT Feather](http://www.ulisp.com/show?49WX#esp32-s2-tft-feather) ¶ | ESP32-S2   ESP32-S2 | 240 MHz   240 MHz | 48 mA   54 mA | 6500   250000 | All   All | †   † | 0.4 ms   131 ms | 2.4 s   4.2 s | 5.2 s   9.1 s |
| [Adafruit QT Py ESP32-S2](http://www.ulisp.com/show?5L74#adafruit-esp32-s2-qt-py) ‡[   Adafruit QT Py ESP32-S2](http://www.ulisp.com/show?5L74#adafruit-esp32-s2-qt-py) ¶‡ | ESP32-S2   ESP32-S2 | 240 MHz   240 MHz |  | 4150   250000 | All   All | †   † | 0.3 ms   132 ms | 3.0 s   4.1 s | 7.8 s   10.5 s |
| [ESP32-S2-Soala-1 WROVER](http://www.ulisp.com/show?5LBF#esp32-s2-saola-1-wrover) | ESP32-S2 | 240 MHz |  | 260000 | All | † | 137 ms | 5.4 s | 13.6 s |
| [ESP32-S2-Soala-1 WROOM](http://www.plasticki.com/show?5LBF#esp32-s2-saola-1-wroom) | ESP32-S2 | 240 MHz |  | 8160 | All | † | 0.6 ms | 3.8 s | 10.0 s |
| [Muse Lab ESP32-S2 Key](http://www.ulisp.com/show?523F#esp32-s2-key) | ESP32-S2 | 240 MHz | 50 mA | 8160 | All | † | 0.6 ms | 2.4 s | 5.8 s |
| [Adafruit ESP32-S3 Feather](http://www.ulisp.com/show?5LF6#adafruit-esp32-s3-feather) ‡   [Adafruit ESP32-S3 Feather](http://www.ulisp.com/show?5LF6#adafruit-esp32-s3-feather) ¶‡ | ESP32-S3   ESP32-S3 | 240 MHz   240 MHz | 65 mA   66 mA | 21900   250000 | All   All | †   † | 1.0 ms   138 ms | 2.3 s   3.6 s | 6.0 s   9.2 s |
| [Adafruit QT Py ESP32-S3](http://www.ulisp.com/show?5LF6#adafruit-qt-py-esp32-s3) ‡   [Adafruit QT Py ESP32-S3](http://www.ulisp.com/show?5LF6#adafruit-qt-py-esp32-s3) ¶‡ | ESP32-S3   ESP32-S3 | 240 MHz   240 MHz |  | 21900   250000 | All   All | †   † | 1.0 ms   138 ms | 2.3 s   3.6 s | 6.0 s   9.2 s |
| [ESP32-S3-DevKitM-1](http://www.ulisp.com/show?5LBF#esp32-s3-devkitm-1) ‡ | ESP32-S3 | 240 MHz | 71 mA | 24750 | All | † | 1.0 ms | 2.4 s | 5.6 s |
| [Adafruit QT Py ESP32-C3](http://www.ulisp.com/show?5LBP#adafruit-qt-py-esp32-c3) ‡ | ESP32-C3 | 160 MHz | 30 mA | 9216 | All | † | 0.9 ms | 3.2 s | 8.4 s |
| [ESP32-C3-DevKitM-1](http://www.ulisp.com/show?5LBK#esp32-c3-devkitm-1) | ESP32-C3 | 160 MHz | 39 mA | 9216 | All | † | 0.9 ms | 4.8 s | 12.1 s |
| [ESP32-C5-DevKitC-1](http://www.ulisp.com/show?5LBK#esp32-c5-devkitc-1) ‡   [ESP32-C5-DevKitC-1](http://www.ulisp.com/show?5LBK#esp32-c5-devkitc-1) ¶‡ | ESP32-C5   ESP32-C5 | 240 MHz   240 MHz | 46 mA   51 mA | 20000   500000 | All   All | †   † | 1.3 ms   287 ms | 2.0 s   3.4 s | 5.0 s   8.0 s |
| [Adafruit ESP32-C6 Feather](http://www.ulisp.com/show?5LBP#adafruit-esp32-s2-feather) ‡ | ESP32-C6 | 160 MHz | 34 mA | 24900 | All | † | 2.4 ms | 3.2 s | 8.5 s |
| [ESP32-C6-DevKitC-1](http://www.ulisp.com/show?5LBK#esp32-c6-devkitc-1) ‡ | ESP32-C6 | 160 MHz | 44 mA | 24900 | All | † | 2.3 ms | 3.6 s | 8.4 s |
| [ESP32-P4 Function EV Board](http://www.ulisp.com/show?54L4) ‡   [ESP32-P4 Function EV Board](http://www.ulisp.com/show?54L4) ¶‡ | ESP32-P4   ESP32-P4 | 360 MHz   360 MHz | 130 mA   124 mA | 26900   2000000 | All §   All § | †   † | 1.0 ms   400 ms | 1.5 s   1.8 s | 3.7 s   4.2 s |
| [Waveshare ESP32-P4-Nano](http://www.ulisp.com/show?5AV5#waveshare-esp32-p4-nano) ‡   [Waveshare ESP32-P4-Nano](http://www.ulisp.com/show?5AV5#waveshare-esp32-p4-nano) ¶‡ | ESP32-P4   ESP32-P4 | 360 MHz   360 MHz | 113 mA   118 mA | 26900   2000000 | All §   All § | †   † | 1.0 ms   203 ms | 1.5 s   1.8 s | 3.6 s   4.4 s |
| [LilyGo T-Deck   ](http://www.ulisp.com/show?4JAO)[LilyGo T-Deck](http://www.ulisp.com/show?4JAO) ¶ | ESP32-S3   ESP32-S3 | 240 MHz   240 MHz | 145 mA   145 mA | 25000   1000000 | All §   All § | †   † | 1.1 ms   230 ms | 2.1 s   3.3 s | 4.9 s   6.9 s |
| [M5Stack Cardputer](http://www.ulisp.com/show?52G4) | ESP32-S3 | 240 MHz | 86 mA | 23750 | All § | † | 1.0 ms | 2.4 s | 5.6 s |

#### RISC-V version

| **Platform** | **Processor** | **Clock** | **Current** | **Objects** | **Image** | **Code** | **GC time** | **Tak** | **Q2** |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| [Sipeed Maixduino](http://www.ulisp.com/show?30X8#sipeed-maixduino) | K210 | 400 MHz | 145 mA | 500000 | \* § | 512 | 44 ms | 1.5 s | 3.1 s |
| [Sipeed MAiX BiT](http://www.ulisp.com/show?30X8#sipeed-maix-bit) | K210 | 400 MHz | 91 mA | 500000 | \* § | 512 | 44 ms | 1.5 s | 3.0 s |

#### Key

**Current** gives the active current consumption at 5V when powered from the USB port and running a benchmark.

**Objects** gives the number of Lisp objects of storage available, each equivalent to 4 bytes on the 8/16-bit platforms and 8 bytes on the 32-bit platforms.  
¶ Shows with PSRAM enabled, where available.

**Image** gives the number of objects that can be saved to non-volatile storage using **save-image**, or **All** which means that the whole workspace can be saved.  
§ These boards include a built-in SD card interface and allow you to save the entire workspace to an SD card.  
\* These platforms don't provide non-volatile storage for saving an image, but you can save images to an SD card with a suitable interface.

**Code** gives the number of bytes of machine code that can be stored by the assembler, by default, on AVR, ARM, or RISC-V platforms.  
† Assembler not available.

**GC time** gives the time taken for a garbage collection.

**Tak** gives the time taken to calculate the recursive integer function (tak 18 12 6); see [Benchmarks-Tak](http://www.ulisp.com/show?1EO1#tak).

**Q2** gives the time taken to calculate the recursive integer function (q2 7 8); see [Benchmarks-Q2](http://www.ulisp.com/show?1EO1#q2).

‡ Results updated for uLisp 4.8f.

---

Previous: [Lisp for microcontrollers](http://www.ulisp.com/show?3M)

Next: [Using uLisp](http://www.ulisp.com/show?19XT)