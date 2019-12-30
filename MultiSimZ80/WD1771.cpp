#include "WD1771.h"

#define trace(x)
#define log(x)
#define cprintf(x)
#define simStop(x) brk()
#define getch()
#define tgetch()

static void brk()
{
	static int ctr=0;
	++ctr;
}

// Model I Read byte from FDC controller
uchar WD1771::in(ushort addr)
{
    uchar byte = 0;

	int &count = floppies_.getCount();

	switch ( addr ) 
	{
        case 0:
        	// CMD/STA register
            byte = status_;

			if ( byte == 0xFF )
				break;

            if ( ( cmd_ & 0x80 ) == 0x00 )
            {
				// we are in a TYPE 1 command (head move)
				if ( status_ & 1 ) 
				{
					if ( ( cmd_ >> 2 ) & 1 ) 
					{
						status_ |= floppies_.seek( track_, sector_, 1 );
					}
					floppies_.irq();
					log( fprintf( logfile, "FDC Type I Status read - IRQ\r" ) );
					trace( cprintf( "FDC Type I Status read - IRQ\r" ) );
					status_ &= ~1; // reset BSY flag for next read
				}
			}

			if ( ( cmd_ & 0xF0 ) == 0x00 )
            {
				// we are in a RESTORE command
				if ( floppies_.isDisk() )
					byte |= 4;					// track_ 0
			}

			if ( ( cmd_ & 0x80 ) == 0x00 || ( cmd_ & 0xF0 ) == 0xD0 ) 
			{
				// we are in type I or type IV command (force interrupt)
				// => generate index pulse
				byte &= ~2;
				if ( floppies_.isDisk() && --count <= 0) 
				{
					//cprintf("Index %X\r\n", fdcselect);
                    byte |= 2;
                    count = 100;
                }
            }

			if ( byte != lastStatus_ )
			{
				log( fprintf( logfile, "FDC RD status_=%x (cmd_=%x)\r\n", byte, cmd_ ) );
				lastStatus_ = byte;
			}
			else
			{
				log( fprintf( logfile, "." ) );
			}

			break;
        case 1:
        	// TRACK register
            byte = track_;
            break;
        case 2:
        	// SECTOR register
            byte = sector_;
//			cprintf("RD Port=%x byte=%x\r\n", port, byte);
            break;
        case 3: 
			// DATA register
            if ( floppies_.isDisk() ) 
			{
                byte = floppies_.getByte();
				if ( --count == 0 ) 
				{
                    // end of sector_ reached
                    status_ |= floppies_.getFlags() & 0x68;
                    if ( floppies_.getFormat() == 'D' )
                    {
						ushort fdccrc = floppies_.getCrc();
						ushort crc = ( floppies_.getByte() << 8 ) | floppies_.getByte();
						if ( fdccrc != crc )
						{
							trace( simStop("FDC RD Port=%x byte=%x: CRC %04x <> CRC %04x\r\n"
								"      DSK=%d TRACK=%d SIDE=%d SECT=%d\r\n",
								port, byte, fdccrc, crc1, fdcselect, track_, fdcside, sector_ ) );
							status_ |= 0x08; // CRC Error
						}
						else
						{
							tgetch();
						}
					}
					status_ &= ~3; // reset busy & drq
                }
            }
            break;
        default:
        	// Undefined READ command => stop
//			simStop("FDC RD Port=%x byte=%x: undefined port!\r\n", port, byte);
			break;
    }
    return byte;
}

uchar WD1771::out( ushort addr, uchar byte )
{
	int steps;
	
	uchar &ptrack = floppies_.getTrack();
	int &count = floppies_.getCount();

    switch ( addr ) 
	{
        case 0:
        	// CMD/STA register
            cmd_ = byte;
			lastStatus_ = 0;
            switch ( byte & 0xF0 )
			{
                case 0x00:  // restore
					ptrack = 0;
					track_ = 0;
                    status_ = 1;
					break;
                case 0x10:  // seek
                    status_ = 1;
					steps = (int)data_ - (int)track_;
                    ptrack += steps;
					track_ += steps;
					break;
                case 0x50:  // step-in
                    status_ = 1;
                    if ( ptrack < 255 )
						ptrack++, track_++;
					else
						simStop( "FDC STEP-IN>255" );
					 // TODO: check U flag to inc track_

					break;
                case 0x70:  // step-out
                    status_ = 1;
					if ( ptrack > 0 )
						ptrack--, track_--;
					else
						simStop( "FDC STEP-OUT<0" );
					 // TODO: check U flag to dec track_

					break;
                case 0x80: // read
					status_ |= floppies_.seek( track_, sector_, 0 );
                    status_ |= 3;
					break;
                case 0xA0: // write
					status_ |= floppies_.seek( track_, sector_, 0 );
                    status_ |= 3;
                    break;
				case 0xC0: // read address
					status_ |= floppies_.seek( track_, sector_, 1 );
					count = 6;
					status_ |= 3;
					break;
                case 0xD0: // set interrupt
					break;
                case 0xF0: // ???
                    break;
                default:
//                    simStop("FDC WR Port=%x byte=%x: undefined cmd_\r\n", port, byte);
					break;
            }
            break;
        case 1:
        	// TRACK register
			lastStatus_ = 0;
            track_ = byte;
            break;
        case 2:
        	// SECTOR register
			lastStatus_ = 0;
            sector_ = byte;
            break;
        case 3:
        	// DATA register
			lastStatus_ = 0;
			data_ = byte;
            if ( (status_ & 2) == 2 &&
                  (cmd_ & 0xF0) == 0xA0 &&
                    floppies_.isDisk() ) 
			{
				floppies_.putByte(byte);

                if ( --count == 0 ) 
				{
                    // end of sector_ reached
                    status_ = 0;
                    if ( floppies_.getFormat() == 'D' )
                    {
						ushort crc = floppies_.getCrc();
						floppies_.putByte( crc >> 8 );
						floppies_.putByte( crc & 0xFF );
					}

					floppies_.irq();
                }
            }

            break;
    }

	return byte;
}

void WD1771::reset()
{
	floppies_.reset();
	cmd_ = 3;
	track_ = 0;
	status_ = 1;
}
