#include "Model1Memory.h"

uchar Model1Memory::read( ushort addr )
{
    if ( addr < 0x3000 ) // ROM
    {
        return rom_.read( addr );
    }
    else if ( addr >= 0x4000 ) // RAM
    {
        return ram_.read( addr - 0x4000 );
    }
    else if ( addr >= 0x3C00 && addr <= 0x3FFF ) // DO read
    {
        return do_.read( addr & 0x03FF );
    }
    else if ( addr == 0x37E0 ) // INT latch
    {
        uchar byte = model1IntLatch_.get();
        model1IntLatch_.or_( 0x20 ); // reset latch
        cpu_.trigIRQ( ~model1IntLatch_.get() );
        return byte;
    }
    else if ( addr == 0x37E1 ) // floppy drive select
    {
        return floppies_.get();
    }		
    else if ( addr >= 0x37EC && addr <= 0x37EF ) // FDC i/o
    {
        return fdc_.in( addr & 3 );
    }
#if USE_TRS80_KI
    else if ( addr >= 0x3800 && addr <= 0x3BFF ) // KI scan
    {
        return ki_.read( addr & 0x03FF );
    }
#endif
    else
    {
        return 0;
    }
}

uchar Model1Memory::write( ushort addr, uchar data )
{
    if ( addr >= 0x4000 ) // RAM
    {
        return ram_.write( addr - 0x4000, data );
    }
    else if ( addr >= 0x3C00 && addr <= 0x3FFF ) // DO write
    {
        return do_.write( addr & 0x03FF, data );
    }
    else if ( addr == 0x37E0 ) // INT latch
    {
        return model1IntLatch_.get();
    }
    else if ( addr == 0x37E1 ) // FDC drive select
    {
        return floppies_.put( data );
    }		
    else if ( addr >= 0x37EC && addr <= 0x37EF ) // FDC i/o
    {
        return fdc_.out( addr & 3, data );
    }
    else
    {
        return data;
    }
}

