#include "SdFile.h"

#include <cstdarg>

static const char *NO_ERROR = "";
static const char *ERROR_INVALID = "Invalid Function";
static const char *ERROR_NOTOPEN = "Failed to open file";

SdFile::SdFile( File &file ) :
	file_( file ), strerror_( NO_ERROR )
{
    if ( !file_ )
    {
        strerror_ = ERROR_NOTOPEN;
    }
    Serial.println( strerror_ );
}

SdFile::~SdFile(void)
{
    close();
}

int SdFile::close()
{
    if ( file_ )
    {
        file_.close();
        return 0;
    }
	return -1;
}

bool SdFile::eof() const
{
	return !file_.available();
}

int SdFile::getc()
{
	return eof() ? -1 : file_.read();
}

int SdFile::putc( int c )
{
//    Serial.println("SdFile::putc( int c )");
    file_.write( c );
}

int SdFile::seek( long offset, int origin )
{
//    Serial.println("SdFile::seek( long offset, int origin )");
	switch ( origin )
	{
	case 0:
		file_.seek( offset );
		break;
	case 1:
		file_.seek( file_.position() + offset );
		break;
	case 2:
		file_.seek( file_.size() + offset );
		break;
	}
	return 0;
}


int SdFile::scanf( const char *format, ... )
{
/*
	va_list args;
	va_start( args, format );
	void *a[20];
	for ( int i = 0; i < sizeof( a ) / sizeof( a[0] ); ++i ) 
		a[i] = va_arg(args, void *);
	// no vfscanf() in VS before VS2013 :-(
	int ret = fscanf( file, format, 
		a[0], a[1], a[2], a[3], a[4], 
		a[5], a[6], a[7], a[8], a[9], 
		a[10], a[11], a[12], a[13], a[14], 
		a[15], a[16], a[17], a[18], a[19]
	);
	va_end( args );
	return ret;
*/
	strerror_ = ERROR_INVALID;
	return -1;
}

const char *SdFile::error() const
{
	return strerror_;
}

SdFile::operator bool() const
{
	return file_ && bool( file_ );
}

