#include "Crc16.h"

void Crc16::update( ushort byte )
{
	byte <<= 8;
	for ( char i=0; i<8; i++ )
	{
		crc_ = ( crc_ << 1 ) ^ ( ( ( crc_ ^ byte ) & 0x8000 ) ? poly_ : 0  );
		byte <<= 1;
	}
}

void Crc16::reset()
{
	crc_ = 0xFFFF;
}

ushort Crc16::get()
{
	return crc_;
}

