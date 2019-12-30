#ifndef MODEL1_INOUT_H
#define MODEL1_INOUT_H

#include "InOut_I.h"

class Model1InOut :
	public InOut_I
{
public:
	Model1InOut()
		: port254_( 0 ), port255_( 0 )
	{
	}

	~Model1InOut()
	{
	}

	// out char
	virtual uchar out( ushort addr, uchar data );

	// in char
	virtual uchar in( ushort addr );

	// audio I/O, cassette motor and wide chars
	void setPort255( InOut_I *_port )
	{
		port255_ = _port;
	}

	// CPU speed control (1.5 times faster)
	void setOut254( InOut_I *_port )
	{
		port254_ = _port;
	}

private:
	InOut_I *port255_;
	InOut_I *port254_;
};

#endif
