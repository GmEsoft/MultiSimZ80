#ifndef SLEEPER_I_H
#define SLEEPER_I_H

#include "runtime.h"

class Sleeper_I
{
public:

	Sleeper_I(void)
	{
	}

	virtual ~Sleeper_I(void)
	{
	}

	virtual void sleep( ulong millis ) = 0;

};

#endif
