#ifndef DSKIMAGE_H
#define DSKIMAGE_H

#include "runtime.h"
#include "FileSystem_I.h"
#include "Crc16.h"

#include <string>

class DskImage
{
public:
	DskImage(void)
		: unit_( 0 ), diskFormat_( '\0' ), doubleSided_( false ), crc_( 0x1021 ), file_( NULL ), pFileSystem_( NULL )
		, track_( 0 ), specFormat_( 0 ), count_( 0 )
	{
	}

	void set( int unit, const std::string& fileName, char diskFormat, bool doubleSided )
	{
		unit_ = unit;
		fileName_ = fileName;
		specFormat_ = diskFormat;
		doubleSided_ = doubleSided;
	}

	virtual ~DskImage(void)
	{
	}

	void open();

	void close();

	uchar seek( uchar side, uchar track, uchar sector, uchar mfm, bool trackOnly );

	bool isDisk();

	uchar &getTrack()
	{
		return track_;
	}

	uchar getFormat()
	{
		return diskFormat_;
	}

	int &getCount()
	{
		return count_;
	}

	uchar &getFlags()
	{
		return flags_;
	}

	uchar getByte();

	void putByte( uchar byte );

	// get DAM byte 
	uchar getDamByte( ushort offset );

	void setFileSystem( FileSystem_I *pFileSystem )
	{
		pFileSystem_ = pFileSystem;
	}

	// get crc
	ushort getCrc()
	{
		return crc_.get();
	}

	void reset()
	{
		track_ = 0;
	}

private:
	struct SectorHeader 
	{
		uchar	track;
		uchar	sector;
		uchar	flags;
		uchar	dam;
		int		offset;
	};				// Sector Header structure

//    static const size_t maxSect_ = 2901;
    static const size_t maxSect_ = 801;
    
	FileSystem_I	*pFileSystem_;
	File_I			*file_;
	Crc16			crc_;					// CRC16 calculator
	int				unit_;
	std::string		fileName_;
	char			diskFormat_;
	char			specFormat_;
	bool			doubleSided_;
	uchar			track_;					// current track
	int				count_;					// Current sector byte counter
	uchar			flags_;					// Current sector flags
	SectorHeader	headers_[maxSect_];
	uchar			dam_[6];
};

#endif
