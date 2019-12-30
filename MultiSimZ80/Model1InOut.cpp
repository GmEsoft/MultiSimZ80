#include "Model1InOut.h"

uchar Model1InOut::in(ushort addr)
{
	if ( addr == 255 )
		return port255_ ? port255_->in( 0 ) : 0xFF;
	else if ( addr == 254 )
		return port254_ ? port254_->in( 0 ) : 0xFF;
	else
		return 0xFF;
}

uchar Model1InOut::out(ushort addr, uchar data)
{
	if ( addr == 255 )
		return port255_ ? port255_->out( 0, data ) : data;
	else if ( addr == 254 )
		return port254_ ? port254_->out( 0, data ) : data;
	else
		return data;
}
