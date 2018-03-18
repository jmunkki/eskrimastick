/*
Notes:
  Hardware has two rows of 3 Pixels in the hilt, plan is to mirror these, so there are only 3 pixels to set.

  Pixels should be settable in RGB with bright/dim and potentially a master brightness. Master brightness
  could simply be a shift right op to limit the max value to 255 -> 127 -> 63 -> 31 -> 15 -> 7

  Color could be 8 bit with R3/G3/B2 like with the Neobites2 palettes. Should consider a gamma table similar
  to NeoBites. Use preprocessing to avoid running too long in the NeoPixel loop.

  The blade has 92 pixels. Plan is to store 8 bits per pixel and use the pixels values in various ways.
  Possible ideas:

  - Direct single color with a tint setting of some kind. Need to work out how to do the tinting fast without
  using a lookup table.

  - Duocolor with mix of two N bit colors. Full 4 bit by 24 bit RGB lookup table would take too much space.
    Could implement a 4 bit multiply using shifts and additions...

  

*/

#include <avr/pgmspace.h>

#define	RESET_EEPROM	0

// Arbitrary version numbers to check if EEPROM is either not configured yet or out of date:
#define	VERSION_SIGNATURE_0	0xBA
#define	VERSION_SIGNATURE_1	0x55

enum
{
	kEEVersion0,
	kEEVersion1,
	//	Blade:	
	kEEBrightness,
	kEETint,
	kEEAnimation,
	kEEColorScheme,
	kEESecondaryTint,
	kEEPrimaryLength,
	
	//	Hilt:
	kEEHiltDisplayMode,	//	Off, BladeColor, Battery
	kEEHiltBrightness,	
	kEESheathMode,		//	Off, Dim, On
	kEEHiltSheathMode,	//	Off, On, Pulse (do pulse later)
	
	kEEButtonMode,		//	Max brightness, etc - do later -
	kEERotaryMode,		//	Hilt display, adjust brightness, adjust color? - do later -
	
	kEEPROMUsed
};

byte	gSettings[kEEPROMUsed];

/*
**	Hardware configuration:
*/

#define	USE_NEOPIXEL_TIME_CORRECTION		0

#define BREADBOARD 0

#define CLOCK_INIT

//	These are now bit numbers into PORTB:
#define NEOPIXBIT    0 /* This is equivalent to digital 8 */

#define	FLIPBUTTON(x)	(~(x))
#define rotarybutton  1
#define rotaryccw   2
#define rotarycw    3

//	These are pin numbers:
#define tipswitch	12
#define debugLED    13
#define comboAnalog   A0

#define	ROTARY_PIN		PINB
#define	ROTARY_DDR		DDRB
#define	ROTARY_PORT		PORTB
#define	CCW_MASK		(1 << rotaryccw)
#define	CW_MASK			(1 << rotarycw)
#define	BUTTON_MASK		(1 << rotarybutton)
#define	ROTARY_MASK		(CCW_MASK | CW_MASK | BUTTON_MASK)

#define PIXEL_PORT  	PORTB  // Port of the pin the pixels are connected to
#define PIXEL_DDR   	DDRB   // Port of the pin the pixels are connected to
#define	POLL_FN			pollRotary


//  Voltage Reference is 3.3V
#define kRefVoltage   33
#define kFullVoltage  41

#if BREADBOARD
#define sprint(v)   Serial.print(v)
#define sprintln(v)   Serial.println(v)

#define kHiltPixels       3
#define kHiltPixelMirrors 0
#define kBladePixels    92

#else
#define sprint(v) 
#define sprintln(v)

#define kHiltPixels       3
#define kHiltPixelMirrors 1
#define kBladePixels     92
//#define kBladePixels      20
#endif

//  Ratio matters, so for instance 47k & 470k would be 1 and 10 and two 47kΩ resistors are 1 and 1:
#define vmResistor1 1
#define vmResistor2 1
#define	VOLTAGE_RATIO (kRefVoltage * (vmResistor1 + vmResistor2) / vmResistor1)

const int neoPixels = kBladePixels + kHiltPixels * (1+kHiltPixelMirrors);


/*
 * Utility for setting up pins:
 */
byte setPullupInput(int pin)
{
  pinMode(pin, INPUT_PULLUP);

  return digitalRead(pin);
}
