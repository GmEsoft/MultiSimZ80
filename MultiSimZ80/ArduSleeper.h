#ifndef ARDUSLEEPER_H
#define ARDUSLEEPER_H

#include "Sleeper_I.h"

#include <arduino.h>

class ArduSleeper :
	public Sleeper_I
{
public:

	ArduSleeper(void)
	{
	}

	virtual ~ArduSleeper(void)
	{
	}

	void sleep( ulong millis )
	{
		delay( millis );
	}
};

#endif

