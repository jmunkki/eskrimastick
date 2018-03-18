
void setup()
{
  //  Overclock, if enabled:
  CLOCK_INIT
  
//  randomSeed(analogRead(comboAnalog));// ^ (micros()));
#if BREADBOARD
  Serial.begin(115200);
  sprintln("Here we go!");
#endif
  
  rotarySetup();
  pinMode(comboAnalog, INPUT);	//	Voltage / reed
  setPullupInput(tipswitch);
  bladeSetup();
  setupMenus();

}

void loop()
{
#if USE_NEOPIXEL_TIME_CORRECTION
	loopStartTime = TIMING_FUNCTION + (loopStartCorrection >> USE_NEOPIXEL_TIME_CORRECTION);
#else
	loopStartTime = TIMING_FUNCTION;
#endif

	readVoltMeter();
	pollRotary();
	menuControl();
	animateBlade();
	updateDisplayUsingScheme();
	//delay(1);
}
