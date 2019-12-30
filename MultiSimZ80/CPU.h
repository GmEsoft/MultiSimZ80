#ifndef CPU_H
#define CPU_H

// CPU API

#include "runtime.h"
#include "Memory_I.h"
#include "Clock_I.h"
#include "InOut_I.h"
#include "Mode.h"
#include "Console_I.h"
#include "FileSystemProxy.h"

class CPU : public FileSystemProxy
{
public:

	CPU()
		: code_( 0 ), mem_( 0 ), io_( 0 ), mode_( 0 ), clock_( 0 ), console_( 0 )
	{
	}

	// set PC
	void setPC( uint pc )
	{
		pc_ = pc;
	}

	// get PC
	uint getPC()
	{
		return pc_;
	}

	// Set Memory Handler
	void setMemory( Memory_I *mem )
	{
		mem_ = mem;
		if ( !code_ )
			code_ = mem;
	}

	// Set Code Handler
	void setCode( Memory_I *mem )
	{
		code_ = mem;
	}

	// Set I/O Handler
	void setInOut( InOut_I *io )
	{
		io_ = io;
	}

	// Set Mode Handler
	void setMode( Mode *mode )
	{
		mode_ = mode;
	}

	// Set Clock
	void setClock( Clock_I *clock )
	{
		clock_ = clock;
	}

	// Set Console
	void setConsole( Console_I *console )
	{
		console_ = console;
	}

	uchar getcode( ushort addr )
	{
		return code_ ? code_->read( addr ) : 0xFF;
	}

	uchar getdata( ushort addr )
	{
		return mem_ ? mem_->read( addr ) : 0xFF;
	}

	uchar putdata( ushort addr, uchar data )
	{
		return mem_ ? mem_->write( addr, data ) : data;
	}

	uchar indata( ushort addr )
	{
		return io_? io_->in( addr ) : 0xFF;
	}

	uchar outdata( ushort addr, uchar data )
	{
		return io_ ? io_->out( addr, data ) : data;
	}

	uchar fetch()
	{
		return getcode( pc_++ );
	}

	// Set mode
	void setMode( ExecMode pMode )
	{
		if ( mode_ )
			mode_->setMode( pMode );
	}

	// get mode
	ExecMode getMode()
	{
		return mode_ ? mode_->getMode() : MODE_RUN;
	}

	// wait
	void wait()
	{
		//...
	}

	// run cycles
	void runcycles( long cycles )
	{
		if ( clock_ )
			clock_->runCycles( cycles );
	}

	// write char to console
	int putch( int ch )
	{
		return console_ ? console_->putch( ch ) : 0;
	}

	// scan char from console
	int kbhit()
	{
		return console_ ? console_->kbhit() : 0;
	}

	// read char from console
	int getch()
	{
		return console_ ? console_->getch() : 0;
	}

	// write char to console
	int ungetch( int ch )
	{
		return console_ ? console_->ungetch( ch ) : 0;
	}

	// Trigger IRQ interrupt
	virtual void trigIRQ( char irq ) = 0;

	// Get IRQ status
	virtual char getIRQ( void ) = 0;

	// Trigger NMI interrupt
	virtual void trigNMI( void ) = 0;

	// Get NMI status
	virtual char getNMI( void ) = 0;

	// CPU Reset
	virtual void reset() = 0;

	// Execute 1 Statement
	virtual void sim() = 0;

	// destructor
	virtual ~CPU()
	{
	}

protected:
	ushort pc_;
	Console_I *console_;

private:
	Memory_I *mem_;
	Memory_I *code_;
	InOut_I *io_;
	Mode *mode_;
	Clock_I *clock_;
};

#endif
