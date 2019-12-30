#ifndef RESFILE_H
#define RESFILE_H

#include "Res.h"

#if USE_RES

#include "File_I.h"

class ResFile :
	public File_I
{
public:
	ResFile(const Res *res);

	virtual ~ResFile(void);

	virtual int close();

	virtual bool eof() const;

	virtual int getc();

	virtual int putc( int c );

	virtual int seek( long offset, int origin );

	virtual int scanf( const char *format, ... );

	virtual const char *error() const;

	virtual operator bool() const;
private:
	const Res	*res_;
	const uchar	*ptr_;
	const uchar *end_;
	const char	*strerror_;
};


#endif

#endif
