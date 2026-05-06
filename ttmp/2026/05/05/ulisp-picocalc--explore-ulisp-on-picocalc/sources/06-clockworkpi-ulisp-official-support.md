[PicoCalc](https://forum.clockworkpi.com/c/picocalc/31)

There’s a page for it [here](http://www.ulisp.com/show?56ZO).

6 Likes

[Pico 2 W compatible with Pico calc?](https://forum.clockworkpi.com/t/pico-2-w-compatible-with-pico-calc/16351/40)

[Foobler notes - 1 - "Getting Started"](https://forum.clockworkpi.com/t/fooblers-getting-started-notes/17940/21)

So when will Emacs be ported to uLisp and integrated into the PicoCalc uf2?  

2 Likes

So it’s time to become “Lost In Stupid Parentheses”

2 Likes

Only when you’re ready to move on from the BASICs…

I’m ready to move, let’s go!

2 Likes

It’s about 35 years since I wrote in LISP, or rather ELisp.  
I was writing extensions for Emacs.  
Maybe it’s time to take it up again… currently waiting for the delivery of the PicoCalc…

1 Like

I helped out a little with this, but my screen is currently broken. Once the replacement shows up, I still have a laundry list of stuff to work on.

Hummmm… Strange… I copied the.uf2 firmware file on the disk but once copied nothing happens, the disk is not ejected and the firmware is still the Picomite mmbasic. I tried again, the ulisp firmware file is no more on the disk and i get the same result.  
Do i need to “erase” the old firmware before? (i already flashed the picocalc with the last mmbasic V6.00.02RC7)

This might happen if the firmware is made for Pico2 and you’re flashing on Pico, or vice versa. I made that mistake once when I had forgotten which module I had installed. Thankfully, the Pico (and Pico2) just ignore firmware files that are invalid.

2 Likes

Is there a way to make “universal” uf2 that support both pico and pico2?

I don’t think that’s possible since the hardware is different.

There’s some interesting [work being done to support booting firmware from files on the SD using a bootloader](https://forum.clockworkpi.com/t/i-made-an-app-that-dynamically-load-firmware-from-sd-card/16664) (and not having to flash each firmware). But even that requires using firmware designed for the device. Last I checked it only supported the Pico, but it might support both it and the Pico2 now. It can’t load the usual firmware files though, and everything has to be custom built for it. Though it sounded like there might be a way for it to load the actual firmware files too, once they figure that out.

I have a pico2w, i suspect it’s a firmware only for a pico… [@picolisper](https://forum.clockworkpi.com/u/picolisper) is it?

1 Like

These may not be the most up to date (I haven’t been following along), but there’s a link here to a page with downloads for the Pico and Pico2 running at different speeds:

Just tried the pico2W@150Mhz, flashing is ok but i got a message error “Error: Unable to autorun from SD card”…

Edit: there are problems with some keys, arrows, backspace,…

I will wait for the official pico 2w release

1 Like

Yes, 2w release is key. Let’s hope it comes soon

1 Like

I can do one from the official image but I can’t test it — my picocalc’s screen is broken right now.

If you’re running from the images I posted previously, the message is normal (autorun is on, but you haven’t saved an autorun image yet). It was also a development version (4.7a, instead of 4.7c).

Yes, that’s why it’s better to wait for the official image.

Sweet! This is my lunchtime learning pastime - Lisp is a real test of my brain. Reading the official page I’m amazed to discover it works with the serial port when connected to a computer. I had no idea! This will speed things up (and make it easier to save Lisp code).

1 Like

You can connect via the serial port until your screen is replaced.

1 Like