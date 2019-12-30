#include "ResFileSystem.h"
#include "ResFile.h"
#include "ResDir.h"

#if USE_RES

#include <string.h>

static const char *NO_ERROR = "";
static const char *ERROR_NOT_FOUND = "File Not Found";
static const char *ERROR_INVALID = "Invalid Function";

ResFileSystem::ResFileSystem(void) : strerror_( NO_ERROR )
{
}

ResFileSystem::~ResFileSystem(void)
{
}

File_I *ResFileSystem::open( const char *filename, const char *mode ) const
{
    const Res *res = Res::get( filename );
    if ( res )
    {
        strerror_ = NO_ERROR;
        return new ResFile( res );
    }
    strerror_ = ERROR_NOT_FOUND;
    return NULL;
}

Dir_I *ResFileSystem::openDir( const char *dirname ) const
{
    strerror_ = NO_ERROR;
    return new ResDir();
}

int ResFileSystem::open( const char *filename, int mode, int access ) const
{
	strerror_ = ERROR_INVALID;
	return 0;
}

int ResFileSystem::errno_() const
{
	return *strerror_ ? 1 : 0;
}

int ResFileSystem::close( int handle ) const
{
	strerror_ = ERROR_INVALID;
	return 0;
}

int ResFileSystem::read( int handle, void *buffer, unsigned length )
{
	strerror_ = ERROR_INVALID;
	return 0;
}

int ResFileSystem::write( int handle, void *buffer, unsigned length )
{
	strerror_ = ERROR_INVALID;
	return 0;
}

const char *ResFileSystem::strerror( int code ) const
{
	return strerror_;
}

int ResFileSystem::lseek( int handle, int offset, int origin )
{
	strerror_ = ERROR_INVALID;
	return 0;
}

#endif
