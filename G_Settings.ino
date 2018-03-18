/*
 *	Settings menus
 */
			
/*
Menu values & Blade color schemes:

	0	Blue
	1	Cyan
	2	Green
	3	Yellow
	4	Red
	5	Purple
	(6)	White
	(7)	Gray
	(8)	Black??
	
	NeoPixels:
	[Area]	[Value]	[Menu]

	If max values is <= 3, double the value to get index color (BLUE/GREEN/RED)
Areas:
	Blade (Green)
√		Brightness		(Yellow)	MENU(mGreen,mYellow,6 VALUES)
√		Primary color	(Green)		MENU(mGreen,mGreen,9 VALUES)
√		Secondary color	(Red)		MENU(mGreen,mRed,9 VALUES)
√		Color scheme	(Purple)	MENU(mGreen,mPurple, 4 VALUES)	Single -> Hilite -> Dual -> Rainbow Animation
√		Animation		(Blue)		MENU(mGreen,mBlue, 2 VALUES)
√		Dual Shift		(Cyan)		MENU(mGreen,mCyan,
	Hilt (Red):
		
	Sheath (Blue):
	
	Extras (Purple)

Button/Rotary
				Button Press: Max Brightness -> 
				Rotate CW:
				Rotate CCW:
Blade:
				Brightness
			    Primary color selector
			    Color Scheme				
			    Primary color length
			    Particle vs. Static
Hilt Display:
				Off
				Battery
				Primary blade color
Sheath:
				Hilt on/off
				Blade on/off

 */

#include <EEPROM.h>

enum
{
	kBlueIndex,
	kCyanIndex,
	kGreenIndex,
	kYellowIndex,
	kRedIndex,
	kPurpleIndex,
	kWhiteIndex
};

const int hiltPalette[] PROGMEM = 
{
	0x00F,	//	0: Blue
	0x0CC,	//	1: Cyan
	0x0F0,	//	2: Green
	0xCC0,	//	3: Yellow
	0xF00,	//	4: Red
	0xC0C,	//	5: Purple
	0xAAA,	//	6: "White"
};

int	colorFromIndex(byte index)
{
	return (pgm_read_word_near(hiltPalette+index) & 0xEEE) >> 1;
}

int darkerColorFromIndex(byte index)
{
	return (pgm_read_word_near(hiltPalette+index) & 0xCCC) >> 2;
}


/*
	top menu color index
	number of menus
	for each menu item:
		menu color index
		default value
		value max (show selection if max is <= 5
		EEPROM offset
*/

const byte redMenus[] PROGMEM = 
{
	kRedIndex, 6, 
		kGreenIndex,	4, 7, kEETint,				//	Done
		kPurpleIndex,	6, 7, kEESecondaryTint,		//	Done
		kCyanIndex,		kBladePixels * 9 / 10, kBladePixels, kEEPrimaryLength,	//	Done
		kBlueIndex,		1, 1, kEEAnimation,			//	Done
		kRedIndex,		0, 2, kEEColorScheme,		
		kYellowIndex,	1, MAX_DIMMER, kEEBrightness,
};

const byte blueMenus[] PROGMEM = 
{
	kBlueIndex, 4, 
		kGreenIndex,	0, 2, kEEHiltDisplayMode,	//	Done
		kBlueIndex,		1, 2, kEESheathMode,		//	Done
		kPurpleIndex,	1, 2, kEEHiltSheathMode,	//	Done
		//kBlueIndex,	0, 1, kEERotaryMode,		//	TODO
		//kCyanIndex,	0, 1, kEEButtonMode,		//	TODO
		kYellowIndex,	1, MAX_DIMMER, kEEHiltBrightness,	//	Done
};

enum {
	kWaiting,
	kTwisting,
	kTopMenu,
	kSubMenu
	};

byte		menuMode = kWaiting;
byte const 	*menuData;
byte		numSubMenus;
byte		menuGroupColorIndex;
char		activeSubMenu;

byte		maxSelection;		//	valid values are 0..maxSelection
byte		selectionTarget;	//	EEPROM index

void readSettingsFromEEPROM()
{
	for(byte i=0;i<kEEPROMUsed;i++)
	{
		gSettings[i] = EEPROM.read(i);
	}
}

void writeSettingsToEEPROM()
{
	menuMode = kWaiting;

	for(byte i=0;i<kEEPROMUsed;i++)
	{
		byte v = gSettings[i];
		if(v !=  EEPROM.read(i))
		{
			sprint("EEPROM ");
			sprint(i);
			sprint(" = ");
			sprintln(v);			
			EEPROM.write(i, v);
		}
	}
}

void setupDefaults(byte const *menuData)
{
	byte	n = pgm_read_byte_near(menuData+1);
	menuData += 2;
	while(n--)
	{	gSettings[pgm_read_byte_near(menuData+3)] = pgm_read_byte_near(menuData+1);
		menuData += 4;
	}
}

byte	getGroupColorIndex()
{
	return pgm_read_byte_near(menuData);
}

byte	getMenuColorIndex()
{
	return pgm_read_byte_near(menuData + 2 + (activeSubMenu << 2));
}

byte	getMenuItemColorIndex()
{
	byte menuSelection = gSettings[selectionTarget];
	
	if(maxSelection > kWhiteIndex)
		return kWhiteIndex;
	else if(maxSelection == 1)
		return 3 * menuSelection;
	else if(maxSelection <= 3)
		return menuSelection << 1;
	
	return menuSelection;
}

void	activateSubMenu()
{
	byte const *menuInfo = menuData + 2 + (activeSubMenu << 2);

	maxSelection = pgm_read_byte_near(menuInfo + 2);
	selectionTarget = pgm_read_byte_near(menuInfo + 3);
	rotationPosition = 0;
}

void	openTopMenu(byte const *aMenu)
{
	menuMode = kTopMenu;
	menuData = aMenu;
	activeSubMenu = 0;
	numSubMenus = pgm_read_byte_near(aMenu + 1); 
	activateSubMenu();
}

void	setupMenus()
{
	readSettingsFromEEPROM();

	if(RESET_EEPROM ||
		(gSettings[kEEVersion0] != VERSION_SIGNATURE_0
		|| gSettings[kEEVersion1] != VERSION_SIGNATURE_1))
	{
		sprintln("EEPROM Reset.");
		gSettings[kEEVersion0] = VERSION_SIGNATURE_0;
		gSettings[kEEVersion1] = VERSION_SIGNATURE_1;
		setupDefaults(redMenus);
		setupDefaults(blueMenus);
		writeSettingsToEEPROM();
	}
}

#define	LONG_PRESS_TIME		300

void	menuControl()
{
	switch(menuMode)
	{	case kWaiting:
			if(rbuttonDown)
			{	menuMode = kTwisting;
				rotationPosition = 0;
				sprintln("Start twisting");
			}
			break;
		case kTwisting:
			if(rbuttonUp)
			{	if(rotationPosition)
				{	if(rotationPosition < 0)
					{	sprintln("Counter Twist!");
						menuGroupColorIndex = kRedIndex;
						openTopMenu(blueMenus);
					}
					else
					{	sprintln("Clockwise Twist!");
						menuGroupColorIndex = kGreenIndex;
						openTopMenu(redMenus);
					}
				}
				else
				{	menuMode = kWaiting;
					sprintln("abort");
				}
			}
			break;		
		case kTopMenu:
			if(rbuttonState)//	Only detect rotation when the button isn't down!
			{	rotationPosition = 0;
			}
			else if(rotationPosition)
			{	
				activeSubMenu += rotationPosition;
				while(activeSubMenu < 0)
					activeSubMenu += numSubMenus;
				while(activeSubMenu >= numSubMenus)
					activeSubMenu -= numSubMenus;
				activateSubMenu();			
	
				sprint("Menu ");
				sprintln((byte)activeSubMenu);
			}
			
			if(rbuttonUp)
			{	
				if(rbuttonPressTime >= LONG_PRESS_TIME)
				{
					sprintln(rbuttonPressTime);
					writeSettingsToEEPROM();
				}
				else
				{
					menuMode = kSubMenu;
					sprint("MenuSelect ");
					sprintln((byte)activeSubMenu);
				}
			}
			break;
		case kSubMenu:
			if(rbuttonState)//	Only detect rotation when the button isn't down!
			{	rotationPosition = 0;
			}
			else if(rotationPosition)
			{	
				int menuSelection = gSettings[selectionTarget] + rotationPosition;
				
				while(menuSelection < 0)
					menuSelection += maxSelection + 1;
				while(menuSelection > maxSelection)
					menuSelection -= maxSelection + 1;
	
				gSettings[selectionTarget] = menuSelection;
				rotationPosition = 0;
				
				sprint("Item ");
				sprintln(menuSelection);
			}
			
			if(rbuttonUp)
			{	
				if(rbuttonPressTime >= LONG_PRESS_TIME)
				{
					sprintln(rbuttonPressTime);
					writeSettingsToEEPROM();
				}
				else
				{
					menuMode = kTopMenu;
					sprint("ItemSelect ");
					sprintln(gSettings[selectionTarget]);
				}
			}
			break;			
	}

	//	Update display:	
	switch(menuMode)
	{	case kWaiting:
			readVoltMeter();
			showIdleHilt();
			break;
		case kTwisting:
			{	int		dkBlue = darkerColorFromIndex(kBlueIndex);			
				hiltValues[0] = dkBlue;
				hiltValues[1] = dkBlue;
				hiltValues[2] = dkBlue;
			}
			break;
			
		case kTopMenu:
			hiltValues[0] = colorFromIndex(getMenuItemColorIndex());
			hiltValues[1] = blinking(colorFromIndex(getMenuColorIndex()));
			hiltValues[2] = colorFromIndex(getGroupColorIndex());
			break;
		case kSubMenu:
			hiltValues[0] = blinking(colorFromIndex(getMenuItemColorIndex()));
			hiltValues[1] = colorFromIndex(getMenuColorIndex());
			hiltValues[2] = colorFromIndex(getGroupColorIndex());
			break;
	}
	
	clearRotaryEvents();
}