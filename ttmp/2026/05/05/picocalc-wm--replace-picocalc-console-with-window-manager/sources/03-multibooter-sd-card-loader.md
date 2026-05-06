[PicoCalc](https://forum.clockworkpi.com/c/picocalc/31)

So I was wondering if anyone had a software in BASIC which can do this:

It would run and let you select and run a.uf2 file, from your firmware folder on your SD card, then it would open and run that file normally as usual, but to exit and go back to PicoMite you would click something like F10.

Anyone know something like this?

How do you run it? because is it a.bas file, and what is the.bas file called?

and… do i need to use a microusb cable or anything to flash it to the PicoCalc, or does this program itself run on the SD card?

SD\_boot is not currently supported by Picomite/Webmite because it does a lot of flash access and makes assumptions about where things will reside in flash (which will be wrong if things are shifted forward by the boot loader which takes up.space). It’s something I’ve been planning to look at, and trying to get working, but it’s also the kind of thing that has a small chance of bricking the Pico if done wrong, so I’d rather not risk that.

The SD\_boot dev identified a way to maybe use regular uf2 files and not require modifications and special builds like is required now. But that’s theoretical and hasn’t been made to work yet (if it’s even possible).

Currently there are very few firmware/applications that support the SD bootloader and most are proof of concept tests. I think MicroPython has a build for it though.

It has nothing to do with basic and there will never be a way to load firmware from within MMBasic unless the mainline developers wanted to do that (and that’s highly unlikely, and might not even be possible anyway). Something would always have to stay in flash like the SD\_Boot bootloader to be able to switch between other firmware. What you can do in PicoMite/WebMite though is use the command

```
UPDATE FIRMWARE
```

To put the device into bootsel mode. That way you at least don’t have to press the bootsel button on the Pico and can just plug in your USB-C cable and update the firmware normally.

Here is a multi boot loader for picocalc  
combined with PicoMite and SD boot into one single uf2

In first boot up,Default app is PicoMite, if no sd card or no firmware bin files, user can still launch PicoMite as Default app

If other bin has ran, user can launch it as the last ran app from menu \[Last app\] to avoid repeated flash read and write.

put all bin files into firmware folder on SD Card

bin files are re-compiled with offset 152k flash address

here is a pre-compiled quick try version for pico1 in PicoCalc

welcome to test

4 Likes

[Turbodeck- usb simulator for pico\_multi\_booter firmware](https://forum.clockworkpi.com/t/turbodeck-usb-simulator-for-pico-multi-booter-firmware/17755/5)

This is super nice. What is the recipe for offsetting bin files?

1 Like

use custom link script

something like this

```
function(enable_sdcard_app target)
  #pico_set_linker_script(${target} ${CMAKE_SOURCE_DIR}/memmap_sdcard_app.ld)
  if(${PICO_PLATFORM} STREQUAL "rp2040")
    pico_set_linker_script(${CMAKE_PROJECT_NAME} ${CMAKE_SOURCE_DIR}/memmap_default_rp2040.ld)
  elseif(${PICO_PLATFORM} MATCHES "rp2350")
    pico_set_linker_script(${CMAKE_PROJECT_NAME} ${CMAKE_SOURCE_DIR}/memmap_default_rp2350.ld)
  endif()
endfunction()

enable_sdcard_app(${CMAKE_PROJECT_NAME})
```

This is like release version of multi boot loader for picocalc

final flash offset set to 200k

ui improvments

3 Likes

Wow, super cool. Will this also work for the pico 2 and including the most recent mmbasic variant?

right now it’s pico1 only

and yes, most recent mmbasic included.

Thank you for the reply. Will wait for pico 2 version. Thanks for all your work on this! The picocalc is so much more capable now!

[https://github.com/clockworkpi/PicoCalc/pull/31](https://github.com/clockworkpi/PicoCalc/pull/31)

## Summary

This PR modifies the multi bootloader so that:

```
Hold UP and power on: boot into SD app booter
Hold DOWN and power on: boot into BOOTSEL
Just power on: silently boot into flashed program
```

This will make the behavior closer to default boot, where PicoCalc can be a boot-to-app gadget, while retaining the option to SDCARD-boot or BOOTSEL-boot (without clicking the reset button).

## Problems

- ~~After fresh installation, “Just power on” won’t boot the default PicoMite. Will have to hold up and flash something.~~ Edit: solved. Got another PR to clean this up.
- ~~Hold UP and power on: i2ckbd fills its queue and send multiple “up” events to sd flasher.~~ Edit: solved

## Notes

I also updated picomite to sync with [@adcockm](https://forum.clockworkpi.com/u/adcockm) 's repo.  
A PR is set up here:

After it’s verified and merged, we can then further sync it to upstream, and then decouple picomite code from the official repo and use a submodule instead.

### Quick thought:

- PicoSDK is baked into the combined image multiple times!
- For example, you can expect both `sd_boot` and `picomite` to leverage stdio, uart, spi, i2c, vfs, etc. etc.
- These are huge!

### Option 1: shared libraries

I suggest we can create a layout like this:

```
FLASH BEGIN
bootloader
picosdk (stdio, uart, spi, etc. etc.)
sd_boot
[padding]
FLASH OFFSET 200k
user app (e.g. picomite)
extended-picosdk (functionalities used by user app, but not bootloader/sd_boot)
```

This will save some good 200k of space, becuase boot/sd\_boot themselves are no more than a few KBs and most of the bits in these individual uf2 are libraries.

### Option 2: A JIT engine powered by the second core

This is wild stuff, but not impossible.

```
core0: executes code in RAM:
20000000 op0
20000004 op1
......
200ffffc WAIT for core1 to finish load the next segment
20100000 next seg op0
20100004 next seg op1
......

core1: streams code from SD to RAM, according to core0 PC
```

I haven’t tried this bootloader yet, but what happens to the flash space that PicoMite uses? For instance, if it is loaded, options will be saved to flash. Then if some other binary is loaded instead, will it wipe out the previously saved PicoMite options? What happens when PicoMite is loaded again?

If every boot to PicoMite is starting over fresh and options for it cannot be saved then I’m not sure how useful it might be. Flash is used for everything shown with “option list” and there are 3 reserved flash areas that can be used for saving code that can be “autorun”. There’s also the “library” functionality which stores code in flash. The editor might also cache what has been loaded in flash, but i might be wrong there, as it could just be using RAM instead. In any case, if all the data in flash gets overwritten, then I doubt many people would find it useful to switch between PicoMite and anything else since not state or settings can be maintained.

I’ve been looking forward to trying the multiboot because it things like the FTP server would make file management on the device possible instead of being restricted to slow single file transfers with xmodem or TFTP. But if settings cannot survive between booting different “apps”, then…?

What do you think if we could backup the data when we load a new program, and restore automatically when we change back?

2 Likes

That would be great! As long as the “scratch” flash PicoMite uses could be saved to SD in a file and then restored with the next flash of PicoMite, that should work.

This is probably something to consider for every firmware. There may be others that save settings and data to flash. And space on the SD is large enough that maybe the whole contents of flash (besides the firmware itself) could be dumped. It would need to have some way to identify the original flashed firmware file, so it could give a similar name to the data dump though.

1 Like

yup yup. just name it “app\_name.dat” for “app\_name.bin”

the only concerning part is, app upgrade. old.dat ain’t compatible with new.bin

one may have to first flash the old bin, restore data, then flash the new bin, and hope that the app can handle this!

2 Likes

Funny enough that’s less of a problem for PicoMite/Webmite. It’s not always required, but some upgrades force clearing out the flash anyway. Going from 6.00.01 to 6.00.02 required the flash wipe since options changed. Some of the RC updates for 6.00.02 didn’t need to clear out flash though. I think it’s fine if changing the version requires starting over fresh.

But if using the same version then it would be nice to have the app\_name association and load it in automatically.

I think that would be fine. Or maybe just have the user rename the old.dat file to match the new upgraded.bin file. If the firmware finds what it needs, it should work, and if it doesn’t, then it would be clearing out the flash anyway and using defaults.