#ifndef SDFILESYSTEM_H
#define SDFILESYSTEM_H

#include "FileSystem_I.h"

#include "FS.h"
#include "stdio_undefs.h"
#include "File_I.h"

class SdFileSystem :
	public FileSystem_I
{
public:
	SdFileSystem( fs::FS &fs );

	virtual ~SdFileSystem( void );

    virtual File_I *open( const char *filename, const char *mode ) const;

    virtual Dir_I *openDir( const char *direname ) const;

	virtual int open( const char *filename, int mode, int access ) const;

	virtual int errno_() const;

	virtual int close( int handle ) const;

	virtual int read( int handle, void *buffer, unsigned length );

	virtual int write( int handle, void *buffer, unsigned length );

	virtual const char *strerror( int code ) const;

	virtual int lseek( int handle, int offset, int origin );
private:
	mutable const char	*strerror_;
    fs::FS &fs_;
};

#endif

