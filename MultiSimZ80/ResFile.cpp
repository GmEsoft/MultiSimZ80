#include "ResFile.h"

#if USE_RES

#include <cstdarg>

static const char *NO_ERROR = "";
static const char *ERROR_INVALID = "Invalid Function";

ResFile::ResFile(const Res *res) :
	res_( res ), ptr_( res->get() ), end_( res->get() + res->len() ), strerror_( NO_ERROR )
{
}

ResFile::~ResFile(void)
{
}

int ResFile::close()
{
	return 0;
}

bool ResFile::eof() const
{
	return ptr_ >= end_;
}

int ResFile::getc()
{
	return eof() ? -1 : *ptr_++;
}

int ResFile::putc( int c )
{
	strerror_ = ERROR_INVALID;
	return -1;
}

int ResFile::seek( long offset, int origin )
{
	switch ( origin )
	{
	case 0:
		ptr_ = res_->get() + offset;
		break;
	case 1:
		ptr_ += offset;
		break;
	case 2:
		ptr_ = end_ + offset;
		break;
	}
	return 0;
}


int ResFile::scanf( const char *format, ... )
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

const char *ResFile::error() const
{
	return strerror_;
}

ResFile::operator bool() const
{
	return res_ != 0;
}

#endif
