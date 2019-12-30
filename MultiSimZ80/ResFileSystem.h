#ifndef RESFILESYSTEM_H
#define RESFILESYSTEM_H

#include "TermCfg.h"

#if USE_RES

#include "FileSystem_I.h"


class ResFileSystem :
	public FileSystem_I
{
public:
	ResFileSystem(void);

	~ResFileSystem(void);

    virtual File_I *open( const char *filename, const char *mode ) const;

    virtual Dir_I *openDir( const char *dirname ) const;

	virtual int open( const char *filename, int mode, int access ) const;

	virtual int errno_() const;

	virtual int close( int handle ) const;

	virtual int read( int handle, void *buffer, unsigned length );

	virtual int write( int handle, void *buffer, unsigned length );

	virtual const char *strerror( int code ) const;

	virtual int lseek( int handle, int offset, int origin );
private:
	mutable const char	*strerror_;
};

#endif

#endif
