[PicoCalc](https://forum.clockworkpi.com/c/picocalc/31)

I just got my PicoCalc and I see that the PicoMite version is V6.00.02.  
I have noticed that there is a new version but it comes as UF2 and the bootloader on the SD card needs BIN file.  
Does anyone has this file?

Thank you,

Ehood

no, it’s better to replace the bootloader with the uf2 loader by pelrun or just flash the uf2 directly. The bin files are a dead end because they require a lot of extra work to create.

Thank you.

I understand that there are some problems with the UF2 loader. Some UF2 files are not usable, am I right or did I miss something?

Will I be able to use the bootloader v1.0 in parallel?

Ehood

the files that are incompatible with uf2 loader are the ones which use the pico flash in a way that they shouldn’t in the first place, i.e. picomite has (from what i understand, but i bet pelrun can explain further/correct me) hardcoded offsets for dealing with the pico flash for its own storage, which messes with how partitioning in the rp2350 works and how uf2 loader uses it to stay resident while flashing a firmware to another partition

picomite at least has already been patched by pelrun:

the closest you might be able to have both loaders is (this is speculation, i haven’t tried) maybe loading the original uf2 from the uf2 loader, but you probably shouldn’t. the bin loader was made for clockworkpi’s forks of firmwares that aren’t going to be maintained moving forward anyway

3 Likes

OK, got it.

Thank you for the answer.

Ehood