#include "DskImage.h"

#define trace(x)
#define log(x)
#define cprintf(x)
#define getch()
#define tgetch()

static int sectorsizes[] = {256,128,1024,512};


void DskImage::open()
{
	int track;
	int tracks;
	int tracklen;
	int sector;
	int offset;
	int	dam;
	int i;
	int side;
	int sides;
	int TC, SPT, SPTS, GPL, DDSL, DDGA;
	int vdiskoptions;
	int dden;
	SectorHeader *pHeader;

	trace(cprintf("openfdc:\r\n"));
    if ( file_ == NULL )
    {
		file_ = pFileSystem_->open( fileName_.c_str(), "r+b" );
	}

	if ( file_ != NULL )
    {
        // check for DiskREAD img file format
		i = file_->getc();
		i += 256 * file_->getc();

        offset = ( i == 360 || i == 720 ) ? 2 : 0;

        if ( file_ != NULL )
        {
			if ( specFormat_ == 'N' || specFormat_ == 0 &&
			  (!file_->seek( offset, SEEK_SET ) &&
            	file_->getc() == 0x00 &&
                file_->getc() == 0xFE &&
				!file_->seek( offset + 0x00FD, SEEK_SET ) && // check NEWDOS signature
				file_->getc() == 0x00 &&
                file_->getc() == 0x04/* &&
                file_->getc() == 0x01*/ ) )
            {
				// Variant JV1 file NEWDOS/80
				trace(cprintf("JV1 file for NEWDOS/80\r\n"));
				tgetch();
				file_->seek(0x200+offset,SEEK_SET);		// PDRIVE table
				DDSL = file_->getc(); 					// 0 = Disk Dir Start Lump
				file_->getc(); 							// 1
				file_->getc(); 							// 2
				TC = file_->getc(); 					// 3 = Track Count
				SPT = file_->getc(); 					// 4 = Sectors Per Track
				GPL = file_->getc(); 					// 5 = Grans Per Lump
				file_->getc(); 							// 6
				sides = file_->getc() & 0x40 ? 2 : 1;	// 7 = Grans Per Lump
				file_->getc(); 							// 8
				DDGA = file_->getc(); 					// 9 = Grans Per Lump

				trace(cprintf("sides=%d TC=%d SPT=%d GPL=%d DDSL=%d DDGA=%d\r\n",
							  sides,   TC,   SPT,   GPL,   DDSL,   DDGA ));

				trace(getch());

				i = 0;

				if ( doubleSided_ )
				{
					DDGA *= 2/sides;
					GPL *= 2/sides;
					SPT *= 2/sides;
					sides = 2;
				}

				SPTS = SPT / sides;

				for ( track=0; track<TC && i<maxSect_; track++ )
				{
					for ( side=0; side<sides && i<maxSect_; side++)
					{
						for ( sector=0; sector<SPT/sides && i<maxSect_; sector++ )
						{
							if (i==maxSect_)
							{
								cprintf("FDC More than 2901 sectors!\r\n");
								getch();
								break;
							}
							else
							{
								pHeader = &(headers_[i]);
								pHeader->track = track;
								pHeader->sector = sector;
								pHeader->flags = ( (i>=DDSL*GPL*5 && i<(DDSL*GPL+DDGA)*5) ? 0x60 : 0x00 )
									| ( side << 4 ) | ( SPTS > 10 ? 0x80 : 0x00 );
								pHeader->offset = 256*i+offset;
								trace(cprintf("DRS=%4d track=%d sector=%2d flags=%2d offset=%6x\r\n",
									i, track, sector, pHeader->flags, pHeader->offset ));
								++i;
							}
						}
						tgetch();
					}
				}
                diskFormat_ = '1';
            }
            else if (file_->seek(offset,SEEK_SET),
            	file_->getc() == 0x00 &&
                file_->getc() == 0xFE &&
                file_->getc() == 0x11)
            {
				// JV1 file
				trace(cprintf("JV1 file, SS-SD\r\n"));
				tgetch();
                for (i=0; i<maxSect_; i++)
                {
                    headers_[i].track = i/10;
                    headers_[i].sector = i%10;
                    headers_[i].flags = (i/10==0x11) ? 0x60 : 0x00;
                    headers_[i].offset = 256*i + offset;
                }
                diskFormat_ = '1';
            }
            else if ( specFormat_ == 'J' || specFormat_ == 0 &&
			  ( file_->seek(offset,SEEK_SET),
            	file_->getc() == 0x00 &&
                file_->getc() == 0xFE &&
                file_->getc() == 0x14 ) )
            {	
				// JV1 file
				if ( doubleSided_ )
				{
					SPT = 36;
					sides = 2;
					trace(cprintf("JV1 file, DS-DD\r\n"));
				}
				else
				{
					SPT = 18;
					sides = 1;
					trace(cprintf("JV1 file, SS-DD\r\n"));
				}
				tgetch();

				SPTS = SPT / sides;
                for (i=0; i<maxSect_; i++)
                {
					pHeader = &(headers_[i]);
					side = ( i / SPTS ) % sides;
                    headers_[i].track = i / SPT;
                    headers_[i].sector = i % SPTS;
                    headers_[i].flags = 
						( ( i / SPT == 20 ) ? 0x60 : 0x00 )
						| ( side << 4 ) 
						| ( SPTS > 10 ? 0x80 : 0x00 );
                    headers_[i].offset = 256*i + offset;
					trace(cprintf("DRS=%4d track=%d sector=%2d flags=%2d offset=%6x\r\n",
						i, pHeader->track, pHeader->sector, pHeader->flags, pHeader->offset ));
					if ( i % SPTS == 0 ) { tgetch(); };
                }
                diskFormat_ = '1';
            }
            else if (	( file_->seek(3,SEEK_SET),file_->getc() == 0x19 )
					||	( file_->seek(3,SEEK_SET),file_->getc() == 0x0C )
					)
            {
				// DMK format
				trace(cprintf( "DMK file\r\n" ));
				file_->seek(0,SEEK_SET);
				diskFormat_ = 'D';
				file_->getc();	// RW if 00, RO if FF
				tracks = file_->getc();
				tracklen = file_->getc();
				tracklen += ( file_->getc() << 8 );
				vdiskoptions = file_->getc();
				tracks <<= ( (~vdiskoptions) >> 4 ) & 1;
				trace( cprintf( "Tracks:%d TrackLen:%x (VDiskOptions:%x)\r\n", tracks, tracklen, vdiskoptions ) );
				tgetch();
				i = 0;
				for ( track = 0; track < tracks; track++ )
				{
					trace(cprintf("Phys Track:%d\r\n",track));
					for ( sector=0; sector<64; sector++ )
					{
						if (i==maxSect_)
						{
							cprintf("FDC More than 2901 sectors!\r\n");
							getch();
							break;
						}
						file_->seek( 0x10 + track*tracklen + sector*2, SEEK_SET );
						offset = file_->getc();
						offset += ( file_->getc() << 8 );
						if (offset<=0)
							break;
						dden = ( offset >> 15 ) & 1;
						offset = 0x10 + track*tracklen + (offset&0x3FFF) + 1;
						file_->seek( offset, SEEK_SET );

						headers_[i].track = file_->getc() ;
						headers_[i].flags = ( file_->getc() << 4 ) | ( dden << 7 );
						headers_[i].sector = file_->getc() ;
						if ( dden )
							offset += 0x1A;
						else
							offset += 0x15;

						file_->seek( offset, SEEK_SET );
						while ( file_->getc() != 0 )
							offset += 1;
						file_->seek( offset, SEEK_SET );

						while ( file_->getc() == 0 )
							offset += 1;

						if ( dden )
							offset += 3;

						file_->seek( offset, SEEK_SET );
						dam = file_->getc() ;
						++offset;
						headers_[i].flags |= ( ( ~dam ) & 1 ) << 5;
						headers_[i].dam = dam;

						headers_[i].offset = offset;
						i++;
					}
				}
				tgetch();
            }
            else
            {
				// JV3 file
				trace(cprintf("JV3 file\r\n"));
				tgetch();
                file_->seek( 0, SEEK_SET );
                for (i=0; i<maxSect_; i++) {
                    headers_[i].track = file_->getc();
                    headers_[i].sector = file_->getc();
                    headers_[i].flags = file_->getc();
                    headers_[i].offset = 3 * 2901 + 1 + 256 * i;
                }
                diskFormat_ = '3';
            }
			trace( cprintf("loaded.\r\n") );
			tgetch();
            return ;
        }
    }

    //cprintf("FDC Failed opening %s\r\n", filename);
    //getch();
}

void DskImage::close()
{
	if ( isDisk() )
	{
		file_->close();
		file_ = NULL;
	}
}

uchar DskImage::seek( uchar side, uchar track, uchar sector, uchar mfm, bool trackOnly )
{
	uchar status = 0;

	int i, j;

	if ( isDisk() && track == track_ )
	{
		for (i=0; i<maxSect_; i++) 
		{
			if ( headers_[i].track == track &&
				( trackOnly || headers_[i].sector == sector ) &&
				( headers_[i].flags & 0x10 ) == ( side << 4 ) &&
				( headers_[i].flags & 0x80 ) == ( mfm << 7 )
				) 
			{
				// ID Field
				crc_.reset();
				dam_[0] = headers_[i].track;					// Track addr
				dam_[1] = ( headers_[i].flags & 0x10 ) >> 4;	// Side number
				dam_[2] = headers_[i].sector;					// sector address
				dam_[3] = headers_[i].flags & 0x03;				// sector length (256)
				for ( j=0; j<4; ++j )
				{
					crc_.update( dam_[j] );
				}
				ushort crc = crc_.get();
				dam_[4] = ( crc >> 8 ) & 0xFF;					// CRC 1
				dam_[5] = crc & 0xFF;							// CRC 2

				file_->seek( headers_[i].offset,SEEK_SET);
				count_ = sectorsizes[headers_[i].flags & 0x03];
				flags_ = headers_[i].flags;

				// Preset CRC
				crc_.reset();		// reset crc
				if ( headers_[i].flags & 0x80 )
				{	// double density
					crc_.update( 0xA1 );
					crc_.update( 0xA1 );
					crc_.update( 0xA1 );
				}
				crc_.update( headers_[i].dam );

				return status;
			}
		}

		status |= trackOnly ? 0x10 : 0x10; // Seek error or record not found error
		log( fprintf( logfile, "=== FDC Seek error: %d H:%d T:%d S:%d - not found\r\n", fdcselect,side,track ,sector ) );
		trace( simStop("FDC Seek error: %d H:%d T:%d S:%d - not found\r\n", fdcselect,side,track ,sector ) );
		//FDCint();
		return status;
	}

	status |= trackOnly ? 0x10 : 0x10; // Seek error or record not found error
	log( fprintf( logfile, "=== FDC Seek error: %d H:%d T:%d S:%d - no disk\r\n", fdcselect,side,track ,sector ));
	trace( simStop("FDC Seek error: %d H:%d T:%d S:%d - no disk\r\n", fdcselect,side,track ,sector ) );

	return status;
}

bool DskImage::isDisk()
{
	return file_ != NULL;
}

uchar DskImage::getByte()
{
	if ( isDisk() )
	{
		uchar byte = file_->getc();
		crc_.update( byte );
		return byte;
	}
	return 0;
}

void DskImage::putByte( uchar byte )
{
	if ( isDisk() )
	{
		file_->putc( byte );
		crc_.update( byte );
	}
}

uchar DskImage::getDamByte( ushort offset )
{
	if ( isDisk() )
	{
		return dam_[offset];
	}
	return 0;
}

