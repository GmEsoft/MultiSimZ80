#ifndef MODEL1FLOPPIES_H
#define MODEL1FLOPPIES_H

#include "Floppies.h"
#include "Crc16.h"

class Model1Floppies :
	public Floppies
{
public:
	Model1Floppies( CPU &_cpu ) 
		: Floppies( _cpu ), select_( 0 ), mfm_( 0 ), side_( 0 )
	{
		pDskImages_[0] = pDskImages_[1] = pDskImages_[2] = pDskImages_[3] = NULL;
	}

	virtual ~Model1Floppies(void)
	{
	}

	// out char
	virtual uchar put( uchar data );

	// in char
	virtual uchar get();

	// generate interrupt
	virtual void irq()
	{
		// nothing
	}

	// seek sector
	virtual uchar seek( uchar track, uchar sector, bool trackOnly )
	{
		return pDskImages_[select_] ? pDskImages_[select_]->seek( side_, track, sector, mfm_, trackOnly ) : 0;
	}

	// is disk image mounted ?
	virtual bool isDisk()
	{
		return pDskImages_[select_] ? pDskImages_[select_]->isDisk() : false;
	}

	// get byte counter
	virtual int &getCount()
	{
		static int dummy;
		return pDskImages_[select_] ? pDskImages_[select_]->getCount() : dummy;
	}

	// get sector flags
	virtual uchar &getFlags()
	{
		static uchar dummy;
		return pDskImages_[select_] ? pDskImages_[select_]->getFlags() : dummy;
	}
		
	// get track counter
	virtual uchar &getTrack()
	{
		static uchar dummy;
		return pDskImages_[select_] ? pDskImages_[select_]->getTrack() : dummy;
	}

	// get byte and update crc
	virtual uchar getByte()
	{
		return pDskImages_[select_] ? pDskImages_[select_]->getByte() : 0;
	}

	// put byte and update crc
	virtual void putByte( uchar byte )
	{
		if ( pDskImages_[select_] )
			pDskImages_[select_]->putByte( byte );
	}

	// get DAM byte 
	virtual uchar getDamByte( ushort offset )
	{
		return pDskImages_[select_] ? pDskImages_[select_]->getDamByte( offset ) : 0;
	}

	// get crc
	virtual ushort getCrc()
	{
		return pDskImages_[select_] ? pDskImages_[select_]->getCrc() : 0;
	}

	// get format
	virtual uchar getFormat()
	{
		return pDskImages_[select_] ? pDskImages_[select_]->getFormat() : 0;
	}

	// get Image
	virtual DskImage &getImage( int unit )
	{
		return *pDskImages_[unit];
	}

	// set Image
	virtual void setImage( int unit, DskImage *pDskImage )
	{
		pDskImages_[unit] = pDskImage;
	}

	// reset
	virtual void reset()
	{
		select_ = 0;
		if ( pDskImages_[0] )
		{
			pDskImages_[0]->reset();
		}
	}

private:

	DskImage *pDskImages_[4];
	
	uchar mfm_;					// Virtual FDC MFM (dden) mode register
	uchar side_;				// Virtual FDC Side register
	uchar select_;				// Virtual FDC Drive Select latch (0..3)
};


#endif
