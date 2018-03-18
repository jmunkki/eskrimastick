/*
 *  Blade:
 */

#define	MAX_DIMMER	6

byte bladeDimmer;	//	0 = 255 max, 1 = 127 max, 2 = 63 max, 3 = 31 max
byte hiltDimmer;
int hiltValues[kHiltPixels];	//	12 bit RGB where 0xFFF is white, for example and 0xF00 is red.
byte bladeValues[kBladePixels+1];
int tint = 0x124;

#define swap(value) asm("swap %0" : "=r" (value) : "0" (value)) 

#define	TINTBITS	2

#if TINTBITS == 2
//	Color components are from 0 to 4.
#define COLORSMALL(X,V,C) if(C == 4) X = V << 2; else { X = ((C & 1) ? V : 0); if(C & 2) X += V << 1; }
#define TINTCOLOR	COLORSMALL
#define kRedTint	0x400
#define kGreenTint	0x040
#define kBlueTint	0x004
#define kYellowTint	0x440
#define kCyanTint	0x044
#define kPurpleTint	0x404
#define kWhiteTint	0x444
#define	kGrayTint	0x222
#else
//	This was taking a bit too much program space...
//	Color components are from 0 to 8.
#define COLORCOMP(X,V,C) \
switch(C)\
{ case 0: X = 0; break;\
  case 1: X = V; break;\
  case 2: X = V << 1; break;\
  case 3: X = V + (V << 1); break;\
  case 4: X = V << 2; break;\
  case 5: X = V + (V << 2); break;\
  case 6: X = (V + (V << 1)) << 1; break;\
  case 7: X = (V << 3) - V; break;\
  case 8: X = V << 3; break;\
}
#define TINTCOLOR	COLORCOMP
#define kRedTint	0x800
#define kGreenTint	0x080
#define kBlueTint	0x008
#define kYellowTint	0x880
#define kCyanTint	0x088
#define kPurpleTint	0x808
#define kWhiteTint	0x888
#define	kGrayTint	0x444
#endif
/*
	Color RLE Params
		First tint (int) -> decode into RGB
		First span (byte)
		PROGMEM flag
		Pointer
	
	RLE data:
		Color change:
		FR GB
		Span:
		0x00-0x7F
		
		Ends with 0x00 span or when all pixels are filled
*/

#define	COLORCHANGE(tint)	(tint >> 8) | 0xF0, (tint & 0xFF)

#define	USE_PROGRAM_RLE		0

#if USE_PROGRAM_RLE
boolean	pgmRLE = false;
#define	PROGRAM_RLE(x)	pgmRLE = x;
#define	READ_RLE(v,p) if(pgmRLE) { v = pgm_read_byte_near(p++); } else { v = *p++; }
#else
#define	PROGRAM_RLE(x)
#define	READ_RLE(v,p) v = *p++;
#endif

//	Doesn't seem to actually save any power and takes up ~30 bytes of program space...
#define	POWERSAVER	0

byte	*bladeRLE;
byte	initialRun;
#if POWERSAVER
byte	litBlade;
#endif

#define	SIMPLECOLOR	1

void	updateDisplay()
{
	LIMIT_FREQUENCY(displayLimiter, 40)
	
	byte	i;
#if POWERSAVER
	byte	litNow;
	
	for(i=0;i<kBladePixels;i++)	
	{	litNow = bladeValues[i];
		if(litNow)
		{	break;
		}
	}
#endif	

	byte bitDoubler[16];
	
	for(i=0;i<16;i++)
		bitDoubler[i] = (i | (i << 4)) >> hiltDimmer;

	byte	*bv = bladeValues;
	byte 	rTint = tint >> 8;
	byte 	gTint = tint & 0xF0; swap(gTint);
	byte 	bTint = tint & 0xF;
	byte 	dimmer = bladeDimmer + TINTBITS;
	byte	*rle = bladeRLE;
	byte	run = initialRun;

	if(run > kBladePixels)
		run = kBladePixels;

	byte	pixels = (run < kBladePixels) ? (kBladePixels-run) : 0;
	
#if POWERSAVER
	if(!(litNow | litBlade))
	{	run = 0;
	//	sprintln("Dark");
	}
	else
	{
#if USE_NEOPIXEL_TIME_CORRECTION
		loopStartCorrection += ledCorrection;
#endif
	}
	litNow = litBlade;
#else
#if USE_NEOPIXEL_TIME_CORRECTION
	loopStartCorrection += ledCorrection;
#endif
#endif

	cli();
	//	Status pixels:
	byte *hpx = (byte *)hiltValues;

	#if kHiltPixelMirrors
		hpx += 2*kHiltPixels;
		for(i=0;i<kHiltPixels;i++)
		{
			byte	h = *--hpx;
			byte	l = *--hpx;
			swap(l);
			sendByte(bitDoubler[l & 0xF]);	//	G
			sendByte(bitDoubler[h & 0xF]);	//	R
			swap(l);
			sendByte(bitDoubler[l & 0xF]);	//	B
		}
	#endif

	for(i=0;i<kHiltPixels;i++)
	{
		byte	l = *hpx++;
		byte	h = *hpx++;
		swap(l);
		sendByte(bitDoubler[l & 0xF]);
		sendByte(bitDoubler[h & 0xF]);
		swap(l);
		sendByte(bitDoubler[l & 0xF]);
	}


	do
	{
		while(run--)
		{	int		v = *bv++;
			int		x;
			TINTCOLOR(x, v, gTint); sendByte(x >> dimmer);
			TINTCOLOR(x, v, rTint); sendByte(x >> dimmer);
			TINTCOLOR(x, v, bTint); sendByte(x >> dimmer);
		}
		
		READ_RLE(run,rle);
		if((char)run < 0)
		{	
			rTint = run & 0xF;
			READ_RLE(run,rle);
			gTint = run & 0xF0; swap(gTint);
			bTint = run & 0xF;
			READ_RLE(run,rle);
		}
		
		if(run > pixels)
			run = pixels;

		pixels -= run;
	} while(run);

	sei();
	show();
}

void bladeSetup()
{
	ledSetup(1<<NEOPIXBIT);
	
	for(byte i=0;i<kBladePixels;i++)
		bladeValues[i] = 0;
	
#if USE_NEOPIXEL_TIME_CORRECTION
	show();
	//  Measure full NeoPixel update time with and without interrupts
	//	Disabling interrupts distorts the clock readings, so we adjust using
	//	this measured value. It's still not exact.
	TIME_TYPE a = TIMING_FUNCTION;
	for(byte n = 0; n < (1<<USE_NEOPIXEL_TIME_CORRECTION); n++)
	{	for(byte i=0;i<neoPixels;i++)
			sendPixel(0,0,0);
		show();
	}
	a = TIMING_FUNCTION - a;
	
	TIME_TYPE b = TIMING_FUNCTION;
	for(byte n = 0; n < (1<<USE_NEOPIXEL_TIME_CORRECTION); n++)
	{	cli();
		for(byte i=0;i<neoPixels;i++)
			sendPixel(0,0,0);
		sei();
		show();
	}
	b = TIMING_FUNCTION - b;
	sprintln(a);
	sprintln(b);

	ledCorrection = (a > b) ? a - b : 0; 

	sprint("LED correction: ");
	sprintln(ledCorrection);
#endif
}

