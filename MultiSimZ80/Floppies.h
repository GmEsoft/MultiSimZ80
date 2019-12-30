#ifndef FLOPPIES_H
#define FLOPPIES_H

#include "Latch_I.h"
#include "DskImage.h"
#include "CPU.h"

class Floppies :
	public Latch_I
{
public:
	Floppies( CPU &_cpu
		)
		: cpu_( _cpu )
	{
	}

	virtual ~Floppies(void)
	{
	}

	// generate interrupt
	virtual void irq() = 0;

	// seek sector
	virtual uchar seek( uchar track, uchar sector, bool trackOnly ) = 0;

	// is disk image mounted ?
	virtual bool isDisk() = 0;

	// get byte counter
	virtual int &getCount() = 0;

	// get sector flags
	virtual uchar &getFlags() = 0;

	// get track counter
	virtual uchar &getTrack() = 0;

	// get byte and update crc
	virtual uchar getByte() = 0;

	// put byte and update crc
	virtual void putByte( uchar byte ) = 0;

	// get DAM byte 
	virtual uchar getDamByte( ushort offset ) = 0;

	// get crc
	virtual ushort getCrc() = 0;

	// get format
	virtual uchar getFormat() = 0;

	// get Image
	virtual DskImage &getImage( int unit ) = 0;

	// set Image
	virtual void setImage( int unit, DskImage *pDskImage ) = 0;

	// reset
	virtual void reset() = 0;

	// and char
	virtual uchar and_( uchar data )
	{
		// not supported
		return *(char*)0=0;
	}

	// or char
	virtual uchar or_( uchar data )
	{
		// not supported
		return *(char*)0=0;
	}

	// xor char
	virtual uchar xor_( uchar data )
	{
		// not supported
		return *(char*)0=0;
	}

	// not
	virtual uchar not_()
	{
		// not supported
		return *(char*)0=0;
	}

protected:
	CPU &cpu_;
};

#endif
