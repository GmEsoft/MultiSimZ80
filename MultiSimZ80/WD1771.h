#ifndef WD1771_H
#define WD1771_H

#include "FDC.h"
#include "Floppies.h"

class WD1771 :
	public FDC
{
public:
	WD1771( Floppies &_floppies ) 
		: FDC( _floppies), lastStatus_( 0 ), status_( 0xFF )
		, cmd_( 0 ), data_( 0 ), track_( 0 ), sector_( 0 )
	{
	}

	virtual ~WD1771( void )
	{
	}


	// out char
	virtual uchar out( ushort addr, uchar data );

	// in char
	virtual uchar in( ushort addr );

	virtual void setEnabled( bool enabled )
	{
		status_ = enabled ? 0x80 : 0xFF;
	}

	virtual void reset();

private:
	uchar cmd_;			// Virtual FDC Command register
	uchar status_;		// Virtual FDC Status register
	uchar track_;		// Virtual FDC Track register
	uchar sector_;		// Virtual FDC Sector register
	uchar data_;		// Virtual FDC Data register
	uchar lastStatus_;	// Last FDC Status

};

#endif
