[PicoCalc](https://forum.clockworkpi.com/c/picocalc/31)

I see there’s a keyboard BIOS version 1.4 but I don’t see what the changes are.  
Does anyone know? Is it a good upgrade from 1.2?

It’s add a power off register and fix the missing reset feature (through REG\_ID\_RST) too.  
The power off register implement a delay feature.  
And the power button is considered like a keyboard key press too (value = 0x91).

Needed to update my custom firmware to keep it compatible…

So to answer if it’s good: I think not required, but useful if you want to use the shutdown feature through register and using the power button event like a key press

7 Likes

Oh that’s pretty cool. I was wondering if there was a way to detect a power off request and write state to flash so that it “looks like” a sleep function.

Some programs, like microPython v1.25, are currently not compatible, resulting in a flashing display.

1 Like

[PicoCalc MicroPython Port Status](https://forum.clockworkpi.com/t/picocalc-micropython-port-status/16669/73)

Have you tried this? I could not get this to work, though I am just guessing on what it does and how to use it.

I’ll check on that. Did you use the last 0.7 release?

Not yet and you’re right, I noticed I’m still instant shutdown the pico.  
In the details: the bit 6 and 7 set the sleep or full shutdown request, and the bit 5 to 0 are the delay value in second.  
The 7th bit has priority in case of the both are set.

In any case, I’ll take today and tomorrow to do some testing on the last features and push the corrected code.  
I released it a bit too quickly, given the mistakes I left in it…

I was asking about the “official” 1.4 firmware.

However, for the firmware you are working on, how can we get community support for it? I would love for my apps to take advantage of it. Perhaps, if there is way to detect your firmware and then my apps could “advertise” that users will get additional functionality if they are using this firmware over the “official” one to encourage community adoption?

like normal i2c

send two bytes

first is [0x0e](https://github.com/clockworkpi/PicoCalc/blob/a04995218922fe52c2d33ce5a570718fb170a166/Code/picocalc_keyboard/reg.h#L22)

second is the delay seconds, like 0x07 for 7 seconds

the purpose of this is to coordinate with the Linux board shutdown

when linux shutting down,do a hook to send this i2c shutdown command so that finally we can have all power off

I successfully send that message, using a delay value of, say 10, but I do not know what to do next or what to expect? Nothing appears to happen or I have not seen a change in behaviour.

the Green led on the upper-right corner and the LCD backlight did not turn off??

No, the green LED and the LCD remain on.

I do see and use the difference when reading from this 0x0E register from the previous firmware and 1.4. I read a ‘0’ on previous release and a ‘1’ on 1.4, so this is how I detect if this feature is supported.

From what I read from the official firmware, the delay should be at least 6s (if you put something else, the code set it to 6). And like you said, the return value is always 1.

Normally after that, the keyboard shouldn’t respond during the delay value and should trigger the AXP2101’s driver power off.

In fact, you can use the register @0x00 on 8 bits. The official firmware will be always 0, custom firmware can use any other value. I take **0xCA**.  
You can read the version register, I update it for each version too.

I have it working now. Actually, you need to send `0x8e` followed by the delay in seconds. Bit 7 needs to be set to indicate that a write to the register is occurring, bit 7 clear indicates a read operation.

Good to know  
sorry I forgot the write mask

```
#define WRITE_MASK (1<<7)
```

Hi,

uLisp don’t work either

Just came here to say that the 6s minimum is not really necessary, and should be configurable down to 0s (instant power off) IMO. The linux shutdown handler is no longer in need of an artificial delay, and if it needs one it can ask for one itself. I don’t really think it’s the firmware’s job to force a delay here.

However I’d hope for a more featureful keyboard driver before a new release is warranted. I dont think the above is enough to justify it.

I plan to support the additional features of [@JackCarterSmith](https://forum.clockworkpi.com/u/jackcartersmith) ’s firmware in the [Calculinux](https://github.com/Calculinux) project drivers. registers and features are configurable through the device tree and/or udev, and we can do feature detection as well.

2 Likes

I completely agree. For my use case (rogue) it is somewhat annoying to require my users to wait for 6s for nothing.

In fact, I needed to write witty messages to read whilst we wait. Though I did get positive feedback for those messages!

I’ve installed the new keyboard BIOS 1.4 on my clockwork pi PicoCalc and it doesn’t seem to have improved the keyboard’s performance. Keys still not registering when pressed or multiple characters displayed from one key press. Needs some serious work to sort this out. Keyboard looks fantastic but functionally it’s very poor.

1 Like

My findings with the keyboard on my PicoCalc is that missed keypresses are *much* more likely if you press keys off-center than if you press them square in the middle. This is due to the way the keys physically function, rather than anything with the BIOS.

Once I got used to pressing the keys a certain way I found keypresses to get much more reliable overall, with the exception of pressing some control-key combos where I can’t help but often press the control key off-center.

Mind you this is all with zeptoforth – I have not tried any other software on the PicoCalc, so there *may* be issues with the keyboard BIOS drivers on other applications that I am not aware of.

1 Like