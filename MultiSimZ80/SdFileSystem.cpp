#include "SdFileSystem.h"
#include "SdFile.h"
#include "SdDir.h"

#include <string.h>

static const char *NO_ERROR = "";
static const char *ERROR_NOT_FOUND = "File Not Found";
static const char *ERROR_INVALID = "Invalid Function";
static const char *ERROR_NOT_DIR = "File is not a directory";
//static const char *ERROR_NOT_MOUNTED = "Failed to mount SD";
//static const char *ERROR_NOT_OPEN = "Failed to open SD";

SdFileSystem::SdFileSystem( fs::FS &fs ) 
    : fs_( fs ), strerror_( NO_ERROR )
{
}

SdFileSystem::~SdFileSystem(void)
{
//    fs_.close();
}


File_I *SdFileSystem::open( const char *filename, const char *mode ) const
{
    char path[41] = "/";
    strcpy( path+1, filename );
	File file = fs_.open( path );
	if ( file )
	{
		strerror_ = NO_ERROR;
		return new SdFile( file );
	}
	strerror_ = ERROR_NOT_FOUND;
    Serial.println( path );
    Serial.println( strerror_ );
	return NULL;
}

Dir_I *SdFileSystem::openDir( const char *dirname ) const
{
    char path[41] = "/";
    strcpy( path+1, dirname );
    File file = fs_.open( path );
    if ( !file )
    {
        strerror_ = ERROR_NOT_FOUND;
        return NULL;
    }
    if ( !file.isDirectory() )
    {
        strerror_ = ERROR_NOT_DIR;
        return NULL;
    }
    strerror_ = NO_ERROR;
    return new SdDir( file );
}

int SdFileSystem::open( const char *filename, int mode, int access ) const
{
	strerror_ = ERROR_INVALID;
	return 0;
}

int SdFileSystem::errno_() const
{
	return *strerror_ ? 1 : 0;
}

int SdFileSystem::close( int handle ) const
{
	strerror_ = ERROR_INVALID;
	return 0;
}

int SdFileSystem::read( int handle, void *buffer, unsigned length )
{
	strerror_ = ERROR_INVALID;
	return 0;
}

int SdFileSystem::write( int handle, void *buffer, unsigned length )
{
	strerror_ = ERROR_INVALID;
	return 0;
}

const char *SdFileSystem::strerror( int code ) const
{
	return strerror_;
}

int SdFileSystem::lseek( int handle, int offset, int origin )
{
	strerror_ = ERROR_INVALID;
	return 0;
}
