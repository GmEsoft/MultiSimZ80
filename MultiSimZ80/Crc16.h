#ifndef CRC16_H
#define CRC16_H

#include "runtime.h"

class Crc16
{
public:
	Crc16( ushort poly ) : poly_( poly )
	{
	}

	~Crc16()
	{
	}

	void update( ushort byte );

	void reset();

	ushort get();

private:
	ushort poly_;
	ushort crc_;
};

#endif
	