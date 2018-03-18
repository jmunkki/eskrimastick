#define	JACKPOTLIMIT			64
#define	NORMAL_TARGETLEVEL		255
#define	DIM_TARGETLEVEL			64
#define	SHEATHED_TARGETLEVEL	0
#define	MINIMUM_DEFICIT			31

int	targetLevel = NORMAL_TARGETLEVEL;

void particleBlade()
{
	bladeValues[kBladePixels] = 255;
	
	int		total = 0;
	for(byte i=0;i<kBladePixels;i++)
		total += bladeValues[i];

	//int		avg = total / kBladePixels;
	int		target = kBladePixels * targetLevel;
	int		delta = (target - total) / 2;
	
	while(delta > MINIMUM_DEFICIT)
	{
		int		energy = random(1, delta);
		delta -= energy;

		if(energy > 1)
		{	int		a = random(energy);
			energy  = random(energy);		
			if(a < energy)
				energy = a;
	
			if(energy > JACKPOTLIMIT)
				energy = JACKPOTLIMIT;
			
			if(energy)
			{	int		t = random(kBladePixels);
				int		pv = bladeValues[t] + energy;
				
				if(pv > 255)
					pv = 255;
			
				bladeValues[t] = pv;
				#ifdef POLL_FN
					POLL_FN();
				#endif
			}
		}
	}
	
	#define DECAY(x)	((x > 7) ? (1+(x >> 4)) : 0)
	byte	*p = bladeValues;
	int		prev = DECAY(p[kBladePixels-1]);
	int		current = DECAY(p[0]);
	int		next = DECAY(p[1]);
	int		pixel;

	bladeValues[kBladePixels] = *p;

	for(byte i=0;i<kBladePixels;i++)
	{	
		//	New pixel is current value + 25% previous decay - 62.5% current decay + 25% next decay - 1
		pixel = p[0]
				+ (prev >> 2)
				- ((current + (current << 2) + 2) >> 3)
				+ (next >> 2)
				- 1;
		if(pixel > 255)
			pixel = 255;
		else if(pixel < 0)
			pixel = 0;

		total += pixel;
		*p++ = pixel;
			
		prev = current;
		current = next;
		next = DECAY(p[1]);
		
		#ifdef POLL_FN
		if(!(i & 0x1F))
			POLL_FN();
		#endif
	}
}

void	animateBlade()
{
	LIMIT_FREQUENCY(displayLimiter, 20)

	if(gSettings[kEEAnimation])
	{	particleBlade();
	}
	else
	{
		for(byte i=0;i<kBladePixels;i++)
		{	byte	v = bladeValues[i];
			if(v != targetLevel)
			{	if(v < targetLevel)
					v += 1 + ((targetLevel - v) >> 5);
				else
					v -= 1 + ((v - targetLevel) >> 5);
				bladeValues[i] = v;
			}
		}		
	}
}

const int		tints[] PROGMEM = 
	{
		kGreenTint, kWhiteTint, kGrayTint, kCyanTint, 
		kBlueTint, kPurpleTint, kRedTint, kYellowTint
	};

#define	BLADETINT(x) pgm_read_word(tints+(x))

const int		hiltTints[] PROGMEM =
{
	0x070, 0x777, 0x444, 0x077, 
	0x007, 0x707, 0x700, 0x770
};
#define	HILTTINT(x) pgm_read_word(hiltTints+(x))

void	showBatteryLevel()
{
	const int green = 0x080;
	const int yellow = 0x880;
	const int red = 0x800;
	const int blue = 0x084;
	byte	volts = inputVoltage;

	if(volts >= kFullVoltage)
	{	hiltValues[0] = blue; hiltValues[1] = blue; hiltValues[2] = blue;
	}
	else
	{
		hiltValues[0] = volts >= (kFullVoltage - 7) ? ((volts >= (kFullVoltage - 6)) ? green : yellow) : red;
		hiltValues[1] = volts >= (kFullVoltage - 5) ? ((volts >= (kFullVoltage - 4)) ? green : yellow) : 0;
		hiltValues[2] = volts >= (kFullVoltage - 3) ? ((volts >= (kFullVoltage - 2)) ? green : yellow) : 0;
		//hiltValues[0] = volts >= 35 ? ((volts >= 36) ? green : yellow) : red;
		//hiltValues[1] = volts >= 37 ? ((volts >= 38) ? green : yellow) : 0;
		//hiltValues[2] = volts >= 39 ? ((volts >= 40) ? green : yellow) : 0;
	}
}

void	showIdleHilt()
{
	byte	hiltMode = gSettings[kEEHiltDisplayMode];
	
	if(reedTriggered)
	{
		byte	sheathMode = gSettings[kEEHiltSheathMode];
		
		if(sheathMode != 1)
		{	if(sheathMode == 0)
			{	hiltMode = 0;
			}
			else
			{	hiltMode = 2;	//	Show battery level
			}
		}
	}
	
	if(!firstPress || hiltMode == 2)
	{	showBatteryLevel();
	}
	else
	{	int	rgb;

		if(hiltMode == 0)
		{	rgb = 0;
		}
		else //if(hiltMode == 1)
		{	rgb = HILTTINT(gSettings[kEETint]);
		}
		hiltValues[0] = rgb;	
		hiltValues[1] = rgb;
		hiltValues[2] = rgb;
	}
}

#define	PEAKING_CHANGES		48
#define	PEAKING_TRESHOLD	220

void	bladePeakingMode()
{
	byte	colorChangeRLE[(PEAKING_CHANGES+1)*3];
	int		colors[] = { 0xF000 | BLADETINT(gSettings[kEETint]),
						 0xF000 | BLADETINT(gSettings[kEESecondaryTint]) };
	char	space = PEAKING_CHANGES;
	byte *	p = colorChangeRLE;
	byte	showing;
	byte	firstColor;
	byte	count = 0;
	
	firstColor = bladeValues[0] >= PEAKING_TRESHOLD;
	showing = firstColor;
	tint = BLADETINT(gSettings[showing ? kEESecondaryTint : kEETint]);
	initialRun = 0;
	count = 0;
	for(byte i=0;i<kBladePixels;i++)
	{	if((bladeValues[i] >= PEAKING_TRESHOLD) != showing)
		{	if(initialRun)
			{	if(--space > 0 || showing != firstColor)
				{
					*p++ = ((byte *)(colors+showing))[1];
					*p++ = ((byte *)(colors+showing))[0];
					*p++ = count;
				}
				else
				{	count = kBladePixels;
					break;
				}
			}
			else
			{	initialRun = count;
			}
			showing ^= 1;
			count = 0;
			space--;
		}
		count++;
	}

	*p++ = ((byte *)(colors))[1];
	*p++ = ((byte *)(colors))[0];
	*p = kBladePixels;

	bladeRLE = colorChangeRLE;
	updateDisplay();
}

/*
const byte	singleBlade[] PROGMEM = {
	kBladePixels,							//	Default color
};
*/

void	updateDisplayUsingScheme()
{
	targetLevel = NORMAL_TARGETLEVEL;
	if(reedTriggered)
	{
		byte	sheathMode = gSettings[kEESheathMode];
		
		if(sheathMode == 0)
			targetLevel = SHEATHED_TARGETLEVEL;
		else if(sheathMode == 1)
			targetLevel = DIM_TARGETLEVEL;
	}

	tint = BLADETINT(gSettings[kEETint]);
	byte	simpleScheme[] = {
	//	COLORCHANGE(BLADETINT(gSettings[kEESecondaryTint])), 2,
	//	COLORCHANGE(BLADETINT(gSettings[kEETint])), 2,
		COLORCHANGE(BLADETINT(gSettings[kEESecondaryTint])), kBladePixels
	};
	PROGRAM_RLE(false);
	bladeRLE = simpleScheme;

	bladeDimmer = MAX_DIMMER-gSettings[kEEBrightness];
	hiltDimmer = MAX_DIMMER-gSettings[kEEHiltBrightness];

	switch(gSettings[kEEColorScheme])
	{
		default:
		case 0:		//	Single color scheme
			initialRun = kBladePixels;
			updateDisplay();
			break;
		case 1:		//	Dual color scheme
			initialRun = gSettings[kEEPrimaryLength];
			updateDisplay();
			break;
		case 2:
			bladePeakingMode();
			break;
	}
}