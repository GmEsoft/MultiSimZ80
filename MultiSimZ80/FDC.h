#ifndef FDC_H
#define FDC_H

#include "InOut_I.h"
#include "Floppies.h"

class FDC :
	public InOut_I
{
public:
	FDC( Floppies &_floppies ) 
		: floppies_( _floppies )
	{
	}

	virtual ~FDC( void )
	{
	}

	// out char
	virtual uchar out( ushort addr, uchar data ) = 0;

	// in char
	virtual uchar in( ushort addr ) = 0;

	virtual void setEnabled( bool enabled ) = 0;

	virtual void reset() = 0;

protected:
	Floppies &floppies_;
};

#endif
