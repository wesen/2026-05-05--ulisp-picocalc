My PicoCalc displays the following menu when I turn it on:

```
PicoCalc Bootloader v1.0

Path: SD/firmware

[Default App]
Lua_180a58e.bin             353KB
MicroPython_fa8b24c.bin     360KB
MP3player_v0.5.bin          202KB
PicoCalc_NES_v1.0.bin       169KB
PicoMite_cbf6d71.bin        766KB
uLisp_4.8f.bin              186KB

---------------------------------
Up/Down to select, Enter to exec.
```

This is how it arrived from Hong Kong and I’ve not changed anything.

How do I either

1/ Just update the bootloader on its own and preserve the original files?

2/ Update the bootloader AND update the files to the latest versions?

I’m assuming that these.bin files are deprecated and are stored on the SD card thats plugged into the PicoCalc? Are there any tutorials or links that I can follow or can anyone guide me on what steps to follow? I’m also planning on fitting a Pimoroni Pico Plus 2W when I’ve soldered on the pins. Thanks.

Follow the instructions here to update the bootloader: [GitHub - pelrun/uf2loader: Bootloader that loads firmware from PicoCalc's SD card Slot. · GitHub](https://github.com/pelrun/uf2loader)

The new bootloader uses the regular uf2 file format and will not read.bin files.

I assume the idea of the latest official clockwork pi release is that you delete your entire contents and replace it with what is in their release.

Be aware the clockworkpi versions of things are often a very long way behind what is actually available from development projects.

Personally I just downloaded individual applications and put them in the relevant apps folder as described in the uf2loader documentation.

It’s entirely up to you. The presence of the bin files will harm nothing. Equally not adding extra files just means you cannot run those applications.

As to specifically how:

For installing the bootloader, it works exactly the same as flashing any other UF2 file: [Firmware Installation | GP2040-CE](https://gp2040-ce.info/installation/)

For all other files, just put your SD card in any card reader on a computer and copy files as normal