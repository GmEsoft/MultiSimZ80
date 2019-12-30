#ifndef TRS80_I_H
#define TRS80_I_H

// TRS-80 SYSTEM

#include "TermCfg.h"
#include "System_I.h"
#include "TRS80Term.h"
#include "SystemConsole.h"
#include "ROM32.h"
#include "RAM32.h"
#include "Z80CPU.h"
#include "DskImage.h"
#include "FDC.h"
#include "DO.h"
#if USE_TRS80_KI
#include "KI.h"
#endif
#include "Z80Disassembler.h"
#include "SystemClock.h"

#include <memory>

class TRS80 :
	public System_I
{
public:

	TRS80() : pTerm_( 0 )
	{
		do_.setMode( &mode_ );
		do_.setTerm( &term_ );
#if USE_TRS80_KI
		ki_.setTerm( &term_ );
#endif
		systemConsole_.setSystem( this );
		systemConsole_.setConsole( &do_ );
		cpu_.setMode( &mode_ );
		cpu_.setConsole( &systemConsole_ );
		cpu_.setClock( &systemClock_ );
	}

	virtual ~TRS80()
	{
	}

	virtual void run() = 0;
	
	virtual void stop()
	{
		mode_.setMode( MODE_STOP );
	}
	
	virtual void reset()
	{
		cpu_.reset();
	}

	virtual void exit()
	{
		mode_.setMode( MODE_EXIT );
	}

	virtual void wakeup()
	{
	}

	virtual void step()
	{
	}

	virtual void callstep()
	{
	}

	virtual uchar* data() = 0;

	virtual uint datasize() = 0;

	virtual uchar* code() = 0;

	virtual uint codesize() = 0;

	void setTerm( Term_I *pTerm )
	{
		pTerm_ = pTerm;
		term_.setTerm( pTerm );
	}

	void setFileSystem( FileSystem_I *pFileSystem )
	{
		pFileSystem_ = pFileSystem;
		cpu_.setFileSystem( pFileSystem );
	}

	void setSleeper( Sleeper_I *sleeper )
	{
		systemClock_.setSleeper( sleeper );
	}

	virtual void setDiskImage( int unit, DskImage *pDskImage ) = 0;

	DO *getDO()
	{
		return &do_;
	}

#if USE_TRS80_KI
	KI *getKI()
	{
		return &ki_;
	}
#endif

	virtual FDC &getFdc() = 0;

	TRS80Term *getTRS80Term()
	{
		return &term_;
	}

protected:
	Z80CPU			cpu_;
	Z80Disassembler	disass_;
	ROM32			rom_;
	RAM32			ram_;
	DO				do_;
#if USE_TRS80_KI
	KI				ki_;
#endif
	SystemClock		systemClock_;
  Term_I			*pTerm_;
	SystemConsole	systemConsole_;
	Mode			mode_;
	FileSystem_I	*pFileSystem_;

private:
	TRS80Term		term_;
};

#endif

