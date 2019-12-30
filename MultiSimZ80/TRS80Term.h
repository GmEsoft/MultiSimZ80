#ifndef TRS80TERM_H
#define TRS80TERM_H

#include "TermProxy.h"

class TRS80Term :
	public TermProxy
{
public:
	TRS80Term(void) : shiftTab_( 0 ), shiftBack_( 0 )
	{
	}

	virtual ~TRS80Term(void)
	{
	}

	int putch( int ch );

	int kbhit();

	int getch();

private:
	int			shiftTab_;
	int			shiftBack_;
};

#endif
