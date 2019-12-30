#ifndef SDFILE_H
#define SDFILE_H

#include "FS.h"
#include "stdio_undefs.h"
#include "File_I.h"

class SdFile :
	public File_I
{
public:
	SdFile( File& file );

	virtual ~SdFile(void);

	virtual int close();

	virtual bool eof() const;

	virtual int getc();

	virtual int putc( int c );

	virtual int seek( long offset, int origin );

	virtual int scanf( const char *format, ... );

	virtual const char *error() const;

	virtual operator bool() const;
private:
    mutable File    file_;
	const char      *strerror_;
};


#endif
