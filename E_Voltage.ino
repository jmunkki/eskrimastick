boolean	reedTriggered;
byte	inputVoltage;

void readVoltMeter()
{
	//	rawV & smoother = 0..4096
	static unsigned int	smoother = 0;
	unsigned int    	rawV = (analogRead(comboAnalog) + analogRead(comboAnalog) + analogRead(comboAnalog) + analogRead(comboAnalog));

//	if(rawV > 20)
	{
		smoother = (smoother * 7 + rawV + 3) >> 3;
		inputVoltage = (((smoother + 15) >> 4) * VOLTAGE_RATIO) >> 8;
		reedTriggered = false;
#if 0		
		sprint(rawV); sprint(" ");
		sprint(smoother); sprint(" ");
		sprintln(inputVoltage);
#endif
	}

//	else
	{
		reedTriggered = digitalRead(tipswitch);
	}
}

