# eskrimastick
Arduino code for https://www.myminifactory.com/object/3d-print-black-widow-infinity-war-eskrima-stick-53846

Most of this code was written for my Kryptonite Sword project and then just quickly adapted for the Eskrima stick. I'm planning on writing all new code for the stick. At the moment, the animations are relatively passive and you just use the menus to set up colors, brightness etc. This was fine for the Kryptonite sword, but the Eskrima stick needs something more dynamic.

The sword used an Adafruit Trinket, which only has 512 bytes of RAM and 5.5kB of program memory available. The Arduino Pro Mini that I used on the Eskrima stick has 2kB RAM and 32kB program memory, so I will be able to add a lot of new stuff and also eliminate some of the limitations and dirty tricks that were used in this first release.

However, the code works on the hardware, so if you have built the circuit, you should be able to use this to give it a try.

NOTE:

	To use the menus, hold down the button and twist either clockwise (main menus) or
	counterclockwise (alternate menus). Six Neopixels are used to show menu status
	(or battery charge level), but it's really just 3 pixels that are mirrored.
	The first one indicates main/alt menu, the second one is the submenu and the
	third one is the current value of a setting (if the setting has too many values,
	it just shows white and doesn't change). Click to adjust (note that the blinking
	will move to the next level) and click again to confirm. A long click will
	exit the menus.