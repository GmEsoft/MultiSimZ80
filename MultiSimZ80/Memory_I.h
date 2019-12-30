#ifndef MEMORY_I_H
#define MEMORY_I_H

// MEMORY API

#include "runtime.h"

class Memory_I
{
public:
	// write char
	virtual uchar write( ushort addr, uchar data ) = 0;

	// read char
	virtual uchar read( ushort addr ) = 0;

	// destructor
	virtual ~Memory_I()
	{
	}
};

#endif
