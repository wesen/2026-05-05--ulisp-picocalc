## Porting uLisp to a new platform

Here are some instructions for porting uLisp to a new platform that it doesn't currently support.

#### Update

31st January 2022: Updated this description to reflect changes to the latest version of uLisp, 4.0/4.1.

### Requirements

To port uLisp to a new platform it needs to satisfy the following requirements:

- At least 2 Kbytes of RAM.
- At least 32 Kbytes of program memory; probably more for 32-bit platforms.
- A core for the Arduino IDE.
- Support for serial via the Arduino IDE Serial Monitor.

Check that you can upload and run the **Blink** sketch from **File** -> **Examples** -> **01.Basics** in the Arduino IDE.

### Select the most appropriate starting point

Choose an existing version of uLisp that's close to the platform that you're planning to support. Currently the choice is:

- AVR uLisp: for 16-bit AVR processors. No floating-point or array support. Images are saved in the on-chip EEPROM on ATmega328P, or flash on ATmega1284P or AVR128DA/DB.
- ARM uLisp: for 32-bit ARM chips. Images are saved to DataFlash if available, or to flash on M0.
- ESP uLisp: for 32-bit ESP8266 and ESP32 platforms. Images are saved to flash using EEPROM emulation. Includes Wi-Fi extensions.
- RISC-V uLisp: for 32/64-bit RISC-V chips.

### Specify the workspace

The parameters for each board supported by a version of uLisp are defined by a set of **#define** statements in the uLisp source beginning:

// Workspace

For example:

```
#elif defined(ARDUINO_AVR_NANO_EVERY)
  #define WORKSPACESIZE (1060-SDSIZE)     /* Objects (4*bytes) */
  #define EEPROMSIZE 256                  /* Bytes */
  #define STACKDIFF 160
  #define CPU_ATmega4809
```

Add a section to the list of supported platforms; for example:

```
#elif defined(MY_PLATFORM)
  #define WORKSPACESIZE 3072              /* Cells (8*bytes) */
```

#### Identify the platform

**MY\_PLATFORM** should be the **#define** used by the Arduino IDE to identify the platform. You can find this as follows:

Locate the Arduino core for your platform. This will be in a directory such as:

```
/Users/david/Library/Arduino15/packages
```

In the package for your board locate the **boards.txt** file.

Locate the section of definitions for your board, and then locate the line **build.board**; for example:

```
uno.build.board=AVR_UNO
```

The symbol for your board is then got by appending **ARDUINO\_** onto the front of this; for example:

```
ARDUINO_AVR_UNO
```

If in doubt invent a name, and put a:

```
#define MY_PLATFORM
```

at the start of the source file.

#### Define the workspace size

Add a line:

```
#define WORKSPACESIZE 3072
```

to specify the number of Lisp objects to allocate. On 16-bit platforms each object is 4 bytes, and on 32-bit platforms each object is 8 bytes.

On some platforms the Arduino core displays a warning if you use more than 75% of the RAM, so the best approach is to give a small value at first, and then increase it until you get the low-memory warning. Alternatively, increase it until you get erratic behaviour due to not enough stack space.

#### Specify the stack difference

Some versions of uLisp give a stack overflow error, end exit gracefully, when the stack is used up. The parameter **STACKDIFF** is used to fine-tune this:

```
#define STACKDIFF 160
```

It specifies how much stack should be available before the error is displayed. If in doubt set this to 0

#### Specify the CPU

This is optional, but the CPU symbol is used later in the uLisp source to select features specific to a particular CPU.

```
#define CPU_ATmega4809
```

At this point you should be able to compile the uLisp source and see what errors you get.

### Specify the number of available serial ports

If you don't do this you will get an error such as:

```
'serial1read' was not declared in this scope
```

The simplest thing at this stage is to specify that there is only one serial port; you can add support for **Serial1**, **Serial2**, etc later if they are available.

Locate the section in the uLisp source beginning:

```
// Streams
```

In the definition of **gstreamfun()** edit this section as follows:

```
else if (streamtype == SERIALSTREAM) {
    if (address == 0) gfun = gserial;
    #if !defined(MY_PLATFORM)
    else if (address == 1) gfun = serial1read;
    #endif
  }
```

Add a **#if** to the definition of **serial1write()** as follows:

```
#if !defined(MY_PLATFORM)
inline void serial1write (char c) { Serial1.write(c); }
#endif
```

In the definition of **pgstreamfun()** edit this section as follows:

```
else if (streamtype == SERIALSTREAM) {
    if (address == 0) pfun = pserial;
    #if !defined(MY_PLATFORM)
    else if (address == 1) pfun = serial1write;
    #endif 
  }
```

You should now be able to compile uLisp without errors.

Open the Serial Monitor and check that you get a uLisp prompt. Try typing an input and check that an appropriate response is echoed.

This is the first step; there's more you'll need to do to provide support for all uLisp's features on your platform.

To check that uLisp is working correctly try some of the benchmarks at:

[Benchmarks](http://www.ulisp.com/show?1EO1)

---

Previous: [Converting between C and uLisp](http://www.ulisp.com/show?2E0A)

Next: [uLisp Builder](http://www.ulisp.com/show?3F07)