#ifndef CLOCK_I_H
#define CLOCK_I_H

// CLOCK API

class Clock_I
{
public:
	// do CPU cycles
	virtual void runCycles( long cycles ) = 0;

	// Set clock speed (kHz)
	virtual void setClockSpeed( long speed ) = 0;

	// Set RTC rate (kHz)
	virtual void setRtcRate( long rate ) = 0;

	// destructor
	virtual ~Clock_I()
	{
	}
};
#endif
