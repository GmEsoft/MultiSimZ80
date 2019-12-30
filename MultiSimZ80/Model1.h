#ifndef MODEL1_H
#define MODEL1_H

// TRS-80 MODEL 1 SYSTEM

#include "arduino.h"
#include "stdio_undefs.h"
#include "TermCfg.h"

#include "TRS80.h"
#include "TRS80Term.h"
#include "SystemConsole.h"
#include "Rom.h"
#include "Ram.h"
#include "Z80CPU.h"
#include "Model1Memory.h"
#include "romLEVEL2.h"

#if USE_FDC
#include "Model1Floppies.h"
#endif

#include "Model1InOut.h"

#if USE_DISASS
#include "Z80Disassembler.h"
#endif

#if USE_RTC
#include "SystemClock.h"
#endif

#if USE_FDC
#include "WD1771.h"
#endif

#include "IRQ_I.h"

#include <memory>

#define RAM_SIZE 0xC000

extern const char romLEVEL2_data[];

class Model1 :
	public TRS80
{
	struct RTCIntHandler : public IRQ_I
	{
		RTCIntHandler( Model1 &model1 )
			: model1_( model1 )
		{
		}

		void trigger()
		{
			model1_.rtcInt();
		}
		
		Model1 &model1_;
	};

public:

	Model1() 
		: TRS80()
		, mem_( rom_, ram_, do_, /*ki_, */fdc_, floppies_, cpu_ )
		, fdc_( floppies_ )
		, floppies_( cpu_ )
		, rtcIntHandler_( *this )
	{

		rom_.setBuffer( (uchar *)romLEVEL2_data, 0x3000 );
		ram_.setBuffer( data_, RAM_SIZE );


		cpu_.setMemory( &mem_ );
		cpu_.setInOut( &io_ );

#if USE_DISASS
		disass_.setCode( &mem_ );
#endif      

    systemClock_.setClockSpeed( 1770 );
#if USE_RTC
    systemClock_.setRtcRate( 40 );
    systemClock_.setIRQ( &rtcIntHandler_ );
#endif      

	}

	virtual ~Model1()
	{
	}

	virtual void run();

  void init();
  
  void loop();
	

  uchar* data()
  {
    return ram_.getBuffer();
  }

  uint datasize()
  {
    return ram_.getSize();
  }

  uchar* code()
  {
    return rom_.getBuffer();
  }

  uint codesize()
  {
    return rom_.getSize();
  }


#if USE_FDC
	void setDiskImage( int unit, DskImage *pDskImage )
	{
		pDskImage->setFileSystem( pFileSystem_ );
		floppies_.setImage( unit, pDskImage );
	}
#endif


#if USE_FDC
	FDC &getFdc()
	{
		return fdc_;
	}
#endif

  void rtcInt();

  virtual void reset()
  {
    TRS80::reset();
    fdc_.reset();
  }


private:
	Model1Memory	mem_;
	Model1InOut		io_;

	uchar			data_[RAM_SIZE];


#if USE_FDC
	Model1Floppies	floppies_;
	WD1771			fdc_;
#endif

  RTCIntHandler rtcIntHandler_;
};

#undef RAM_SIZE

#endif

