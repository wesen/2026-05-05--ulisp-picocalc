[PicoCalc](https://forum.clockworkpi.com/c/picocalc/31)

I posted, then deleted, a topic earlier as I was struggling to get an app up and running.  
I’m posting this topic just to give some clues to the next person..

There are several Github projects out there claiming to be PicoCalc bootloaders.  
My mistake was to try to use the linker file from one of those.

So ClockworkPi/PicoCalc repository has the bootloader that is actually shipped on the device. Using that bootloader’s linker files solved the problem.

I was able to get picocalc-text-starter up and running, and I am going to use pieces of that to get my app up.

4 Likes

Please tel me more, I want a bootloader for my Pico2W to load different taste of booting.

Can you make a quick tutorial?

Thx

1 Like

There’s no need to mess about with custom linker scripts and.BIN files anymore. uf2loader supercedes all the existing bootloader implementations (multibooter, sdboot, etc) and supports the same uf2 files you would have flashed if there was no bootloader.

nice but apparently the bootloader its shipped with doesnt just add uf2 files to the boot menu, only bin files, as the kit comes with no clue about any documentation for the bootloader, i had to do guess work. and the readme on the bootloader directory in the picocalc repository only talks about link files for.bins

can you direct me to documentation for getting uf2s to boot?

i followed the readme for the multi boot loader, on the picocalc gitub repository.

but lets wait and see if someone answers about getting uf2 files to boot since thats preferable

Have a look at the uf2loader release thread for the links:

Installation is similar to the older bootloaders, but there’s an extra file that needs to be put on the SD card which contains the UI. (flash bootloader\_pico.uf2 to the board, put BOOT2040.uf2 on the sdcard, make a pico1-apps folder on the card and put your uf2’s in there.) It also supports the Pico 2 if you decide to swap to that.

1 Like