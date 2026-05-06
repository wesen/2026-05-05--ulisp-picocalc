[PicoCalc](https://forum.clockworkpi.com/c/picocalc/31)

I’ve been the proud owner of this calculator for two days now and I’m trying to understand the mechanism for loading firmware files stored on an SD card.

As I understand it, the bootloader itself is installed in the processor module’s memory, either using a button on the module itself or from PicoMite using the UPDATE FIRMWARE command. However, I don’t understand what’s stored on the SD card. The root directory displays BIN files, which appear in the bootloader menu after inserting the SD card into the device. It seems that these files, not the contents of the UF2 directory, are loaded by the bootloader.

So how do I update PicoMite, for example, if version 6.00.02RC23 is included with the device, but GitHub lists it as 6.00.03? The same question applies to the other modules/firmware available on the card. How can I install updates so that the bootloader recognizes them, since it doesn’t need the UF2 file, but the BIN file, which is not used by the Pico tool publishers on GitHub?

Another question concerns BIOS updates. Specifically, is it sufficient to replace the current BIN and MD5SUM version 1.2 files from the PicoCalc\_BIOS\_STM32 subdirectory on the SD card with the latest version 1.4, and the BIOS should be flashed automatically, or is some additional procedure required? Also, is this operation reversible in the event of a failure? What is the procedure for reverting to the previous, stable version?

Your confusion mostly comes from trying to combine information that’s been changing rapidly, and some of it is now obsolete. So a (recent) history lesson should help.

Originally the only way to load a new application was via the standard Pico bootsel mode, and there was no SD card loading available at all. Really inconvenient!

Then Aduward released SD Boot which was a resident bootloader that could load applications from the SD card. It did work well, but it needed everything to be recompiled and stored as raw binaries - these are the BIN files you have. Which means you can’t just use the normal UF2 files and you need to make a tricky change to every piece of new software you wanted to use. This is still really inconvenient for most people!

Cuu merged SD Boot with a copy of PicoMite, which is almost certainly what you have on your unit.

I then got motivated to fix the remaining issues, which resulted in my UF2 Loader - it’s SD Boot but it’s been heavily reworked so that it uses the same UF2 files that you would normally have flashed via BOOTSEL, no weird edits needed.

So basically, I recommend deleting all the BIN files off your SD card, forgetting they ever existed, installing UF2 Loader, and then putting whatever picocalc UF2 files you want to use on your SD card. Then you can update just by putting a new version of the UF2 on the card and then loading it from the menu. (There’s a couple more steps involved, but nothing terrible.)

5 Likes

As for updating the STM32 bios, it’s a bit complicated. You have to open the case, flip a dip-switch on the back to connect the stm32 programming lines to the usb-c connector, connect the picocalc to the PC, and run a utility to reprogram the chip. Finally you need to flip the dip-switch back and reassemble the case.

I do not miss the bad old days of early microcontroller development…

1 Like

Thank You Pelrun for all the information.

You’ve confirmed my assumptions and the speculations I’ve developed in the meantime from reading other threads on this forum and Reddit.