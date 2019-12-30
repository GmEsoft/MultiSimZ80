#include "Model1.h"

#include <ctype.h>

static unsigned ttyTimer = 0;
static uint lastpc = 0, pc = 0;

#if USE_TRS80_DO
static int fontBank = 1;
#endif

void Model1::run()
{
    init();
    
	while( mode_.getMode() != MODE_EXIT )
	{
        loop();
	}

	if ( mode_.getMode() != MODE_EXIT )
	{
		pTerm_->printf( "\nSTOP: pc=%04X - lastpc=%04X", pc, lastpc );
		pTerm_->getch();
	}

	pTerm_->printf( "\n" );
}

void Model1::init()
{
#if USE_TRS80_DO
    do_.setFont( 0 );
    do_.selectFont( fontBank );
#endif   

    pTerm_->textAttr( 0x0B );
    pTerm_->setFullScreen( true );
    cpu_.reset();
    cpu_.setBreakOnHalt( false );
    mode_.setMode( MODE_RUN );
}

void Model1::loop()
{
    lastpc = pc;
    pc = cpu_.getPC();

    if ( mode_.getMode() != MODE_RUN )
    {
#if USE_DISASS        
        disass_.setPC( pc );
#endif            
        pTerm_->printf( "\n%04X  ", pc );
#if USE_DISASS        
        const char *src = disass_.source();
        int i = 0;
        for ( ; pc < disass_.getPC(); ++pc, ++i )
            pTerm_->printf( "%02X", mem_.read( pc ) );
        for ( ; i < 4; ++i )
            pTerm_->printf( "  " );
        pTerm_->printf( "  %s", src );
#else
        int i = 0;
        for ( ; i<4; ++pc, ++i )
            pTerm_->printf( "%02X", mem_.read( pc ) );
#endif            
        pc = cpu_.getPC();
        int c = toupper( systemConsole_.getch() );
//        if ( !c )
//            return;
        if ( c == 'G' )
        {
#if USE_TRS80_DO
            do_.setDisplayUpdated( true );
#endif
            mode_.setMode( MODE_RUN );

            pTerm_->clrScr();
        }
        else if ( c == 'F' )
        {
#if USE_TRS80_DO
            fontBank = 1 - fontBank;
            do_.selectFont( fontBank );
#endif
        }
    }
    else
    {
        if ( !ttyTimer-- )
        {
            ttyTimer = 0x1000;
#if USE_TRS80_KI
            if ( systemConsole_.kbhit() )
            {
                ki_.setKeyPressed( systemConsole_.getch() );
            }
#else
#if !USE_MCORE
            do_.updateDisplay();
#endif
#endif
        }
    }
    cpu_.sim();
}

void Model1::rtcInt()
{
  if ( mem_.model1IntMask_.get() & 0x20 )
  {
    mem_.model1IntLatch_.and_( ~0x20 );
    cpu_.trigIRQ( ~mem_.model1IntLatch_.get() );
  }
}


