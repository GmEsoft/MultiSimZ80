#ifndef CONSOLE_I_H
#define CONSOLE_I_H

#include "Mode.h"

#include <stdarg.h>

// STANDARD CONSOLE API

class Console_I
{
public:

	virtual int kbhit() = 0;
	virtual int getch() = 0;
	virtual int ungetch( int ch ) = 0;
	virtual int putch( int ch ) = 0;
	virtual int puts( const char * str ) = 0;
	virtual char *gets( char *str ) = 0;
	virtual int vprintf( const char *format, va_list args ) = 0;

	int printf( const char* format, ... )
	{
		va_list args;
		va_start( args, format );
		return vprintf( format, args );
	}

	// destructor
	virtual ~Console_I()
	{
	}
};

#endif
