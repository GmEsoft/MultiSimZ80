#include "Model1Floppies.h"

uchar Model1Floppies::get()
{
	return 0;
}

uchar Model1Floppies::put( uchar data )
{
	switch ( data & 0x0F ) 
	{
        case 1:
            select_ = 0;
            side_ = 0;
            break;
        case 2:
            select_ = 1;
            side_ = 0;
            break;
        case 4:
            select_ = 2;
            side_ = 0;
            break;
        case 8:
            select_ = 3;
            side_ = 0;
            break;
        case 9:
            select_ = 0;
            side_ = 1;
            break;
        case 10:
            select_ = 1;
            side_ = 1;
            break;
        case 12:
            select_ = 2;
            side_ = 1;
            break;
        default:
			break;
    }

	mfm_ = ( data >> 7 ) & 1;		// FM* / MFM: double density select

	return data;
}
