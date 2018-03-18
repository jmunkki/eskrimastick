//  Set to 0x3, 0x7 or 0xF or even higher if you need more debouncing.
//  The voting takes care of a lot of debouncing as well, so I found
//  the smaller & faster version using 0x1 works just fine.
#define kRotaryVoteLimit  0
#define kButtonDebounce   0xF

byte	oldRotary;

byte 	rccwState = 0;
byte 	rcwState = 0;
char 	voteDirection = 0;
int  	rotationPosition = 0;//kBladePixels / 2;

byte 	rbuttonState;
boolean rbuttonDown = false;
boolean	rbuttonUp = false;
boolean	firstPress = false;
unsigned int	rbuttonPressTime;
TIME_TYPE	rbuttonDownTime;

void clearRotaryEvents()
{
  rbuttonDown = false;
  rbuttonUp = false;
}

void rotarySetup()
{
  DDRB &= ~ROTARY_MASK;	//	Configure as input
  ROTARY_PORT |= ROTARY_MASK;		//	Internal pullup
  delay(1);
  oldRotary = ROTARY_PIN & (ROTARY_MASK);
  rbuttonState = oldRotary & BUTTON_MASK;
  clearRotaryEvents();
}

#define	READMASK(value, mask)	(0 != (mask & value))

void doRotary(byte regValue)
{
	byte rccwNew = READMASK(regValue, CCW_MASK);
	byte rcwNew = READMASK(regValue, CW_MASK);
	
	if(rccwNew != rccwState)
	{
	  if(rcwNew != rccwNew)
		voteDirection++;
	  else
		voteDirection--;
	}
	
	if(rcwNew != rcwState)
	{
	  if(rcwNew  != rccwNew)
		voteDirection--;
	  else
		voteDirection++;
	}
	
	rcwState = rcwNew;
	rccwState = rccwNew;

	if(voteDirection && rccwState == 1 && rcwState == 1)
	{
		if(voteDirection < -kRotaryVoteLimit)
			rotationPosition--;
		else if(voteDirection > kRotaryVoteLimit)
			rotationPosition++;

		voteDirection = 0;
	}

	byte buttonNew = FLIPBUTTON(regValue) & BUTTON_MASK;
	if((buttonNew ^ rbuttonState) == BUTTON_MASK)
	{
		rbuttonState = buttonNew;
		if(rbuttonState)
		{
			rbuttonDown = true;
			rbuttonDownTime = loopStartTime;
		}
		else
		{
			rbuttonUp = true;
			firstPress = true;
			rbuttonPressTime = (loopStartTime - rbuttonDownTime) >> QUICK_MILLIS;
		}
	}
}

inline void pollRotary()
{
	byte	newRotary = ROTARY_PIN & ROTARY_MASK;
	if(newRotary != oldRotary)
	{
		oldRotary = newRotary;
		doRotary(newRotary);
	}
}